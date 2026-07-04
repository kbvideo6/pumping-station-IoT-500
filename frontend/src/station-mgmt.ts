import {
  ref as dbRef,
  get,
  set,
  update,
  remove,
} from 'firebase/database';
import {
  doc,
  setDoc,
  deleteDoc,
  serverTimestamp,
} from 'firebase/firestore';
import { httpsCallable } from 'firebase/functions';
import { db, firestore, functions } from './firebase';
import { AuthService } from './auth';
import { Utils, refreshIcons } from './utils';
import { i18n } from './i18n';

interface StationRecord {
  id: string;
  name: string;
  online: boolean;
  lat: number;
  lng: number;
}

export const StationManagement = {
  _stations: [] as StationRecord[],

  render(container: HTMLElement): void {
    if (!AuthService.isAdmin()) {
      container.innerHTML = `<div class="content-wrapper"><div class="card" style="text-align:center; padding: 60px; color: var(--text-secondary);">${i18n.t('no_access')}</div></div>`;
      return;
    }

    container.innerHTML = `
      <div class="content-wrapper">
        <div class="detail-header">
          <h1>${i18n.t('manage_stations')}</h1>
          <button id="btn-add-station" class="btn btn-primary">
            <i data-lucide="plus-circle"></i> ${i18n.t('add_station')}
          </button>
        </div>
        <div class="table-wrapper">
          <table class="table">
            <thead>
              <tr>
                <th>${i18n.t('station_id')}</th>
                <th>${i18n.t('name')}</th>
                <th>${i18n.t('status')}</th>
                <th>${i18n.t('gps')}</th>
                <th>${i18n.t('actions')}</th>
              </tr>
            </thead>
            <tbody id="stations-mgmt-body">
              <tr><td colspan="5"><div class="loading-container"><div class="spinner"></div></div></td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;

    refreshIcons();
    void this._loadStations();
    this._setupListeners();
  },

  _setupListeners(): void {
    document.getElementById('btn-add-station')?.addEventListener('click', () => this._showAddModal());

    // Event delegation for table actions
    document.getElementById('stations-mgmt-body')?.addEventListener('click', (e) => {
      const btn = (e.target as HTMLElement).closest('[data-action]') as HTMLElement | null;
      if (!btn) return;
      const { action, id, name, lat, lng } = btn.dataset;
      if (action === 'edit' && id && name !== undefined) {
        this._showEditModal(id, name, parseFloat(lat ?? '0'), parseFloat(lng ?? '0'));
      } else if (action === 'delete' && id) {
        this._confirmDelete(id);
      } else if (action === 'provision' && id) {
        void this._provisionStation(id);
      } else if (action === 'livedata' && id) {
        void this._showLiveDataModal(id);
      }
    });
  },

  async _showLiveDataModal(stationId: string): Promise<void> {
    try {
      const snapshot = await get(dbRef(db, `/stations/${stationId}`));
      const data = snapshot.val();
      if (!data) return;

      const live = data.live || {};
      const config = data.config || {};
      
      const content = `
        <h3 class="modal__title">${i18n.currentLang === 'de' ? 'Live-Daten' : 'Live Data'} - ${config.stationName || stationId}</h3>
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 20px;">
          <div class="card" style="padding: 10px;">
            <div style="font-size: 0.8rem; color: var(--text-secondary);">${i18n.t('current_a')}</div>
            <div style="font-size: 1.2rem; font-weight: bold;">${live.current !== undefined ? live.current.toFixed(2) + ' A' : '--'}</div>
          </div>
          <div class="card" style="padding: 10px;">
            <div style="font-size: 0.8rem; color: var(--text-secondary);">Voltage</div>
            <div style="font-size: 1.2rem; font-weight: bold;">${live.voltage !== undefined ? live.voltage.toFixed(1) + ' V' : '--'}</div>
          </div>
          <div class="card" style="padding: 10px;">
            <div style="font-size: 0.8rem; color: var(--text-secondary);">Power</div>
            <div style="font-size: 1.2rem; font-weight: bold;">${live.power !== undefined ? live.power.toFixed(0) + ' W' : '--'}</div>
          </div>
          <div class="card" style="padding: 10px;">
            <div style="font-size: 0.8rem; color: var(--text-secondary);">Energy</div>
            <div style="font-size: 1.2rem; font-weight: bold;">${live.energy !== undefined ? live.energy.toFixed(2) + ' kWh' : '--'}</div>
          </div>
          <div class="card" style="padding: 10px;">
            <div style="font-size: 0.8rem; color: var(--text-secondary);">Frequency</div>
            <div style="font-size: 1.2rem; font-weight: bold;">${live.frequency !== undefined ? live.frequency.toFixed(1) + ' Hz' : '--'}</div>
          </div>
          <div class="card" style="padding: 10px;">
            <div style="font-size: 0.8rem; color: var(--text-secondary);">Power Factor</div>
            <div style="font-size: 1.2rem; font-weight: bold;">${live.powerFactor !== undefined ? live.powerFactor.toFixed(2) : '--'}</div>
          </div>
        </div>
        <div class="modal__footer">
          <button class="btn btn-primary" id="modal-close-livedata">${i18n.currentLang === 'de' ? 'Schließen' : 'Close'}</button>
        </div>
      `;
      Utils.openModal(content);
      document.getElementById('modal-close-livedata')?.addEventListener('click', () => Utils.closeModal());
    } catch (e) {
      console.error(e);
      Utils.showToast('Failed to load data', 'error');
    }
  },

  async _loadStations(): Promise<void> {
    const tbody = document.getElementById('stations-mgmt-body') as HTMLElement;
    try {
      const snapshot = await get(dbRef(db, '/stations'));
      const data = snapshot.val() as Record<string, { config?: { stationName?: string; lat?: number; lng?: number }; status?: { online?: boolean } }> | null ?? {};

      this._stations = Object.entries(data).map(([id, station]) => ({
        id,
        name: station.config?.stationName ?? id,
        online: station.status?.online ?? false,
        lat: station.config?.lat ?? 0,
        lng: station.config?.lng ?? 0,
      }));

      if (this._stations.length === 0) {
        tbody.innerHTML = `<tr><td colspan="5" style="text-align:center; padding:40px; color: var(--text-secondary);">${i18n.t('no_stations_found')}</td></tr>`;
        return;
      }

      tbody.innerHTML = this._stations.map((s) => `
        <tr>
          <td class="td--primary td--mono">${s.id}</td>
          <td>${s.name}</td>
          <td>
            ${s.online
              ? `<span class="badge badge--ok"><span class="led led--ok"></span> ${i18n.t('online')}</span>`
              : `<span class="badge badge--offline"><span class="led led--offline"></span> ${i18n.t('offline')}</span>`}
          </td>
          <td style="font-size:0.8rem; color: var(--text-muted);">
            ${(s.lat !== 0 || s.lng !== 0) ? `${s.lat.toFixed(4)}, ${s.lng.toFixed(4)}` : (i18n.currentLang === 'de' ? 'Kein GPS-Fix' : 'No GPS Fix')}
          </td>
          <td>
            <div style="display: flex; gap: 6px;">
              <button class="btn btn-secondary btn-icon" title="${i18n.currentLang === 'de' ? 'Live-Daten ansehen' : 'View Live Data'}"
                data-action="livedata" data-id="${s.id}">
                <i data-lucide="activity"></i>
              </button>
              <button class="btn btn-ghost btn-icon" title="${i18n.currentLang === 'de' ? 'Bearbeiten' : 'Edit'}"
                data-action="edit" data-id="${s.id}" data-name="${s.name}"
                data-lat="${s.lat}" data-lng="${s.lng}">
                <i data-lucide="pencil"></i>
              </button>
              <button class="btn btn-danger btn-icon" title="${i18n.currentLang === 'de' ? 'Löschen' : 'Delete'}"
                data-action="delete" data-id="${s.id}">
                <i data-lucide="trash-2"></i>
              </button>
            </div>
          </td>
        </tr>
      `).join('');

      refreshIcons();
    } catch (error) {
      console.error('Failed to load stations:', error);
      tbody.innerHTML = `<tr><td colspan="5" style="text-align:center; color:var(--status-alert); padding:30px;">${i18n.currentLang === 'de' ? 'Fehler beim Laden.' : 'Error loading.'}</td></tr>`;
    }
  },

  _showAddModal(): void {
    Utils.openModal(`
      <h3 class="modal__title">${i18n.currentLang === 'de' ? 'Neue Station hinzufügen' : 'Add New Station'}</h3>
      <div class="input-group">
        <label class="input-label">${i18n.currentLang === 'de' ? 'Station ID (z.B. STATION_002)' : 'Station ID (e.g., STATION_002)'}</label>
        <input type="text" id="modal-new-id" class="input" placeholder="STATION_002">
      </div>
      <br>
      <div class="input-group">
        <label class="input-label">${i18n.currentLang === 'de' ? 'Anzeigename' : 'Display Name'}</label>
        <input type="text" id="modal-new-name" class="input" placeholder="${i18n.currentLang === 'de' ? 'Pumpe Nördlich' : 'North Pump'}">
      </div>
      <br>
      <div class="input-group">
        <label class="input-label">${i18n.t('notification_emails') || 'Notification Emails (comma separated)'}</label>
        <input type="text" id="modal-new-emails" class="input" placeholder="admin@example.com, alert@example.com">
      </div>
      <br>
      <div style="display:flex; gap: var(--space-4);">
        <div class="input-group">
          <label class="input-label">${i18n.t('pump_power')}</label>
          <input type="number" id="modal-new-power" class="input input--numeric" step="0.1" value="1.5">
        </div>
      </div>
      <div class="modal__footer">
        <button class="btn btn-ghost" id="modal-cancel-btn">${i18n.t('cancel')}</button>
        <button class="btn btn-primary" id="modal-provision-btn">
          <i data-lucide="plus-circle"></i> ${i18n.currentLang === 'de' ? 'Provisionieren' : 'Provision'}
        </button>
      </div>
    `);

    document.getElementById('modal-cancel-btn')?.addEventListener('click', () => Utils.closeModal());
    document.getElementById('modal-provision-btn')?.addEventListener('click', () => {
      const id = (document.getElementById('modal-new-id') as HTMLInputElement).value.trim().toUpperCase();
      const name = (document.getElementById('modal-new-name') as HTMLInputElement).value.trim();
      const emails = (document.getElementById('modal-new-emails') as HTMLInputElement).value.trim();
      const power = parseFloat((document.getElementById('modal-new-power') as HTMLInputElement).value);

      if (!id || !name) {
        Utils.showToast(i18n.currentLang === 'de' ? 'ID und Name sind erforderlich.' : 'ID and Name are required.', 'warn');
        return;
      }
      Utils.closeModal();
      void this._provisionStation(id, name, power, emails);
    });
  },

  async _provisionStation(stationId: string, name?: string, pumpPowerKW?: number, notificationEmails?: string): Promise<void> {
    try {
      const stationObj = this._stations.find((s) => s.id === stationId);
      const resolvedName = (name || stationObj?.name || stationId).trim();

      const provisionDeviceFn = httpsCallable<
        { stationId: string; stationName: string; name: string; pumpPowerKW?: number; notificationEmails?: string },
        { customToken: string }
      >(functions, 'provisionDevice');

      const result = await provisionDeviceFn({
        stationId,
        stationName: resolvedName,
        name: resolvedName,
        pumpPowerKW,
        notificationEmails,
      });
      const token = result.data.customToken;

      Utils.openModal(`
        <h3 class="modal__title">${i18n.currentLang === 'de' ? `Station "${stationId}" provisioniert!` : `Station "${stationId}" provisioned!`}</h3>
        <p style="color: var(--text-secondary); font-size: 0.9rem; margin-bottom: var(--space-4);">
          ${i18n.currentLang === 'de' 
            ? 'Kopiere diesen Token und trage ihn in <code>src/config.h</code> als <code>DEFAULT_CUSTOM_TOKEN</code> ein, dann flashe die Firmware auf das ESP32-Gerät.' 
            : 'Copy this token and insert it in <code>src/config.h</code> as <code>DEFAULT_CUSTOM_TOKEN</code>, then flash the firmware to the ESP32 device.'}
        </p>
        <label class="input-label">Provisioning Token</label>
        <div class="token-box">${token}</div>
        <div class="modal__footer">
          <button class="btn btn-primary" id="modal-close-provision">${i18n.currentLang === 'de' ? 'Fertig' : 'Done'}</button>
        </div>
      `);

      document.getElementById('modal-close-provision')?.addEventListener('click', () => {
        Utils.closeModal();
        void this._loadStations();
      });

      Utils.showToast(i18n.currentLang === 'de' ? `Station ${stationId} erfolgreich angelegt!` : `Station ${stationId} successfully created!`, 'success');
    } catch (error: unknown) {
      console.error('Provisioning failed:', error);
      const errObj = error as { message?: string; details?: string };
      const errorMessage = errObj?.message || errObj?.details || (i18n.currentLang === 'de' ? 'Fehler bei der Provisionierung.' : 'Provisioning failed.');
      Utils.showToast(errorMessage, 'error');
    }
  },

  async _showEditModal(id: string, name: string, lat: number, lng: number): Promise<void> {
    // Fetch current emails if any
    let currentEmails = '';
    try {
      const snap = await get(dbRef(db, `/stations/${id}/config/notificationEmails`));
      if (snap.exists()) currentEmails = snap.val();
    } catch (e) {}

    Utils.openModal(`
      <h3 class="modal__title">${i18n.currentLang === 'de' ? `Station bearbeiten: ${id}` : `Edit Station: ${id}`}</h3>
      <div class="input-group">
        <label class="input-label">${i18n.currentLang === 'de' ? 'Anzeigename' : 'Display Name'}</label>
        <input type="text" id="modal-edit-name" class="input" value="${name}">
      </div>
      <br>
      <div class="input-group">
        <label class="input-label">${i18n.t('notification_emails') || 'Notification Emails (comma separated)'}</label>
        <input type="text" id="modal-edit-emails" class="input" value="${currentEmails}">
      </div>
      <br>
      <div style="display:flex; gap: var(--space-4);">
        <div class="input-group">
          <label class="input-label">${i18n.currentLang === 'de' ? 'Breitengrad (Lat)' : 'Latitude (Lat)'}</label>
          <input type="number" id="modal-edit-lat" class="input input--numeric" step="0.000001" value="${lat}">
        </div>
        <div class="input-group">
          <label class="input-label">${i18n.currentLang === 'de' ? 'Längengrad (Lng)' : 'Longitude (Lng)'}</label>
          <input type="number" id="modal-edit-lng" class="input input--numeric" step="0.000001" value="${lng}">
        </div>
      </div>
      <div class="modal__footer" style="justify-content: space-between;">
        <button class="btn btn-danger" id="modal-edit-regen" style="margin-right: auto;">
          <i data-lucide="refresh-cw"></i> ${i18n.currentLang === 'de' ? 'Token neu generieren' : 'Regenerate Token'}
        </button>
        <div style="display:flex; gap: 8px;">
          <button class="btn btn-ghost" id="modal-edit-cancel">${i18n.t('cancel')}</button>
          <button class="btn btn-primary" id="modal-edit-save">${i18n.t('save')}</button>
        </div>
      </div>
    `);

    document.getElementById('modal-edit-regen')?.addEventListener('click', () => {
      const warningMsg = i18n.currentLang === 'de' 
        ? 'WARNUNG: Dies macht das alte Token ungültig und das Gerät wird getrennt, bis das neue Token geflasht wird. Fortfahren?'
        : 'WARNING: This will invalidate the old token and disconnect the device until the new token is flashed. Continue?';
        
      const overlay = document.createElement('div');
      overlay.className = 'modal-backdrop';
      overlay.style.zIndex = '300';
      overlay.innerHTML = `
        <div class="modal" style="max-width: 400px; text-align: center; border: 1px solid var(--status-alert-border);">
          <i data-lucide="alert-triangle" style="width: 48px; height: 48px; color: var(--status-alert); margin-bottom: 16px;"></i>
          <h3 style="margin-bottom: 12px; color: var(--text-primary);">Regenerate Token?</h3>
          <p style="margin-bottom: 24px; color: var(--text-secondary); font-size: 0.9rem;">${warningMsg}</p>
          <div style="display: flex; gap: 12px; justify-content: center;">
            <button class="btn btn-ghost" id="confirm-cancel">${i18n.t('cancel')}</button>
            <button class="btn btn-danger" id="confirm-yes">Regenerate</button>
          </div>
        </div>
      `;
      document.body.appendChild(overlay);
      refreshIcons();

      document.getElementById('confirm-cancel')?.addEventListener('click', () => {
        document.body.removeChild(overlay);
      });

      document.getElementById('confirm-yes')?.addEventListener('click', () => {
        document.body.removeChild(overlay);
        const stationName = (document.getElementById('modal-edit-name') as HTMLInputElement).value;
        Utils.closeModal(); // Close the edit modal
        void this._provisionStation(id, stationName);
      });
    });

    document.getElementById('modal-edit-cancel')?.addEventListener('click', () => Utils.closeModal());
    document.getElementById('modal-edit-save')?.addEventListener('click', async () => {
      const newName = (document.getElementById('modal-edit-name') as HTMLInputElement).value.trim();
      const newEmails = (document.getElementById('modal-edit-emails') as HTMLInputElement).value.trim();
      const newLat = parseFloat((document.getElementById('modal-edit-lat') as HTMLInputElement).value);
      const newLng = parseFloat((document.getElementById('modal-edit-lng') as HTMLInputElement).value);

      try {
        await update(dbRef(db, `/stations/${id}/config`), {
          stationName: newName,
          notificationEmails: newEmails,
          lat: newLat,
          lng: newLng,
        });
        Utils.closeModal();
        Utils.showToast(i18n.currentLang === 'de' ? 'Station aktualisiert.' : 'Station updated.', 'success');
        void this._loadStations();
      } catch (error) {
        console.error('Edit failed:', error);
        Utils.showToast(i18n.currentLang === 'de' ? 'Fehler beim Speichern.' : 'Failed to save.', 'error');
      }
    });
  },

  _confirmDelete(id: string): void {
    Utils.openModal(`
      <h3 class="modal__title">${i18n.currentLang === 'de' ? 'Station löschen?' : 'Delete Station?'}</h3>
      <p style="color: var(--text-secondary);">
        ${i18n.currentLang === 'de' 
          ? `Soll Station <strong>${id}</strong> und alle zugehörigen Daten unwiderruflich gelöscht werden?` 
          : `Are you sure you want to permanently delete station <strong>${id}</strong> and all of its associated data?`}
      </p>
      <div class="modal__footer">
        <button class="btn btn-ghost" id="modal-delete-cancel">${i18n.t('cancel')}</button>
        <button class="btn btn-danger" id="modal-delete-confirm">${i18n.currentLang === 'de' ? 'Endgültig löschen' : 'Permanently Delete'}</button>
      </div>
    `);

    document.getElementById('modal-delete-cancel')?.addEventListener('click', () => Utils.closeModal());
    document.getElementById('modal-delete-confirm')?.addEventListener('click', async () => {
      try {
        await remove(dbRef(db, `/stations/${id}`));
        await deleteDoc(doc(firestore, 'devices', id));
        // Remove from Firestore station metadata if present
        await setDoc(doc(firestore, 'deletedStations', id), {
          deletedAt: serverTimestamp(),
          stationId: id,
        });
        Utils.closeModal();
        Utils.showToast(i18n.currentLang === 'de' ? `Station ${id} gelöscht.` : `Station ${id} deleted.`, 'success');
        void this._loadStations();
      } catch (error) {
        console.error('Delete failed:', error);
        Utils.showToast(i18n.currentLang === 'de' ? 'Fehler beim Löschen.' : 'Failed to delete.', 'error');
      }
    });
  },

  destroy(): void {
    this._stations = [];
  },
};
