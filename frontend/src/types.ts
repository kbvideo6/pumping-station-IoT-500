import type { Timestamp } from 'firebase/firestore';

// ── Realtime Database shapes ──────────────────────────────────

export interface LiveData {
  current: number;
  alert: boolean;
  alertType: string | null;
  rssi: number;
  uptimeSeconds?: number;
  firmwareVersion?: string;
  battVolts?: number;
  battPercent?: number;
  timestamp?: number;
}

export interface StationConfig {
  stationName?: string;
  highThreshold: number;
  lowThreshold: number;
  reportIntervalSec: number;
  configPollIntervalSec: number;
  pumpPowerKW: number;
  calibration: number;
  lat?: number;
  lng?: number;
  latestFirmware?: {
    version: string;
    url: string;
    checksum: string;
  };
}

export interface StationStatus {
  online: boolean;
  lastSeen: number;
}

export interface Station {
  live?: LiveData;
  config?: StationConfig;
  status?: StationStatus;
}

export interface StationsSnapshot {
  [stationId: string]: Station;
}

// ── Firestore document shapes ─────────────────────────────────

export interface AlertDocument {
  stationId: string;
  stationName: string;
  type: string;
  currentValue: number | null;
  threshold: number | null;
  timestamp: Timestamp | null;
  acknowledged: boolean;
  acknowledgedBy: string | null;
  acknowledgedAt: Timestamp | null;
  notifiedVia?: string[];
  emailSentTo?: string[];
}

export interface AlertDocumentWithId extends AlertDocument {
  id: string;
}

export interface UserDocument {
  email: string;
  role: 'admin' | 'viewer';
  displayName?: string;
  photoURL?: string;
  createdAt?: Timestamp;
  lastSeenAt?: Timestamp;
  isPlaceholder?: boolean;
}

// ── UI enumerations ────────────────────────────────────────────

export type ToastType = 'success' | 'error' | 'warn' | 'info';
export type StatusFilter = 'ALL' | 'ONLINE' | 'OFFLINE' | 'ALERT';
export type AlertFilterAck = 'ALL' | 'ACK' | 'UNACK';
export type AlertFilterType = 'ALL' | 'HIGH_CURRENT' | 'LOW_CURRENT' | 'DEVICE_OFFLINE';
