// SPDX-License-Identifier: GPL-3.0-or-later
// Echtes Bluetooth LE über @capacitor-community/bluetooth-le.
//
// MTU: Das Android-Plugin handelt die MTU beim Verbinden selbst aus (es fragt nach dem
// Maximum, die Firmware begrenzt). Die App liest die ausgehandelte MTU mit getMtu() und
// stückelt mit min(MTU, 247), wie es docs/ble-protocol.md verlangt.
//
// Kopplung: rx/tx verlangen eine verschlüsselte, authentifizierte Verbindung. Auf Android
// stößt createBond() die Kopplung an; das System fragt den Passkey vom Display ab.

import { BleClient, numbersToDataView } from '@capacitor-community/bluetooth-le';
import { Capacitor } from '@capacitor/core';

import {
  DeviceError,
  Op,
  OpParams,
  OpResult,
  REQUESTED_MTU,
  RX_UUID,
  SERVICE_UUID,
  TX_UUID,
} from '../protocol/protocol';
import { RpcChannel } from '../protocol/rpc-channel';
import { DeviceTransport, FoundDevice } from './device-transport';

/** Zeit für den Passkey-Dialog */
const BOND_TIMEOUT_MS = 60_000;
/** Mindest-MTU laut BLE-Spezifikation */
const MIN_MTU = 23;

export class BleTransport implements DeviceTransport {
  readonly kind = 'ble' as const;

  private initialized = false;
  private scanTimer: ReturnType<typeof setTimeout> | undefined;
  private scanDone: (() => void) | undefined;
  private deviceId: string | undefined;
  private mtu = MIN_MTU;
  private channel: RpcChannel | undefined;

  private async init(): Promise<void> {
    if (this.initialized) return;
    try {
      await BleClient.initialize({ androidNeverForLocation: true });
    } catch (err) {
      throw new DeviceError('ble_unavailable', [], String(err));
    }
    this.initialized = true;
  }

  async scan(onFound: (device: FoundDevice) => void, durationMs: number): Promise<void> {
    await this.init();
    await this.stopScan();
    if (!(await BleClient.isEnabled()))
      throw new DeviceError('ble_unavailable', [], 'Bluetooth ist aus');
    await BleClient.requestLEScan({ services: [SERVICE_UUID], allowDuplicates: false }, (r) => {
      onFound({
        id: r.device.deviceId,
        name: r.device.name ?? r.localName ?? r.device.deviceId,
        rssi: r.rssi,
      });
    });
    return new Promise<void>((resolve) => {
      this.scanDone = resolve;
      this.scanTimer = setTimeout(() => void this.stopScan(), durationMs);
    });
  }

  async stopScan(): Promise<void> {
    clearTimeout(this.scanTimer);
    this.scanTimer = undefined;
    const done = this.scanDone;
    this.scanDone = undefined;
    if (done) {
      try {
        await BleClient.stopLEScan();
      } finally {
        done();
      }
    }
  }

  async connect(deviceId: string, onDisconnect: () => void): Promise<void> {
    await this.init();
    await this.stopScan();
    await this.disconnect();

    await BleClient.connect(deviceId, () => {
      if (this.deviceId !== deviceId) return;
      this.channel?.close('disconnected');
      this.channel = undefined;
      this.deviceId = undefined;
      onDisconnect();
    });
    this.deviceId = deviceId;
    try {
      if (Capacitor.getPlatform() === 'android' && !(await BleClient.isBonded(deviceId))) {
        await BleClient.createBond(deviceId, { timeout: BOND_TIMEOUT_MS });
      }
      this.mtu = await this.readMtu(deviceId);
      const channel = new RpcChannel({
        mtu: () => this.mtu,
        write: (frame) =>
          BleClient.write(deviceId, SERVICE_UUID, RX_UUID, numbersToDataView(Array.from(frame))),
      });
      this.channel = channel;
      await BleClient.startNotifications(deviceId, SERVICE_UUID, TX_UUID, (value) => {
        channel.receive(new Uint8Array(value.buffer, value.byteOffset, value.byteLength));
      });
    } catch (err) {
      await this.disconnect();
      throw err instanceof DeviceError ? err : new DeviceError('disconnected', [], String(err));
    }
  }

  async disconnect(): Promise<void> {
    const id = this.deviceId;
    this.deviceId = undefined;
    this.channel?.close('disconnected');
    this.channel = undefined;
    if (!id) return;
    try {
      await BleClient.stopNotifications(id, SERVICE_UUID, TX_UUID);
    } catch {
      // Verbindung schon weg
    }
    try {
      await BleClient.disconnect(id);
    } catch {
      // Verbindung schon weg
    }
  }

  request<K extends Op>(op: K, params?: OpParams<K>): Promise<OpResult<K>> {
    if (!this.channel) return Promise.reject(new DeviceError('not_connected'));
    return this.channel.request(op, params);
  }

  private async readMtu(deviceId: string): Promise<number> {
    try {
      const mtu = await BleClient.getMtu(deviceId);
      return Math.max(MIN_MTU, Math.min(mtu, REQUESTED_MTU));
    } catch {
      return MIN_MTU; // Web: keine MTU-Abfrage, 23 ist immer erlaubt
    }
  }
}
