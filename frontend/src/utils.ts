import { createIcons, icons } from 'lucide';
import type { ToastType } from './types';
import { i18n } from './i18n';

// Re-exported so every module calls a single helper instead of
// importing lucide directly.
export function refreshIcons(): void {
  createIcons({ icons });
}

export const Utils = {
  showToast(message: string, type: ToastType = 'info'): void {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast toast--${type}`;

    const iconMap: Record<ToastType, string> = {
      success: 'check-circle',
      warn: 'alert-triangle',
      error: 'alert-circle',
      info: 'info',
    };

    toast.innerHTML = `
      <i data-lucide="${iconMap[type]}"></i>
      <div class="toast__message">${message}</div>
    `;

    container.appendChild(toast);
    refreshIcons();

    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateY(10px)';
      toast.style.transition = 'opacity 300ms ease, transform 300ms ease';
      setTimeout(() => toast.remove(), 300);
    }, 4000);
  },

  formatCurrent(current: number | undefined | null): string {
    if (current === undefined || current === null) return '-- A';
    return `${parseFloat(String(current)).toFixed(2)} A`;
  },

  formatVoltage(v: number | undefined | null): string {
    if (v === undefined || v === null) return '-- V';
    return `${parseFloat(String(v)).toFixed(1)} V`;
  },

  formatPower(w: number | undefined | null): string {
    if (w === undefined || w === null) return '-- W';
    const watts = parseFloat(String(w));
    return watts >= 1000 ? `${(watts / 1000).toFixed(2)} kW` : `${watts.toFixed(0)} W`;
  },

  formatEnergy(kwh: number | undefined | null): string {
    if (kwh === undefined || kwh === null) return '-- kWh';
    return `${parseFloat(String(kwh)).toFixed(3)} kWh`;
  },

  formatPowerFactor(pf: number | undefined | null): string {
    if (pf === undefined || pf === null) return '--';
    return parseFloat(String(pf)).toFixed(2);
  },

  powerFactorClass(pf: number | undefined | null): string {
    if (pf === undefined || pf === null) return '';
    if (pf >= 0.85) return 'pf--good';
    if (pf >= 0.70) return 'pf--warn';
    return 'pf--poor';
  },

  formatTimestamp(timestamp: Date | number | null | undefined): string {
    if (!timestamp) return '--';
    const date = timestamp instanceof Date ? timestamp : new Date(timestamp);
    const locale = i18n.currentLang === 'de' ? 'de-AT' : 'en-US';
    const timeZone = i18n.currentLang === 'de' ? 'Europe/Vienna' : undefined;
    return date.toLocaleString(locale, { timeZone });
  },

  formatRelativeTime(timestamp: number | null | undefined): string {
    if (!timestamp) return '--';
    const seconds = Math.floor((Date.now() - timestamp) / 1000);
    if (seconds < 5) return i18n.t('uptime_just_now');
    if (seconds < 60) return i18n.t('uptime_seconds', { seconds });
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return i18n.t('uptime_minutes', { minutes });
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return i18n.t('uptime_hours', { hours });
    return this.formatTimestamp(timestamp);
  },

  rssiToLevel(rssi: number | undefined | null): 0 | 1 | 2 | 3 | 4 {
    if (!rssi || rssi === 99) return 0;
    if (rssi >= -65) return 4;
    if (rssi >= -75) return 3;
    if (rssi >= -85) return 2;
    if (rssi >= -95) return 1;
    return 0;
  },

  getAlertLabel(type: string | null | undefined): string {
    if (!type) return i18n.t('alert_UNKNOWN');
    const key = `alert_${type}`;
    const label = i18n.t(key);
    return label !== key ? label : i18n.t('alert_UNKNOWN');
  },

  openModal(htmlContent: string): void {
    const backdrop = document.createElement('div');
    backdrop.className = 'modal-backdrop';
    backdrop.id = 'modal-backdrop';
    backdrop.innerHTML = `<div class="modal">${htmlContent}</div>`;
    document.body.appendChild(backdrop);
    refreshIcons();

    backdrop.addEventListener('click', (e) => {
      if ((e.target as HTMLElement).id === 'modal-backdrop') {
        this.closeModal();
      }
    });
  },

  closeModal(): void {
    document.getElementById('modal-backdrop')?.remove();
  },
};
