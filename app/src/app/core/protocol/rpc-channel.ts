// SPDX-License-Identifier: GPL-3.0-or-later
// Anfrage/Antwort über eine Frame-Verbindung: id-Vergabe, Stückelung, Zusammensetzen der
// Notifications, Zuordnung per id und Timeouts. Von BleTransport und MockTransport genutzt.

import { FrameAssembler, decodeJson, encodeJson, splitFrames } from './framing';
import {
  DeviceError,
  MAX_MESSAGE_BYTES,
  Op,
  OpParams,
  OpResult,
  REQUESTED_MTU,
  Request,
  Response,
} from './protocol';

/** Die Verbindung unterhalb des Kanals: schreibt Frames auf `rx`. */
export interface FrameLink {
  /** Aktuelle MTU der Verbindung. */
  mtu(): number;
  write(frame: Uint8Array): Promise<void>;
}

export interface RpcChannelOptions {
  /** Timeout je Anfrage in ms */
  timeoutMs?: number;
}

interface Pending {
  resolve(data: unknown): void;
  reject(err: unknown): void;
  timer: ReturnType<typeof setTimeout>;
}

export const DEFAULT_TIMEOUT_MS = 10_000;

export class RpcChannel {
  private nextId = 1;
  private readonly pending = new Map<number, Pending>();
  private readonly assembler = new FrameAssembler();
  private writeQueue: Promise<void> = Promise.resolve();
  private closed = false;

  constructor(
    private readonly link: FrameLink,
    private readonly options: RpcChannelOptions = {},
  ) {}

  get pendingCount(): number {
    return this.pending.size;
  }

  request<K extends Op>(op: K, params?: OpParams<K>, timeoutMs?: number): Promise<OpResult<K>> {
    if (this.closed) return Promise.reject(new DeviceError('not_connected'));
    const id = this.allocateId();
    const req: Request = { ...(params ?? {}), id, op };
    const bytes = encodeJson(req);
    if (bytes.length > MAX_MESSAGE_BYTES) return Promise.reject(new DeviceError('too_large'));

    return new Promise<OpResult<K>>((resolve, reject) => {
      const timer = setTimeout(
        () => {
          this.pending.delete(id);
          reject(new DeviceError('timeout'));
        },
        timeoutMs ?? this.options.timeoutMs ?? DEFAULT_TIMEOUT_MS,
      );
      this.pending.set(id, { resolve: resolve as (d: unknown) => void, reject, timer });

      // Frames einer Nachricht dürfen sich nicht mit denen einer anderen mischen.
      const mtu = Math.min(this.link.mtu(), REQUESTED_MTU);
      const frames = splitFrames(bytes, mtu);
      this.writeQueue = this.writeQueue
        .then(async () => {
          for (const f of frames) {
            if (!this.pending.has(id)) return; // Timeout oder geschlossen
            await this.link.write(f);
          }
        })
        .catch((err: unknown) => this.fail(id, err));
    });
  }

  /** Eine Notification von `tx`. */
  receive(frame: Uint8Array): void {
    const r = this.assembler.push(frame);
    if (r.kind !== 'message') return;
    let msg: unknown;
    try {
      msg = decodeJson(r.bytes);
    } catch {
      return; // kaputte Nachricht: die Anfrage läuft in ihren Timeout
    }
    if (typeof msg !== 'object' || msg === null) return;
    const res = msg as Response;
    if (typeof res.id !== 'number') return;
    const p = this.pending.get(res.id);
    if (!p) return; // verspätete Antwort nach Timeout
    this.pending.delete(res.id);
    clearTimeout(p.timer);
    if (res.ok === true) p.resolve(res.data ?? {});
    else p.reject(new DeviceError(res.error ?? 'bad_response', res.issues ?? []));
  }

  /** Bricht alle offenen Anfragen ab. Danach nimmt der Kanal keine Anfragen mehr an. */
  close(code = 'disconnected'): void {
    this.closed = true;
    this.assembler.reset();
    for (const [id, p] of this.pending) {
      clearTimeout(p.timer);
      p.reject(new DeviceError(code));
      this.pending.delete(id);
    }
  }

  private fail(id: number, err: unknown): void {
    const p = this.pending.get(id);
    if (!p) return;
    this.pending.delete(id);
    clearTimeout(p.timer);
    p.reject(err instanceof DeviceError ? err : new DeviceError('disconnected', [], String(err)));
  }

  private allocateId(): number {
    const id = this.nextId;
    this.nextId = this.nextId >= 0x7fffffff ? 1 : this.nextId + 1;
    return id;
  }
}
