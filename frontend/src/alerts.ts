import {
  collection,
  query,
  where,
  orderBy,
  limit,
  onSnapshot,
  doc,
  updateDoc,
  serverTimestamp,
  type Timestamp,
} from 'firebase/firestore';
import type { Unsubscribe } from 'firebase/firestore';
import { firestore } from './firebase';
import { AuthService } from './auth';
import { Utils, refreshIcons } from './utils';
import { i18n } from './i18n';
import type { AlertDocumentWithId, AlertFilterAck, AlertFilterType } from './types';

export const AlertsList = {
  _stationId: null as string | null,
  _container: null as HTMLElement | null,
  _filterAck: 'ALL' as AlertFilterAck,
  _filterType: 'ALL' as AlertFilterType,
  _alerts: [] as AlertDocumentWithId[],
  _unsubscribe: null as Unsubscribe | null,

  render(container: HTMLElement, stationId: string | null = null): void {
    this._container = container;
    this._stationId = stationId;

    const title = stationId ? i18n.t('alert_history_station') : i18n.t('all_system_alerts');

    container.innerHTML = `
      <div class="content-wrapper">
        <div class="detail-header">
          <h1>${title}</h1>
        </div>
        <div class="card filter-row" style="padding: var(--space-4); display: flex; gap: var(--space-4); flex-wrap: wrap;">
          <div class="input-group" style="flex: 1; min-width: 150px;">
            <label class="input-label">${i18n.t('status')}</label>
            <div class="custom-select">
              <div class="custom-select__trigger">
                <span>${this._filterAck === 'UNACK' ? i18n.t('unack') : this._filterAck === 'ACK' ? i18n.t('ack') : i18n.t('all')}</span>
                <i data-lucide="chevron-down" style="width: 16px; height: 16px; color: var(--text-secondary);"></i>
              </div>
              <div class="custom-select__options">
                <div class="custom-select__option ${this._filterAck === 'ALL' ? 'selected' : ''}" data-value="ALL">${i18n.t('all')}</div>
                <div class="custom-select__option ${this._filterAck === 'UNACK' ? 'selected' : ''}" data-value="UNACK">${i18n.t('unack')}</div>
                <div class="custom-select__option ${this._filterAck === 'ACK' ? 'selected' : ''}" data-value="ACK">${i18n.t('ack')}</div>
              </div>
              <select id="alert-filter-ack" style="display: none;">
                <option value="ALL" ${this._filterAck === 'ALL' ? 'selected' : ''}>${i18n.t('all')}</option>
                <option value="UNACK" ${this._filterAck === 'UNACK' ? 'selected' : ''}>${i18n.t('unack')}</option>
                <option value="ACK" ${this._filterAck === 'ACK' ? 'selected' : ''}>${i18n.t('ack')}</option>
              </select>
            </div>
          </div>
          <div class="input-group" style="flex: 1; min-width: 150px;">
            <label class="input-label">${i18n.t('type')}</label>
            <div class="custom-select">
              <div class="custom-select__trigger">
                <span>${this._filterType === 'HIGH_CURRENT' ? i18n.t('alert_HIGH_CURRENT') : this._filterType === 'LOW_CURRENT' ? i18n.t('alert_LOW_CURRENT') : this._filterType === 'DEVICE_OFFLINE' ? i18n.t('offline') : i18n.t('all_types')}</span>
                <i data-lucide="chevron-down" style="width: 16px; height: 16px; color: var(--text-secondary);"></i>
              </div>
              <div class="custom-select__options">
                <div class="custom-select__option ${this._filterType === 'ALL' ? 'selected' : ''}" data-value="ALL">${i18n.t('all_types')}</div>
                <div class="custom-select__option ${this._filterType === 'HIGH_CURRENT' ? 'selected' : ''}" data-value="HIGH_CURRENT">${i18n.t('alert_HIGH_CURRENT')}</div>
                <div class="custom-select__option ${this._filterType === 'LOW_CURRENT' ? 'selected' : ''}" data-value="LOW_CURRENT">${i18n.t('alert_LOW_CURRENT')}</div>
                <div class="custom-select__option ${this._filterType === 'DEVICE_OFFLINE' ? 'selected' : ''}" data-value="DEVICE_OFFLINE">${i18n.t('offline')}</div>
              </div>
              <select id="alert-filter-type" style="display: none;">
                <option value="ALL" ${this._filterType === 'ALL' ? 'selected' : ''}>${i18n.t('all_types')}</option>
                <option value="HIGH_CURRENT" ${this._filterType === 'HIGH_CURRENT' ? 'selected' : ''}>${i18n.t('alert_HIGH_CURRENT')}</option>
                <option value="LOW_CURRENT" ${this._filterType === 'LOW_CURRENT' ? 'selected' : ''}>${i18n.t('alert_LOW_CURRENT')}</option>
                <option value="DEVICE_OFFLINE" ${this._filterType === 'DEVICE_OFFLINE' ? 'selected' : ''}>${i18n.t('offline')}</option>
              </select>
            </div>
          </div>
        </div>
        <div class="table-wrapper">
          <table class="table">
            <thead>
              <tr>
                <th>${i18n.t('col_timestamp')}</th>
                ${!stationId ? `<th>${i18n.t('col_station')}</th>` : ''}
                <th>${i18n.t('col_type')}</th>
                <th>${i18n.t('col_value')}</th>
                <th>${i18n.t('col_threshold')}</th>
                <th>${i18n.t('col_status')}</th>
                ${AuthService.isAdmin() ? `<th>${i18n.t('col_action')}</th>` : ''}
              </tr>
            </thead>
            <tbody id="alerts-table-body">
              <tr><td colspan="7"><div class="loading-container"><div class="spinner"></div></div></td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;

    refreshIcons();
    this._setupListeners();
    Utils.initCustomSelects();

    // Automatically load alerts on render
    void this.loadAlerts();
  },

  _setupListeners(): void {
    // Watch hidden selects for change event dispatched by custom-select
    document.getElementById('alert-filter-ack')?.addEventListener('change', () => {
      this._filterAck = (document.getElementById('alert-filter-ack') as HTMLSelectElement).value as AlertFilterAck;
      void this.loadAlerts();
    });

    document.getElementById('alert-filter-type')?.addEventListener('change', () => {
      this._filterType = (document.getElementById('alert-filter-type') as HTMLSelectElement).value as AlertFilterType;
      void this.loadAlerts();
    });

    // Event delegation for acknowledge buttons
    document.getElementById('alerts-table-body')?.addEventListener('click', (e) => {
      const btn = (e.target as HTMLElement).closest('[data-action="acknowledge"]') as HTMLElement | null;
      if (btn?.dataset.id) {
        void this.acknowledge(btn.dataset.id);
      }
    });
  },

  async loadAlerts(): Promise<void> {
    if (this._unsubscribe) {
      this._unsubscribe();
      this._unsubscribe = null;
    }

    try {
      const alertsRef = collection(firestore, 'alerts');
      const conditions = [];

      if (this._stationId) {
        conditions.push(where('stationId', '==', this._stationId));
      }
      if (this._filterAck === 'ACK') {
        conditions.push(where('acknowledged', '==', true));
      } else if (this._filterAck === 'UNACK') {
        conditions.push(where('acknowledged', '==', false));
      }
      if (this._filterType !== 'ALL') {
        conditions.push(where('type', '==', this._filterType));
      }

      conditions.push(orderBy('timestamp', 'desc'));

      const q = query(alertsRef, ...conditions, limit(100));
      
      this._unsubscribe = onSnapshot(q, (snapshot) => {
        this._alerts = [];
        snapshot.forEach((d) => {
          this._alerts.push({ id: d.id, ...(d.data() as Omit<AlertDocumentWithId, 'id'>) });
        });

        // Sort by timestamp descending in memory
        this._alerts.sort((a, b) => {
          const tA = (a.timestamp as Timestamp)?.toMillis() || 0;
          const tB = (b.timestamp as Timestamp)?.toMillis() || 0;
          return tB - tA;
        });

        this._renderTable();
      }, (error) => {
        console.error('Error listening to alerts:', error);
        const tbody = document.getElementById('alerts-table-body');
        if (tbody) {
          tbody.innerHTML = `<tr><td colspan="7" style="text-align:center; color:var(--status-alert); padding:40px;">${i18n.t('alerts_load_error')}</td></tr>`;
        }
      });

    } catch (error) {
      console.error('Error setting up alerts listener:', error);
      const tbody = document.getElementById('alerts-table-body');
      if (tbody) {
        tbody.innerHTML = `<tr><td colspan="7" style="text-align:center; color:var(--status-alert); padding:40px;">${i18n.t('alerts_load_error')}</td></tr>`;
      }
    }
  },

  loadCachedAlerts(): void {
    // If we're showing UNACK alerts, we can trigger a reactive reload to reflect changes instantly
    if (this._container && document.getElementById('alerts-table-body')) {
      void this.loadAlerts();
    }
  },

  _renderTable(): void {
    const tbody = document.getElementById('alerts-table-body') as HTMLElement;
    if (!tbody) return;

    if (this._alerts.length === 0) {
      tbody.innerHTML = `<tr><td colspan="7" style="text-align:center; padding:40px; color:var(--text-secondary);">${i18n.t('no_alerts_found')}</td></tr>`;
      return;
    }

    tbody.innerHTML = this._alerts.map((alert) => {
      const ts = alert.timestamp as Timestamp | null;
      const dateStr = Utils.formatTimestamp(ts ? ts.toDate() : null);
      const typeLabel = Utils.getAlertLabel(alert.type);

      const statusBadge = alert.acknowledged
        ? `<span class="badge badge--ok"><i data-lucide="check-check" style="width:10px;height:10px;"></i> ${i18n.t('ack')}</span>`
        : `<span class="badge badge--alert"><i data-lucide="alert-triangle" style="width:10px;height:10px;"></i> ${i18n.t('open_alert')}</span>`;

      const actionCell = AuthService.isAdmin()
        ? `<td data-label="${i18n.t('col_action')}">${!alert.acknowledged
            ? `<button class="btn btn-ghost" style="font-size:0.8rem; padding: 4px 10px;" data-action="acknowledge" data-id="${alert.id}">
                <i data-lucide="check"></i> ${i18n.t('btn_acknowledge')}
               </button>`
            : '<span style="font-size:0.75rem; color: var(--text-muted);">—</span>'
          }</td>`
        : '';

      return `
        <tr>
          <td class="td--mono" data-label="${i18n.t('col_timestamp')}" style="font-size:0.8rem;">${dateStr}</td>
          ${!this._stationId ? `<td class="td--primary" data-label="${i18n.t('col_station')}">${alert.stationName ?? alert.stationId}</td>` : ''}
          <td data-label="${i18n.t('col_type')}">${typeLabel}</td>
          <td data-label="${i18n.t('col_value')}">${alert.currentValue !== null ? alert.currentValue.toFixed(2) + ' A' : '--'}</td>
          <td data-label="${i18n.t('col_threshold')}">${alert.threshold !== null ? alert.threshold.toFixed(1) + ' A' : '--'}</td>
          <td data-label="${i18n.t('col_status')}">${statusBadge}</td>
          ${actionCell}
        </tr>
      `;
    }).join('');

    refreshIcons();
  },

  async acknowledge(alertId: string): Promise<void> {
    if (!AuthService.currentUser) return;
    try {
      await updateDoc(doc(firestore, 'alerts', alertId), {
        acknowledged: true,
        acknowledgedBy: AuthService.currentUser.email,
        acknowledgedAt: serverTimestamp(),
      });
      Utils.showToast(i18n.t('alert_acknowledged'), 'success');
      // Table will reload automatically via the active query subscription!
    } catch (error) {
      console.error('Failed to acknowledge alert:', error);
      Utils.showToast(i18n.t('failed_acknowledge'), 'error');
    }
  },

  destroy(): void {
    if (this._unsubscribe) {
      this._unsubscribe();
      this._unsubscribe = null;
    }
    this._stationId = null;
    this._container = null;
    this._alerts = [];
  },
};
