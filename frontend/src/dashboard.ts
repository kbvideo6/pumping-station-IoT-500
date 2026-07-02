import { ref, onValue, type Unsubscribe } from 'firebase/database';
import { db } from './firebase';
import { Utils, refreshIcons } from './utils';
import { i18n } from './i18n';
import type { Station, StationsSnapshot, StatusFilter } from './types';

export const Dashboard = {
  _unsubscribe: null as Unsubscribe | null,

  render(container: HTMLElement): void {
    container.innerHTML = `
      <div class="content-wrapper">
        <div class="detail-header">
          <h1>${i18n.t('pumping_stations_overview')}</h1>
        </div>

        <!-- Summary Stat Cards -->
        <div class="summary-cards">
          <div class="card summary-card">
            <div class="summary-card__info">
              <span id="stat-total" class="summary-card__value">--</span>
              <span class="summary-card__label">${i18n.t('total_stations')}</span>
            </div>
            <div class="summary-card__icon"><i data-lucide="cpu"></i></div>
          </div>
          <div class="card summary-card">
            <div class="summary-card__info">
              <span id="stat-online" class="summary-card__value" style="color: var(--status-ok)">--</span>
              <span class="summary-card__label">${i18n.t('online')}</span>
            </div>
            <div class="summary-card__icon" style="color: var(--status-ok); background-color: var(--status-ok-dim);"><i data-lucide="wifi"></i></div>
          </div>
          <div class="card summary-card">
            <div class="summary-card__info">
              <span id="stat-alerts" class="summary-card__value">--</span>
              <span class="summary-card__label">${i18n.t('active_alerts')}</span>
            </div>
            <div id="stat-alerts-icon" class="summary-card__icon"><i data-lucide="bell"></i></div>
          </div>
        </div>

        <!-- Filter and Search -->
        <div class="card filter-row" style="padding: var(--space-4); display: flex; gap: var(--space-4); align-items: center;">
          <div class="input-group" style="flex: 1; min-width: 200px;">
            <input type="text" id="search-station" class="input" placeholder="${i18n.t('search_placeholder')}">
          </div>
          <div class="input-group" style="width: 180px;">
            <select id="filter-status" class="input">
              <option value="ALL">${i18n.t('all_statuses')}</option>
              <option value="ONLINE">${i18n.t('online')}</option>
              <option value="OFFLINE">${i18n.t('offline')}</option>
              <option value="ALERT">${i18n.t('alert')}</option>
            </select>
          </div>
        </div>

        <div id="stations-grid" class="station-grid">
          <div style="grid-column: 1/-1;" class="loading-container">
            <div class="spinner spinner--lg"></div>
          </div>
        </div>
      </div>
    `;

    refreshIcons();
    this._setupListeners();
  },

  _setupListeners(): void {
    const grid = document.getElementById('stations-grid')!;
    const searchInput = document.getElementById('search-station') as HTMLInputElement;
    const filterSelect = document.getElementById('filter-status') as HTMLSelectElement;

    let allStationsData: StationsSnapshot = {};

    const filterAndRender = (): void => {
      const searchVal = searchInput.value.toLowerCase().trim();
      const filterVal = filterSelect.value as StatusFilter;

      let html = '';
      let renderedCount = 0;

      for (const [id, station] of Object.entries(allStationsData)) {
        const config = station.config ?? {} as Station['config'] & object;
        const live = station.live ?? {} as Station['live'] & object;
        const status = station.status ?? {} as Station['status'] & object;

        const name = (config as { stationName?: string }).stationName ?? id;
        const online = (status as { online?: boolean }).online ?? false;
        const current = (live as { current?: number }).current ?? 0.0;
        const hasAlert = (live as { alert?: boolean }).alert ?? false;

        const matchesSearch =
          name.toLowerCase().includes(searchVal) ||
          id.toLowerCase().includes(searchVal);

        let matchesStatus = true;
        if (filterVal === 'ONLINE') matchesStatus = online;
        else if (filterVal === 'OFFLINE') matchesStatus = !online;
        else if (filterVal === 'ALERT') matchesStatus = hasAlert;

        if (!matchesSearch || !matchesStatus) continue;

        let modifier = 'card--offline';
        let statusBadge = `<span class="badge badge--offline"><i class="led led--offline"></i> ${i18n.t('offline')}</span>`;

        if (online) {
          if (hasAlert) {
            modifier = 'card--alert';
            statusBadge = `<span class="badge badge--alert"><i class="led led--alert"></i> ${Utils.getAlertLabel((live as { alertType?: string }).alertType)}</span>`;
          } else {
            modifier = 'card--ok';
            statusBadge = `<span class="badge badge--ok"><i class="led led--ok"></i> ${i18n.t('normal')}</span>`;
          }
        }

        const highThreshold = (config as { highThreshold?: number }).highThreshold ?? 18.0;
        let percentFill = (current / highThreshold) * 100;
        if (percentFill > 100) percentFill = 100;

        let fillModifier = '';
        if (hasAlert) fillModifier = 'current-bar__fill--alert';
        else if (percentFill > 80) fillModifier = 'current-bar__fill--warn';

        const liveData = live as { rssi?: number; battPercent?: number; battVolts?: number; timestamp?: number };

        html += `
          <div class="card station-card ${modifier}" data-station-id="${id}">
            <div class="station-card__header">
              <div class="station-card__title">
                <span class="station-card__name">${name}</span>
                <span class="station-card__id">${id}</span>
              </div>
              ${statusBadge}
            </div>
            <div class="station-card__value">${online ? current.toFixed(2) + ' A' : '-- A'}</div>
            <div class="current-bar">
              <div class="current-bar__fill ${fillModifier}" style="width: ${online ? percentFill : 0}%"></div>
            </div>
            <div class="station-card__footer">
              <div class="station-card__footer-item">
                <i data-lucide="signal" style="width: 14px; height: 14px;"></i>
                <span>${online ? (liveData.rssi ?? '--') + ' dBm' : '--'}</span>
              </div>
              ${online && liveData.battPercent !== undefined ? `
              <div class="station-card__footer-item" title="Batteriespannung: ${liveData.battVolts ? liveData.battVolts.toFixed(2) + ' V' : '--'}">
                <i data-lucide="battery" style="width: 14px; height: 14px;"></i>
                <span>${liveData.battPercent.toFixed(0)}%</span>
              </div>
              ` : ''}
              <div class="station-card__footer-item">
                <i data-lucide="clock" style="width: 14px; height: 14px;"></i>
                <span>${Utils.formatRelativeTime(liveData.timestamp)}</span>
              </div>
            </div>
          </div>
        `;
        renderedCount++;
      }

      if (renderedCount === 0) {
        html = `
          <div style="grid-column: 1/-1; text-align: center; padding: 60px; color: var(--text-secondary);">
            ${i18n.t('no_stations_found')}
          </div>
        `;
      }

      grid.innerHTML = html;
      refreshIcons();
    };

    // Event delegation for station card clicks
    grid.addEventListener('click', (e) => {
      const card = (e.target as HTMLElement).closest('[data-station-id]') as HTMLElement | null;
      if (card?.dataset.stationId) {
        window.location.hash = `#/station/${card.dataset.stationId}`;
      }
    });

    searchInput.addEventListener('input', filterAndRender);
    filterSelect.addEventListener('change', filterAndRender);

    // Bind real-time RTDB sync
    this._unsubscribe = onValue(ref(db, '/stations'), (snapshot) => {
      const data: StationsSnapshot = snapshot.val() ?? {};
      allStationsData = data;

      let total = 0, online = 0, alerts = 0;
      for (const station of Object.values(data)) {
        total++;
        if (station.status?.online) online++;
        if (station.live?.alert) alerts++;
      }

      (document.getElementById('stat-total') as HTMLElement).innerText = String(total);
      (document.getElementById('stat-online') as HTMLElement).innerText = String(online);
      (document.getElementById('stat-alerts') as HTMLElement).innerText = String(alerts);

      const alertsIcon = document.getElementById('stat-alerts-icon') as HTMLElement;
      const alertsStat = document.getElementById('stat-alerts') as HTMLElement;
      if (alerts > 0) {
        alertsIcon.style.color = 'var(--status-alert)';
        alertsIcon.style.backgroundColor = 'var(--status-alert-dim)';
        alertsStat.style.color = 'var(--status-alert)';
      } else {
        alertsIcon.style.color = 'var(--text-secondary)';
        alertsIcon.style.backgroundColor = 'var(--status-offline-dim)';
        alertsStat.style.color = 'var(--text-primary)';
      }

      filterAndRender();
    });
  },

  destroy(): void {
    this._unsubscribe?.();
    this._unsubscribe = null;
  },
};
