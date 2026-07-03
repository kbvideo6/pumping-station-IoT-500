import {
  collection,
  query,
  where,
  orderBy,
  getDocs,
  type Timestamp,
} from 'firebase/firestore';
import Chart from 'chart.js/auto';
import { firestore } from './firebase';
import { Utils, refreshIcons } from './utils';
import { i18n } from './i18n';

interface HistoryRecord {
  id: string;
  timestamp: Timestamp | null;
  current: number;
  voltage: number | null;
  power: number | null;
  energy: number | null;
  frequency: number | null;
  powerFactor: number | null;
  alert: boolean;
  alertType: string | null;
  rssi: number | null;
}

export const HistoryLogs = {
  _records: [] as HistoryRecord[],
  _stationId: null as string | null,
  _historyChart: null as Chart | null,
  _themeChangeListener: null as ((e: Event) => void) | null,

  render(container: HTMLElement, stationId: string): void {
    this._stationId = stationId;

    container.innerHTML = `
      <div style="display: flex; flex-direction: column; gap: var(--space-6); padding-top: var(--space-4);">
        <div class="card">
          <h3 style="margin-bottom: var(--space-4);">${i18n.t('query_history')}</h3>
          <div class="config-form__row" style="flex-wrap: wrap; gap: var(--space-4); align-items: flex-end;">
            <div class="input-group" style="width: 180px;">
              <label class="input-label">${i18n.t('from')}</label>
              <input type="date" id="hist-date-from" class="input">
            </div>
            <div class="input-group" style="width: 180px;">
              <label class="input-label">${i18n.t('to')}</label>
              <input type="date" id="hist-date-to" class="input">
            </div>
            <button id="btn-load-history" class="btn btn-primary">
              <i data-lucide="refresh-cw"></i> ${i18n.t('load_data')}
            </button>
            <button id="btn-export-csv" class="btn btn-ghost" disabled>
              <i data-lucide="download"></i> ${i18n.t('export_csv')}
            </button>
          </div>
        </div>
        <div class="card">
          <h3 style="margin-bottom: var(--space-4);">${i18n.t('history_chart')}</h3>
          <div class="chart-container">
            <canvas id="history-chart"></canvas>
          </div>
        </div>
        <div class="table-wrapper">
          <table class="table">
            <thead>
              <tr>
                <th>${i18n.t('col_timestamp')}</th>
                <th>${i18n.t('current_a')}</th>
                <th>${i18n.t('col_voltage')}</th>
                <th>${i18n.t('col_power')}</th>
                <th>${i18n.t('col_energy')}</th>
                <th>${i18n.t('col_pf')}</th>
                <th>${i18n.t('alert')}</th>
                <th>RSSI</th>
              </tr>
            </thead>
            <tbody id="history-table-body">
              <tr><td colspan="8" style="text-align:center; padding:30px; color:var(--text-secondary);">
                ${i18n.t('select_range_prompt')}
              </td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;

    // Default date range: last 7 days
    const today = new Date();
    const weekAgo = new Date(today.getTime() - 7 * 24 * 60 * 60 * 1000);
    (document.getElementById('hist-date-to') as HTMLInputElement).value = today.toISOString().split('T')[0];
    (document.getElementById('hist-date-from') as HTMLInputElement).value = weekAgo.toISOString().split('T')[0];

    refreshIcons();
    this._setupListeners();
  },

  _setupListeners(): void {
    document.getElementById('btn-load-history')?.addEventListener('click', () => void this._loadHistory());
    document.getElementById('btn-export-csv')?.addEventListener('click', () => this._exportCSV());
  },

  async _loadHistory(): Promise<void> {
    if (!this._stationId) return;

    const fromStr = (document.getElementById('hist-date-from') as HTMLInputElement).value;
    const toStr = (document.getElementById('hist-date-to') as HTMLInputElement).value;

    if (!fromStr || !toStr) {
      Utils.showToast(i18n.t('select_range_warn'), 'warn');
      return;
    }

    const fromDate = new Date(fromStr);
    const toDate = new Date(toStr);
    toDate.setHours(23, 59, 59, 999);

    const tbody = document.getElementById('history-table-body') as HTMLElement;
    tbody.innerHTML = `<tr><td colspan="8"><div class="loading-container"><div class="spinner"></div></div></td></tr>`;

    try {
      const q = query(
        collection(firestore, 'history'),
        where('stationId', '==', this._stationId),
        where('timestamp', '>=', fromDate),
        where('timestamp', '<=', toDate),
        orderBy('timestamp', 'asc'),
      );

      const snapshot = await getDocs(q);
      this._records = [];

      snapshot.forEach((d) => {
        const data = d.data();
        this._records.push({
          id: d.id,
          timestamp:   data.timestamp as Timestamp | null,
          current:     data.current     as number  ?? 0,
          voltage:     data.voltage     as number  | null ?? null,
          power:       data.power       as number  | null ?? null,
          energy:      data.energy      as number  | null ?? null,
          frequency:   data.frequency   as number  | null ?? null,
          powerFactor: data.powerFactor as number  | null ?? null,
          alert:       data.alert       as boolean ?? false,
          alertType:   data.alertType   as string  | null ?? null,
          rssi:        data.rssi        as number  | null ?? null,
        });
      });

      this._renderChart();
      this._renderTable();

      const exportBtn = document.getElementById('btn-export-csv') as HTMLButtonElement;
      exportBtn.disabled = this._records.length === 0;

      if (this._records.length === 0) {
        Utils.showToast(i18n.t('no_data_range'), 'info');
      }
    } catch (error) {
      console.error('Error loading history:', error);
      tbody.innerHTML = `<tr><td colspan="8" style="text-align:center; color:var(--status-alert); padding:30px;">${i18n.t('history_load_error')}</td></tr>`;
    }
  },

  _renderChart(): void {
    const ctx = (document.getElementById('history-chart') as HTMLCanvasElement).getContext('2d')!;

    if (this._historyChart) {
      this._historyChart.destroy();
      this._historyChart = null;
    }

    const labels  = this._records.map((r) => {
      const ts = r.timestamp as Timestamp | null;
      return ts ? Utils.formatTimestamp(ts.toDate()) : '';
    });
    const currentValues = this._records.map((r) => r.current);
    const voltageValues = this._records.map((r) => r.voltage ?? 0);

    this._historyChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels,
        datasets: [
          {
            label: i18n.t('current_a'),
            data: currentValues,
            yAxisID: 'yA',
            borderColor: 'rgba(6, 182, 212, 1)',
            backgroundColor: 'rgba(6, 182, 212, 0.05)',
            borderWidth: 1.5,
            pointRadius: 0,
            fill: true,
            tension: 0.3,
          },
          {
            label: i18n.t('voltage_v'),
            data: voltageValues,
            yAxisID: 'yV',
            borderColor: 'rgba(251, 191, 36, 1)',
            backgroundColor: 'rgba(251, 191, 36, 0.04)',
            borderWidth: 1.5,
            pointRadius: 0,
            fill: false,
            tension: 0.3,
          },
        ],
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          x: { display: false },
          yA: {
            type: 'linear',
            position: 'left',
            grid: { color: getComputedStyle(document.documentElement).getPropertyValue('--border-subtle').trim() || 'rgba(0,0,0,0.08)' },
            ticks: { color: getComputedStyle(document.documentElement).getPropertyValue('--text-secondary').trim() || '#64748b', callback: (v) => `${v} A` },
          },
          yV: {
            type: 'linear',
            position: 'right',
            min: 180,
            max: 260,
            grid: { drawOnChartArea: false },
            ticks: { color: 'rgba(251, 191, 36, 0.8)', callback: (v) => `${v} V` },
          },
        },
        plugins: {
          legend: { display: false },
        },
      },
    });

    this._themeChangeListener = () => {
      if (this._historyChart) {
        this._historyChart.options.scales!.yA!.grid!.color = getComputedStyle(document.documentElement).getPropertyValue('--border-subtle').trim() || 'rgba(0,0,0,0.08)';
        this._historyChart.options.scales!.yA!.ticks!.color = getComputedStyle(document.documentElement).getPropertyValue('--text-secondary').trim() || '#64748b';
        this._historyChart.update();
      }
    };
    window.addEventListener('themechange', this._themeChangeListener);
  },

  _renderTable(): void {
    const tbody = document.getElementById('history-table-body') as HTMLElement;

    if (this._records.length === 0) {
      tbody.innerHTML = `<tr><td colspan="8" style="text-align:center; padding:30px; color:var(--text-secondary);">${i18n.t('no_data_range')}</td></tr>`;
      return;
    }

    tbody.innerHTML = [...this._records].reverse().map((r) => {
      const ts = r.timestamp as Timestamp | null;
      const dateStr = ts ? Utils.formatTimestamp(ts.toDate()) : '--';
      const alertCell = r.alert
        ? `<span class="badge badge--alert">${Utils.getAlertLabel(r.alertType)}</span>`
        : `<span class="badge badge--ok">${i18n.t('normal')}</span>`;

      const pfClass = Utils.powerFactorClass(r.powerFactor);

      return `
        <tr>
          <td class="td--mono" style="font-size:0.8rem;">${dateStr}</td>
          <td class="td--primary td--mono">${r.current.toFixed(2)}</td>
          <td class="td--mono">${r.voltage !== null ? r.voltage.toFixed(1) : '--'}</td>
          <td class="td--mono">${r.power !== null ? (r.power >= 1000 ? (r.power/1000).toFixed(2)+' k' : r.power.toFixed(0)) : '--'}</td>
          <td class="td--mono">${r.energy !== null ? r.energy.toFixed(3) : '--'}</td>
          <td class="td--mono ${pfClass}">${r.powerFactor !== null ? r.powerFactor.toFixed(2) : '--'}</td>
          <td>${alertCell}</td>
          <td>${r.rssi !== null ? r.rssi + ' dBm' : '--'}</td>
        </tr>
      `;
    }).join('');
  },

  _exportCSV(): void {
    if (this._records.length === 0) return;

    const header = i18n.currentLang === 'de'
      ? 'Zeitpunkt,Stromstärke (A),Spannung (V),Leistung (W),Energie (kWh),Leistungsfaktor,Alarm,Alarmtyp,RSSI (dBm)\n'
      : 'Timestamp,Current (A),Voltage (V),Power (W),Energy (kWh),Power Factor,Alert,AlertType,RSSI (dBm)\n';

    const rows = [...this._records].reverse().map((r) => {
      const ts = r.timestamp as Timestamp | null;
      const dateStr = ts ? ts.toDate().toISOString() : '';
      return `${dateStr},${r.current.toFixed(3)},${r.voltage ?? ''},${r.power ?? ''},${r.energy ?? ''},${r.powerFactor ?? ''},${r.alert},${r.alertType ?? ''},${r.rssi ?? ''}`;
    }).join('\n');

    const blob = new Blob([header + rows], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `verlauf_${this._stationId ?? 'station'}_${new Date().toISOString().split('T')[0]}.csv`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);

    Utils.showToast(
      i18n.currentLang === 'de'
        ? 'CSV-Datei erfolgreich heruntergeladen.'
        : 'CSV file downloaded successfully.',
      'success'
    );
  },

  destroy(): void {
    if (this._historyChart) {
      this._historyChart.destroy();
      this._historyChart = null;
    }
    if (this._themeChangeListener) {
      window.removeEventListener('themechange', this._themeChangeListener);
      this._themeChangeListener = null;
    }
    this._records = [];
    this._stationId = null;
  },
};
