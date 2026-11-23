// SPDX-License-Identifier: GPL-3.0-or-later
import { Injectable, computed, inject, signal } from '@angular/core';

import { ApiConfig, ConfigPatch } from './config/config';
import { DeviceInfo, DeviceStatus } from './protocol/protocol';
import { DEVICE_TRANSPORT, FoundDevice } from './transport/device-transport';

export type ConnectionState = 'getrennt' | 'verbindet' | 'verbunden';

const SCAN_DURATION_MS = 8000;

/** Zustand der Verbindung und die Operationen der App gegen das Gerät. */
@Injectable({ providedIn: 'root' })
export class DeviceService {
  private readonly transport = inject(DEVICE_TRANSPORT);

  readonly transportKind = this.transport.kind;
  readonly state = signal<ConnectionState>('getrennt');
  readonly scanning = signal(false);
  readonly found = signal<readonly FoundDevice[]>([]);
  readonly device = signal<FoundDevice | null>(null);
  readonly info = signal<DeviceInfo | null>(null);
  readonly config = signal<ApiConfig | null>(null);
  readonly status = signal<DeviceStatus | null>(null);
  /** Zeitpunkt (ms, Handy-Uhr), zu dem `status` gelesen wurde */
  readonly statusAt = signal(0);
  /** Grund, falls die Verbindung zuletzt unerwartet abbrach */
  readonly lostConnection = signal(false);

  readonly connected = computed(() => this.state() === 'verbunden');

  async scan(): Promise<void> {
    this.found.set([]);
    this.scanning.set(true);
    try {
      await this.transport.scan((d) => {
        this.found.update((list) =>
          list.some((x) => x.id === d.id) ? list.map((x) => (x.id === d.id ? d : x)) : [...list, d],
        );
      }, SCAN_DURATION_MS);
    } finally {
      this.scanning.set(false);
    }
  }

  async stopScan(): Promise<void> {
    await this.transport.stopScan();
    this.scanning.set(false);
  }

  async connect(device: FoundDevice): Promise<void> {
    this.state.set('verbindet');
    this.lostConnection.set(false);
    try {
      await this.transport.connect(device.id, () => this.onDropped());
      this.device.set(device);
      this.info.set(await this.transport.request('getInfo'));
      this.config.set(await this.transport.request('getConfig'));
      this.state.set('verbunden');
    } catch (err) {
      await this.transport.disconnect();
      this.reset();
      throw err;
    }
  }

  async disconnect(): Promise<void> {
    await this.transport.disconnect();
    this.reset();
  }

  async loadConfig(): Promise<ApiConfig> {
    const cfg = await this.transport.request('getConfig');
    this.config.set(cfg);
    return cfg;
  }

  async saveConfig(patch: ConfigPatch): Promise<ApiConfig> {
    const cfg = await this.transport.request('setConfig', { data: patch });
    this.config.set(cfg);
    return cfg;
  }

  async loadStatus(): Promise<DeviceStatus> {
    const s = await this.transport.request('getStatus');
    this.status.set(s);
    this.statusAt.set(Date.now());
    return s;
  }

  /** Überträgt die Uhrzeit des Handys (Unix-Sekunden UTC). */
  async setTimeFromPhone(): Promise<number> {
    const epoch = Math.floor(Date.now() / 1000);
    const r = await this.transport.request('setTime', { epoch });
    return r.epoch;
  }

  async redraw(): Promise<void> {
    await this.transport.request('redraw');
  }

  /** Beendet den Setup-Modus. Das Gerät trennt danach BLE und zeichnet neu. */
  async exitSetup(): Promise<void> {
    await this.transport.request('exitSetup');
    await this.transport.disconnect();
    this.reset();
  }

  private onDropped(): void {
    const wasConnected = this.state() !== 'getrennt';
    this.reset();
    this.lostConnection.set(wasConnected);
  }

  private reset(): void {
    this.state.set('getrennt');
    this.device.set(null);
    this.info.set(null);
    this.config.set(null);
    this.status.set(null);
  }
}
