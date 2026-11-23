// SPDX-License-Identifier: GPL-3.0-or-later
// Framing nach docs/ble-protocol.md:
//   Byte 0  Flags: 0x01 = FIRST, 0x02 = LAST (eine Nachricht in einem Frame: 0x03)
//   Byte 1… Nutzdaten, höchstens MTU − 3 − 1 Bytes

import { FLAG_FIRST, FLAG_LAST, MAX_MESSAGE_BYTES } from './protocol';

/** ATT-Header (3 Bytes) und Flag-Byte gehen von der MTU ab. */
export function payloadSize(mtu: number): number {
  const size = mtu - 3 - 1;
  if (!Number.isInteger(size) || size < 1) throw new RangeError(`MTU ${mtu} ist zu klein`);
  return size;
}

/** Zerlegt eine Nachricht in Frames. Eine leere Nachricht ergibt einen Frame [0x03]. */
export function splitFrames(message: Uint8Array, mtu: number): Uint8Array[] {
  const size = payloadSize(mtu);
  const count = Math.max(1, Math.ceil(message.length / size));
  const frames: Uint8Array[] = [];
  for (let i = 0; i < count; ++i) {
    const chunk = message.subarray(i * size, Math.min(message.length, (i + 1) * size));
    const frame = new Uint8Array(chunk.length + 1);
    frame[0] = (i === 0 ? FLAG_FIRST : 0) | (i === count - 1 ? FLAG_LAST : 0);
    frame.set(chunk, 1);
    frames.push(frame);
  }
  return frames;
}

export type AssembleResult =
  | { kind: 'message'; bytes: Uint8Array }
  | { kind: 'partial' }
  | { kind: 'dropped'; reason: 'empty' | 'no_first' | 'too_large' };

/**
 * Setzt empfangene Frames wieder zusammen. Ein FIRST-Frame verwirft eine halb empfangene
 * Nachricht; Frames ohne FIRST ohne laufende Nachricht werden verworfen; eine Nachricht über
 * `maxBytes` wird verworfen, bis der nächste FIRST-Frame kommt.
 */
export class FrameAssembler {
  private chunks: Uint8Array[] = [];
  private size = 0;
  private active = false;

  constructor(private readonly maxBytes = MAX_MESSAGE_BYTES) {}

  get inProgress(): boolean {
    return this.active;
  }

  reset(): void {
    this.chunks = [];
    this.size = 0;
    this.active = false;
  }

  push(frame: Uint8Array): AssembleResult {
    if (frame.length === 0) return { kind: 'dropped', reason: 'empty' };
    const flags = frame[0];
    if (flags & FLAG_FIRST) {
      this.reset();
      this.active = true;
    } else if (!this.active) {
      return { kind: 'dropped', reason: 'no_first' };
    }
    const payload = frame.subarray(1);
    if (this.size + payload.length > this.maxBytes) {
      this.reset();
      return { kind: 'dropped', reason: 'too_large' };
    }
    this.chunks.push(payload.slice());
    this.size += payload.length;
    if (!(flags & FLAG_LAST)) return { kind: 'partial' };
    const bytes = new Uint8Array(this.size);
    let offset = 0;
    for (const c of this.chunks) {
      bytes.set(c, offset);
      offset += c.length;
    }
    this.reset();
    return { kind: 'message', bytes };
  }
}

const encoder = new TextEncoder();
const decoder = new TextDecoder('utf-8', { fatal: true });

export function encodeJson(value: unknown): Uint8Array {
  return encoder.encode(JSON.stringify(value));
}

/** Wirft bei ungültigem UTF-8 oder JSON. */
export function decodeJson(bytes: Uint8Array): unknown {
  return JSON.parse(decoder.decode(bytes));
}
