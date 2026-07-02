import {
  collection,
  doc,
  getDocs,
  setDoc,
  updateDoc,
  deleteDoc,
  serverTimestamp,
  type Timestamp,
} from 'firebase/firestore';
import { firestore } from './firebase';
import { AuthService } from './auth';
import { Utils, refreshIcons } from './utils';
import { i18n } from './i18n';
import type { UserDocument } from './types';

interface UserWithId extends UserDocument {
  id: string;
}

export const UserManagement = {
  _users: [] as UserWithId[],

  render(container: HTMLElement): void {
    if (!AuthService.isAdmin()) {
      container.innerHTML = `<div class="content-wrapper"><div class="card" style="text-align:center; padding: 60px; color: var(--text-secondary);">${i18n.t('no_access')}</div></div>`;
      return;
    }

    container.innerHTML = `
      <div class="content-wrapper">
        <div class="detail-header">
          <h1>${i18n.t('user_management')}</h1>
          <button id="btn-add-user" class="btn btn-primary">
            <i data-lucide="plus-circle"></i> ${i18n.t('add_user')}
          </button>
        </div>
        <div class="table-wrapper">
          <table class="table">
            <thead>
              <tr>
                <th>${i18n.t('email')}</th>
                <th>${i18n.t('role')}</th>
                <th>${i18n.t('last_seen')}</th>
                <th>${i18n.t('created')}</th>
                <th>${i18n.t('actions')}</th>
              </tr>
            </thead>
            <tbody id="users-table-body">
              <tr><td colspan="5"><div class="loading-container"><div class="spinner"></div></div></td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;

    refreshIcons();
    void this._loadUsers();
    this._setupListeners();
  },

  _setupListeners(): void {
    document.getElementById('btn-add-user')?.addEventListener('click', () => this._showAddModal());

    // Event delegation for table actions
    document.getElementById('users-table-body')?.addEventListener('click', (e) => {
      const btn = (e.target as HTMLElement).closest('[data-action]') as HTMLElement | null;
      if (!btn) return;
      const { action, id, role } = btn.dataset;
      if (action === 'toggle-role' && id && role) {
        void this._toggleRole(id, role as 'admin' | 'viewer');
      } else if (action === 'delete' && id) {
        this._confirmDelete(id);
      }
    });
  },

  async _loadUsers(): Promise<void> {
    const tbody = document.getElementById('users-table-body') as HTMLElement;
    try {
      const snapshot = await getDocs(collection(firestore, 'users'));
      this._users = [];
      snapshot.forEach((d) => {
        this._users.push({ id: d.id, ...(d.data() as UserDocument) });
      });

      this._renderTable();
    } catch (error) {
      console.error('Failed to load users:', error);
      tbody.innerHTML = `<tr><td colspan="5" style="text-align:center; color:var(--status-alert); padding:30px;">${i18n.currentLang === 'de' ? 'Fehler beim Laden.' : 'Error loading.'}</td></tr>`;
    }
  },

  _renderTable(): void {
    const tbody = document.getElementById('users-table-body') as HTMLElement;

    if (this._users.length === 0) {
      tbody.innerHTML = `<tr><td colspan="5" style="text-align:center; padding:40px; color:var(--text-secondary);">${i18n.t('no_users_found')}</td></tr>`;
      return;
    }

    tbody.innerHTML = this._users.map((u) => {
      const roleBadge = u.role === 'admin'
        ? `<span class="badge badge--info">${i18n.t('role_admin')}</span>`
        : `<span class="badge badge--offline" style="border:none;">${i18n.t('role_viewer')}</span>`;

      const lastSeenAt = u.lastSeenAt as Timestamp | null | undefined;
      const createdAt = u.createdAt as Timestamp | null | undefined;
      const loginStr = Utils.formatTimestamp(lastSeenAt ? lastSeenAt.toDate() : null);
      const createdStr = Utils.formatTimestamp(createdAt ? createdAt.toDate() : null);

      const isSelf = (AuthService.currentUser?.uid === u.id);

      return `
        <tr>
          <td class="td--primary">${u.email ?? u.id}</td>
          <td>${roleBadge}</td>
          <td style="font-size:0.8rem;">${loginStr}</td>
          <td style="font-size:0.8rem;">${createdStr}</td>
          <td>
            <div style="display: flex; gap: 6px;">
              ${!isSelf ? `
                <button class="btn btn-ghost" style="font-size:0.78rem;"
                  title="${u.role === 'admin' ? (i18n.currentLang === 'de' ? 'Zu Betrachter herabstufen' : 'Demote to Viewer') : (i18n.currentLang === 'de' ? 'Zu Admin hochstufen' : 'Promote to Admin')}"
                  data-action="toggle-role" data-id="${u.id}" data-role="${u.role}">
                  <i data-lucide="${u.role === 'admin' ? 'shield-off' : 'shield-alert'}"></i>
                </button>
                <button class="btn btn-danger btn-icon" title="${i18n.t('delete')}"
                  data-action="delete" data-id="${u.id}">
                  <i data-lucide="trash-2"></i>
                </button>
              ` : `<span style="font-size:0.75rem; color: var(--text-muted);">${i18n.currentLang === 'de' ? 'Ich selbst' : 'Myself'}</span>`}
            </div>
          </td>
        </tr>
      `;
    }).join('');

    refreshIcons();
  },

  _showAddModal(): void {
    Utils.openModal(`
      <h3 class="modal__title">${i18n.t('add_user')}</h3>
      <p style="color: var(--text-secondary); font-size: 0.85rem; margin-bottom: var(--space-4);">
        ${i18n.currentLang === 'de' ? 'Der Benutzer wird beim nächsten Login mit Google automatisch verknüpft.' : 'The user will be automatically associated upon their next Google login.'}
      </p>
      <div class="input-group">
        <label class="input-label">${i18n.currentLang === 'de' ? 'E-Mail-Adresse' : 'Email Address'}</label>
        <input type="email" id="modal-new-email" class="input" placeholder="benutzer@firma.at">
      </div>
      <br>
      <div class="input-group">
        <label class="input-label">${i18n.t('role')}</label>
        <select id="modal-new-role" class="input">
          <option value="viewer">${i18n.t('role_viewer')}</option>
          <option value="admin">${i18n.t('role_admin')}</option>
        </select>
      </div>
      <div class="modal__footer">
        <button class="btn btn-ghost" id="modal-add-cancel">${i18n.t('cancel')}</button>
        <button class="btn btn-primary" id="modal-add-confirm">
          <i data-lucide="plus-circle"></i> ${i18n.currentLang === 'de' ? 'Hinzufügen' : 'Add'}
        </button>
      </div>
    `);

    document.getElementById('modal-add-cancel')?.addEventListener('click', () => Utils.closeModal());
    document.getElementById('modal-add-confirm')?.addEventListener('click', async () => {
      const email = (document.getElementById('modal-new-email') as HTMLInputElement).value.trim().toLowerCase();
      const role = (document.getElementById('modal-new-role') as HTMLSelectElement).value as 'admin' | 'viewer';

      if (!email) {
        Utils.showToast(i18n.currentLang === 'de' ? 'Bitte eine gültige E-Mail-Adresse eingeben.' : 'Please enter a valid email address.', 'warn');
        return;
      }

      try {
        await setDoc(doc(firestore, 'users', email), {
          email,
          role,
          createdAt: serverTimestamp(),
          isPlaceholder: true,
        });
        Utils.closeModal();
        Utils.showToast(i18n.currentLang === 'de' ? `Benutzer ${email} hinzugefügt.` : `User ${email} added.`, 'success');
        void this._loadUsers();
      } catch (error) {
        console.error('Failed to add user:', error);
        Utils.showToast(i18n.currentLang === 'de' ? 'Fehler beim Hinzufügen.' : 'Failed to add user.', 'error');
      }
    });
  },

  async _toggleRole(userId: string, currentRole: 'admin' | 'viewer'): Promise<void> {
    const newRole: 'admin' | 'viewer' = currentRole === 'admin' ? 'viewer' : 'admin';
    try {
      await updateDoc(doc(firestore, 'users', userId), { role: newRole });
      Utils.showToast(i18n.currentLang === 'de' ? `Rolle auf "${newRole}" geändert.` : `Role changed to "${newRole}".`, 'success');
      void this._loadUsers();
    } catch (error) {
      console.error('Failed to toggle role:', error);
      Utils.showToast(i18n.t('failed_update_role'), 'error');
    }
  },

  _confirmDelete(userId: string): void {
    const user = this._users.find((u) => u.id === userId);
    Utils.openModal(`
      <h3 class="modal__title">${i18n.currentLang === 'de' ? 'Benutzer löschen?' : 'Delete User?'}</h3>
      <p style="color: var(--text-secondary);">
        ${i18n.currentLang === 'de' 
          ? `Soll <strong>${user?.email ?? userId}</strong> aus dem System entfernt werden?` 
          : `Should <strong>${user?.email ?? userId}</strong> be removed from the system?`}
      </p>
      <div class="modal__footer">
        <button class="btn btn-ghost" id="modal-del-cancel">${i18n.t('cancel')}</button>
        <button class="btn btn-danger" id="modal-del-confirm">${i18n.t('delete')}</button>
      </div>
    `);

    document.getElementById('modal-del-cancel')?.addEventListener('click', () => Utils.closeModal());
    document.getElementById('modal-del-confirm')?.addEventListener('click', async () => {
      try {
        await deleteDoc(doc(firestore, 'users', userId));
        Utils.closeModal();
        Utils.showToast(i18n.currentLang === 'de' ? 'Benutzer entfernt.' : 'User removed.', 'success');
        void this._loadUsers();
      } catch (error) {
        console.error('Failed to delete user:', error);
        Utils.showToast(i18n.currentLang === 'de' ? 'Fehler beim Löschen.' : 'Failed to delete.', 'error');
      }
    });
  },

  destroy(): void {
    this._users = [];
  },
};
