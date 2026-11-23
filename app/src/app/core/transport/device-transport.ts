// SPDX-License-Identifier: GPL-3.0-or-later
import { InjectionToken } from '@angular/core';

import { Op, OpParams, OpResult } from '../protocol/protocol';

export interface FoundDevice {
  /** Plattform-ID (Android: MAC-Adresse) */
  id: string;
  /** Advertising-Name, z. B. since-AB12 */
  name: string;
  rssi?: number;
}

/**
 * Verbindung zu einem since-Gerät. BleTransport spricht echtes Bluetooth LE,
 * MockTransport simuliert ein Gerät im Browser, EmuTransport spricht mit dem Geräte-Emulator.
 */
export interface DeviceTransport {
  readonly kind: 'ble' | 'mock' | 'emu';

  /** Sucht nach Geräten mit der since-Service-UUID, bis `durationMs` um ist oder stopScan(). */
  scan(onFound: (device: FoundDevice) => void, durationMs: number): Promise<void>;
  stopScan(): Promise<void>;

  /**
   * Verbindet, koppelt bei Bedarf (Android zeigt den Passkey-Dialog) und abonniert `tx`.
   * `onDisconnect` meldet ein Verbindungsende, das nicht von disconnect() kam.
   */
  connect(deviceId: string, onDisconnect: () => void): Promise<void>;
  disconnect(): Promise<void>;

  /** Schickt eine Anfrage und wartet auf die Antwort mit derselben id. Wirft DeviceError. */
  request<K extends Op>(op: K, params?: OpParams<K>): Promise<OpResult<K>>;
}

export const DEVICE_TRANSPORT = new InjectionToken<DeviceTransport>('DEVICE_TRANSPORT');
