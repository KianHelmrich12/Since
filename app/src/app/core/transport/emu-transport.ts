// SPDX-License-Identifier: GPL-3.0-or-later
// Verbindung zum Geräte-Emulator (firmware/tools/emulator.py) über HTTP statt Bluetooth.
// Der Emulator beantwortet Anfragen mit derselben Firmware-Logik (since::handleRpc); die
// App nutzt dabei denselben Framing- und RPC-Codepfad wie bei BLE.
// Aktivieren: ?transport=emu&emu=http://localhost:8080

import { FrameAssembler, splitFrames } from '../protocol/framing';
import { DeviceError, Op, OpParams, OpResult } from '../protocol/protocol';
import { RpcChannel } from '../protocol/rpc-channel';
import { DeviceTransport, FoundDevice } from './device-transport';

interface EmuState {
  setup: { active: boolean; ssid: string };
}

export class EmuTransport implements DeviceTransport {
  readonly kind = 'emu' as const;
  private readonly mtu = 185;
  private channel: RpcChannel | undefined;
  private scanAbort: AbortController | undefined;
  private poll: ReturnType<typeof setInterval> | undefined;

  constructor(
    readonly baseUrl: string,
    private readonly fetchFn: typeof fetch = (...a) => fetch(...a),
  ) {}

  private async state(): Promise<EmuState> {
    const r = await this.fetchFn(`${this.baseUrl}/emu/state`, { signal: this.scanAbort?.signal });
    return (await r.json()) as EmuState;
  }

  async scan(onFound: (device: FoundDevice) => void, durationMs: number): Promise<void> {
    await this.stopScan();
    this.scanAbort = new AbortController();
    const end = Date.now() + durationMs;
    // Wie echtes BLE: das Gerät ist nur im Setup-Modus (Taste 1) sichtbar.
    while (Date.now() < end && this.scanAbort && !this.scanAbort.signal.aborted) {
      try {
        const s = await this.state();
        if (s.setup.active) {
          onFound({ id: `emu:${s.setup.ssid}`, name: s.setup.ssid, rssi: -48 });
          return;
        }
      } catch {
        /* Emulator nicht erreichbar → weiter versuchen */
      }
      await new Promise((r) => setTimeout(r, 700));
    }
  }

  async stopScan(): Promise<void> {
    this.scanAbort?.abort();
    this.scanAbort = undefined;
  }

  async connect(_deviceId: string, onDisconnect: () => void): Promise<void> {
    await this.disconnect();
    let s: EmuState;
    try {
      s = await this.state();
    } catch {
      throw new DeviceError('not_connected', [], 'Emulator nicht erreichbar');
    }
    if (!s.setup.active) throw new DeviceError('not_connected', [], 'Gerät nicht im Setup-Modus');

    const assembler = new FrameAssembler();
    const channel: RpcChannel = new RpcChannel({
      mtu: () => this.mtu,
      write: async (frame) => {
        const r = assembler.push(frame);
        if (r.kind !== 'message') return;
        const resp = await this.fetchFn(`${this.baseUrl}/api/rpc`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: new TextDecoder().decode(r.bytes),
        });
        const bytes = new Uint8Array(await resp.arrayBuffer());
        for (const f of splitFrames(bytes, this.mtu)) channel.receive(f);
      },
    });
    this.channel = channel;
    // Endet der Setup-Modus im Emulator, trennt das „Gerät“ die Verbindung.
    this.poll = setInterval(async () => {
      try {
        if (!(await this.state()).setup.active && this.channel === channel) {
          await this.disconnect();
          onDisconnect();
        }
      } catch {
        /* ignorieren */
      }
    }, 1500);
  }

  async disconnect(): Promise<void> {
    clearInterval(this.poll);
    this.poll = undefined;
    this.channel?.close();
    this.channel = undefined;
  }

  request<K extends Op>(op: K, params?: OpParams<K>): Promise<OpResult<K>> {
    if (!this.channel) return Promise.reject(new DeviceError('not_connected'));
    return this.channel.request(op, params);
  }
}
