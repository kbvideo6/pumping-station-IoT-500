import {
  ref as dbRef,
  onValue,
  update,
  type Unsubscribe,
} from 'firebase/database';
import Chart from 'chart.js/auto';
import 'chartjs-plugin-annotation'; // augments Chart.js types for annotation plugin
import { db } from './firebase';
import { AuthService } from './auth';
import { Utils, refreshIcons } from './utils';
import { AlertsList } from './alerts';
import { HistoryLogs } from './history';
import { i18n } from './i18n';
import type { LiveData, StationConfig } from './types';

export const StationDetail = {
  _stationId: null as string | null,
  _liveUnsub: null as Unsubscribe | null,
  _configUnsub: null as Unsubscribe | null,
  _chart: null as Chart | null,
  _chartLabels: [] as string[],
  _chartData: [] as number[],
  _themeChangeListener: null as ((e: Event) => void) | null,

  render(container: HTMLElement, stationId: string): void {
    this._stationId = stationId;
    this._chartLabels = [];
    this._chartData = [];

    container.innerHTML = `
      <div class="content-wrapper">
        <div class="detail-header">
          <div class="detail-header__title-block">
            <a href="#/" class="btn btn-icon" title="${i18n.t('back')}" id="btn-back">
              <i data-lucide="arrow-left"></i>
            </a>
            <div>
              <h1 id="station-name-heading">${stationId}</h1>
              <div style="font-size: 0.8rem; font-family: monospace; color: var(--text-muted);">${stationId}</div>
            </div>
            <span id="station-status-badge" class="badge badge--offline">
              <span class="led led--offline"></span> ${i18n.t('offline')}
            </span>
          </div>
          <div id="station-header-actions" style="display: flex; gap: var(--space-3);"></div>
        </div>

        <!-- Live Metric Cards -->
        <div class="detail-metrics">
          <div class="card">
            <div style="font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 6px;">${i18n.t('current_amperage')}</div>
            <div id="metric-current" style="font-size: 2rem; font-weight: 700; font-family: monospace;">-- A</div>
          </div>
          <div class="card">
            <div style="font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 6px;">${i18n.t('signal_strength')}</div>
            <div id="metric-rssi" style="font-size: 2rem; font-weight: 700; font-family: monospace;">-- dBm</div>
          </div>
          <div class="card">
            <div style="font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 6px;">${i18n.t('uptime')}</div>
            <div id="metric-uptime" style="font-size: 2rem; font-weight: 700; font-family: monospace;">--</div>
          </div>
          <div class="card">
            <div style="font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 6px;">${i18n.t('battery')}</div>
            <div id="metric-battery" style="font-size: 2rem; font-weight: 700; font-family: monospace;">--</div>
          </div>
        </div>

        <!-- Realtime Chart -->
        <div class="card">
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: var(--space-4);">
            <h3>${i18n.t('live_view')}</h3>
            <span style="font-size: 0.75rem; color: var(--text-muted);">${i18n.t('last_50_points')}</span>
          </div>
          <div class="chart-container">
            <canvas id="live-chart"></canvas>
          </div>
        </div>

        <!-- Tab Navigation -->
        <div class="tabs" id="station-tabs">
          <button class="tab-btn active" data-tab="alerts-tab">${i18n.t('tab_alerts')}</button>
          <button class="tab-btn" data-tab="history-tab">${i18n.t('tab_history')}</button>
          ${AuthService.isAdmin() ? `<button class="tab-btn" data-tab="config-tab">${i18n.t('tab_config')}</button>` : ''}
        </div>

        <div id="alerts-tab" class="tab-content active">
          <div id="alerts-tab-mount"></div>
        </div>
        <div id="history-tab" class="tab-content">
          <div id="history-tab-mount"></div>
        </div>
        ${AuthService.isAdmin() ? `
        <div id="config-tab" class="tab-content">
          <div id="config-form-mount"></div>
        </div>
        ` : ''}
      </div>
    `;

    refreshIcons();
    this._setupTabs();
    this._initChart();
    this._bindListeners();

    // Render alerts sub-view immediately
    const alertsMount = document.getElementById('alerts-tab-mount')!;
    AlertsList.render(alertsMount, stationId);
  },

  _setupTabs(): void {
    const tabBtns = document.querySelectorAll<HTMLButtonElement>('.tab-btn');
    tabBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const tabId = btn.dataset.tab!;
        tabBtns.forEach((b) => b.classList.remove('active'));
        btn.classList.add('active');
        document.querySelectorAll('.tab-content').forEach((tc) => tc.classList.remove('active'));
        document.getElementById(tabId)?.classList.add('active');

        // Lazy-render sub-views on tab click
        if (tabId === 'history-tab') {
          const mount = document.getElementById('history-tab-mount')!;
          if (!mount.hasChildNodes()) {
            HistoryLogs.render(mount, this._stationId!);
          }
        }
        if (tabId === 'config-tab' && AuthService.isAdmin()) {
          const mount = document.getElementById('config-form-mount')!;
          if (!mount.hasChildNodes()) {
            this._renderConfigForm(mount);
          }
        }
      });
    });
  },

  _initChart(): void {
    const ctx = (document.getElementById('live-chart') as HTMLCanvasElement).getContext('2d')!;

    this._chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: this._chartLabels,
        datasets: [{
          label: i18n.t('current_a'),
          data: this._chartData,
          borderColor: 'rgba(6, 182, 212, 1)',
          backgroundColor: 'rgba(6, 182, 212, 0.05)',
          borderWidth: 2,
          pointRadius: 0,
          fill: true,
          tension: 0.3,
        }],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: { duration: 300 },
        scales: {
          x: { display: false },
          y: {
            min: 0,
            grid: { color: getComputedStyle(document.documentElement).getPropertyValue('--border-subtle').trim() || 'rgba(0,0,0,0.08)' },
            ticks: { color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary').trim() || '#64748b', callback: (v) => `${v} A` },
          },
        },
        plugins: {
          legend: { display: false },
          annotation: {
            annotations: {
              highLine: {
                type: 'line' as const,
                yMin: 18,
                yMax: 18,
                borderColor: 'rgba(239, 68, 68, 0.5)',
                borderWidth: 1.5,
                borderDash: [6, 4],
                label: {
                  content: i18n.t('chart_limit'),
                  display: true,
                  position: 'end',
                  backgroundColor: 'rgba(239,68,68,0.7)',
                  color: 'white',
                  font: { size: 9, weight: 'bold' },
                },
              },
            },
          },
        },
      },
    });

    this._themeChangeListener = () => {
      if (this._chart) {
        this._chart.options.scales!.y!.grid!.color = getComputedStyle(document.documentElement).getPropertyValue('--border-subtle').trim() || 'rgba(0,0,0,0.08)';
        this._chart.options.scales!.y!.ticks!.color = getComputedStyle(document.documentElement).getPropertyValue('--text-secondary').trim() || '#64748b';
        this._chart.update();
      }
    };
    window.addEventListener('themechange', this._themeChangeListener);
  },

  _bindListeners(): void {
    // Live data listener
    this._liveUnsub = onValue(dbRef(db, `/stations/${this._stationId}/live`), (snapshot) => {
      const live = snapshot.val() as LiveData | null;
      if (!live) return;

      // Update metric cards
      (document.getElementById('metric-current') as HTMLElement).innerText =
        Utils.formatCurrent(live.current);
      (document.getElementById('metric-rssi') as HTMLElement).innerText =
        live.rssi !== undefined ? `${live.rssi} dBm` : '-- dBm';

      // Uptime
      if (live.uptimeSeconds !== undefined) {
        const h = Math.floor(live.uptimeSeconds / 3600);
        const m = Math.floor((live.uptimeSeconds % 3600) / 60);
        (document.getElementById('metric-uptime') as HTMLElement).innerText = `${h}h ${m}m`;
      }

      // Battery
      const battEl = document.getElementById('metric-battery') as HTMLElement;
      if (live.battPercent !== undefined && live.battPercent >= 0) {
        battEl.innerText = `${live.battPercent.toFixed(0)}%`;
      } else {
        battEl.innerText = '--';
      }

      // Status badge
      const badge = document.getElementById('station-status-badge') as HTMLElement;
      if (live.alert) {
        badge.className = 'badge badge--alert';
        badge.innerHTML = `<span class="led led--alert"></span> ${Utils.getAlertLabel(live.alertType ?? null)}`;
      } else {
        badge.className = 'badge badge--ok';
        badge.innerHTML = `<span class="led led--ok"></span> ${i18n.t('normal')}`;
      }

      // Append to chart
      const locale = i18n.currentLang === 'de' ? 'de-AT' : 'en-US';
      const now = new Date().toLocaleTimeString(locale, { hour: '2-digit', minute: '2-digit', second: '2-digit' });
      this._chartLabels.push(now);
      this._chartData.push(live.current ?? 0);
      if (this._chartLabels.length > 50) {
        this._chartLabels.shift();
        this._chartData.shift();
      }
      this._chart?.update('none');
    });

    // Config listener — update station name heading and chart thresholds
    this._configUnsub = onValue(dbRef(db, `/stations/${this._stationId}/config`), (snapshot) => {
      const config = snapshot.val() as StationConfig | null;
      if (!config) return;

      const heading = document.getElementById('station-name-heading') as HTMLElement;
      if (config.stationName) heading.innerText = config.stationName;

      // Update chart annotation lines
      if (this._chart && config.highThreshold) {
        const annotations = (this._chart.options.plugins as Record<string, unknown>)?.annotation as
          { annotations: { highLine?: { yMin: number; yMax: number } } } | undefined;
        if (annotations?.annotations?.highLine) {
          annotations.annotations.highLine.yMin = config.highThreshold;
          annotations.annotations.highLine.yMax = config.highThreshold;
          this._chart.update('none');
        }
      }

      // Render config form if admin tab is active
      if (AuthService.isAdmin()) {
        const mount = document.getElementById('config-form-mount');
        if (mount && mount.hasChildNodes()) {
          this._renderConfigForm(mount, config);
        }
      }
    });
  },

  _renderConfigForm(container: HTMLElement, config?: StationConfig): void {
    const cfg = config ?? {} as Partial<StationConfig>;
    container.innerHTML = `
      <div class="config-form" style="padding-top: var(--space-4);">
        <h3>${i18n.t('device_config')}</h3>
        <div class="config-form__row">
          <div class="input-group">
            <label class="input-label">${i18n.t('station_name')}</label>
            <input type="text" id="cfg-name" class="input" value="${cfg.stationName ?? ''}">
          </div>
          <div class="input-group">
            <label class="input-label">${i18n.t('pump_power')}</label>
            <input type="number" id="cfg-power" class="input input--numeric" step="0.1" value="${cfg.pumpPowerKW ?? 1.5}">
          </div>
        </div>
        <div class="config-form__row">
          <div class="input-group">
            <label class="input-label">${i18n.t('upper_threshold')}</label>
            <input type="number" id="cfg-high" class="input input--numeric" step="0.1" value="${cfg.highThreshold ?? 18}">
          </div>
          <div class="input-group">
            <label class="input-label">${i18n.t('lower_threshold')}</label>
            <input type="number" id="cfg-low" class="input input--numeric" step="0.1" value="${cfg.lowThreshold ?? 2}">
          </div>
        </div>
        <div class="config-form__row">
          <div class="input-group">
            <label class="input-label">${i18n.t('report_interval')}</label>
            <input type="number" id="cfg-report-interval" class="input input--numeric" min="10" value="${cfg.reportIntervalSec ?? 30}">
          </div>
          <div class="input-group">
            <label class="input-label">${i18n.t('config_poll_interval')}</label>
            <input type="number" id="cfg-poll-interval" class="input input--numeric" min="60" value="${cfg.configPollIntervalSec ?? 300}">
          </div>
        </div>
        <button id="btn-save-config" class="btn btn-primary">
          <i data-lucide="save"></i> ${i18n.t('btn_save_config')}
        </button>
      </div>
    `;

    refreshIcons();

    document.getElementById('btn-save-config')?.addEventListener('click', () => void this._saveConfig());
  },

  async _saveConfig(): Promise<void> {
    if (!this._stationId) return;

    const getValue = (id: string): string =>
      (document.getElementById(id) as HTMLInputElement)?.value ?? '';

    const configUpdate: Partial<StationConfig> = {
      stationName: getValue('cfg-name'),
      pumpPowerKW: parseFloat(getValue('cfg-power')),
      highThreshold: parseFloat(getValue('cfg-high')),
      lowThreshold: parseFloat(getValue('cfg-low')),
      reportIntervalSec: parseInt(getValue('cfg-report-interval'), 10),
      configPollIntervalSec: parseInt(getValue('cfg-poll-interval'), 10),
    };

    try {
      await update(dbRef(db, `/stations/${this._stationId}/config`), configUpdate);
      Utils.showToast(i18n.t('config_saved'), 'success');
    } catch (error) {
      console.error('Failed to save config:', error);
      Utils.showToast(i18n.t('failed_save_config'), 'error');
    }
  },

  destroy(): void {
    this._liveUnsub?.();
    this._liveUnsub = null;
    this._configUnsub?.();
    this._configUnsub = null;
    if (this._chart) {
      this._chart.destroy();
      this._chart = null;
    }
    if (this._themeChangeListener) {
      window.removeEventListener('themechange', this._themeChangeListener);
      this._themeChangeListener = null;
    }
    HistoryLogs.destroy();
    AlertsList.destroy();
    this._stationId = null;
  },
};
