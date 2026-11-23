// SPDX-License-Identifier: GPL-3.0-or-later
// Simuliertes Gerät hinter derselben Framing- und RPC-Schicht wie BLE: Anfragen werden
// gestückelt, im MockDevice zusammengesetzt, beantwortet und als "Notifications" zurück
// gestückelt. So läuft `ng serve` ohne Hardware über denselben Codepfad.

import { FrameAssembler, encodeJson, splitFrames } from '../protocol/framing';
import { DeviceError, Op, OpParams, OpResult } from '../protocol/protocol';
import { RpcChannel } from '../protocol/rpc-channel';
import { DeviceTransport, FoundDevice } from './device-transport';
import { MockDevice } from './mock-device';

export interface MockTransportOptions {
  /** Verzögerung je Antwort in ms */
  latencyMs?: number;
  /** MTU der simulierten Verbindung; klein genug, dass mehrere Frames entstehen */
  mtu?: number;
  devices?: MockDevice[];
  timeoutMs?: number;
}

const wait = (ms: number) => new Promise<void>((r) => setTimeout(r, ms));

export class MockTransport implements DeviceTransport {
  readonly kind = 'mock' as const;
  readonly devices: MockDevice[];

  private readonly latencyMs: number;
  private readonly mtu: number;
  private readonly timeoutMs?: number;
  private scanTimer: ReturnType<typeof setTimeout> | undefined;
  private scanDone: (() => void) | undefined;
  private device: MockDevice | undefined;
  private channel: RpcChannel | undefined;
  private onDisconnect: (() => void) | undefined;

  constructor(options: MockTransportOptions = {}) {
    this.latencyMs = options.latencyMs ?? 120;
    this.mtu = options.mtu ?? 185;
    this.timeoutMs = options.timeoutMs;
    this.devices = options.devices ?? [new MockDevice()];
  }

  get connectedDevice(): MockDevice | undefined {
    return this.device;
  }

  async scan(onFound: (device: FoundDevice) => void, durationMs: number): Promise<void> {
    await this.stopScan();
    this.devices.forEach((d, i) => {
      setTimeout(
        () => onFound({ id: `mock:${d.name}`, name: d.name, rssi: -52 - 9 * i }),
        300 + 250 * i,
      );
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
    done?.();
  }

  async connect(deviceId: string, onDisconnect: () => void): Promise<void> {
    await this.disconnect();
    const device = this.devices.find((d) => `mock:${d.name}` === deviceId);
    if (!device) throw new DeviceError('not_connected', [], `Gerät ${deviceId} nicht gefunden`);
    await wait(this.latencyMs * 3);
    device.setupEnded = false;
    this.device = device;
    this.onDisconnect = onDisconnect;

    const assembler = new FrameAssembler();
    const channel = new RpcChannel(
      {
        mtu: () => this.mtu,
        write: async (frame) => {
          if (this.channel !== channel) throw new DeviceError('disconnected');
          const r = assembler.push(frame);
          if (r.kind === 'dropped' && r.reason === 'too_large') {
            this.notify(channel, { id: 0, ok: false, error: 'too_large' });
          } else if (r.kind === 'message') {
            this.notify(channel, device.handle(r.bytes));
          }
        },
      },
      { timeoutMs: this.timeoutMs },
    );
    this.channel = channel;
  }

  async disconnect(): Promise<void> {
    this.channel?.close();
    this.channel = undefined;
    this.device = undefined;
    this.onDisconnect = undefined;
  }

  request<K extends Op>(op: K, params?: OpParams<K>): Promise<OpResult<K>> {
    if (!this.channel) return Promise.reject(new DeviceError('not_connected'));
    return this.channel.request(op, params);
  }

  /** Simuliert einen Verbindungsabbruch vom Gerät aus (z. B. Setup-Timeout). */
  simulateDrop(): void {
    const cb = this.onDisconnect;
    void this.disconnect();
    cb?.();
  }

  private notify(channel: RpcChannel, reply: Record<string, unknown>): void {
    const frames = splitFrames(encodeJson(reply), this.mtu);
    setTimeout(() => {
      if (this.channel !== channel) return;
      for (const f of frames) channel.receive(f);
      // Nach exitSetup beendet das Gerät BLE.
      if (reply['ok'] === true && this.device?.setupEnded) this.simulateDrop();
    }, this.latencyMs);
  }
}
