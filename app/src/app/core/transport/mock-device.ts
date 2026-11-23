// SPDX-License-Identifier: GPL-3.0-or-later
// Simuliertes since-Gerät für Browser-Entwicklung und Tests. Verhält sich wie die Firmware
// laut docs/ble-protocol.md: gleiche Ops, gleiche Fehlercodes, gleiche Config-Validierung.

import { Config, ConfigStatus, applyJson, defaultConfig, toJson } from '../config/config';
import {
  DeviceInfo,
  DeviceStatus,
  MAX_MESSAGE_BYTES,
  PROTOCOL_VERSION,
} from '../protocol/protocol';

export interface MockDeviceOptions {
  name?: string;
  fw?: string;
  config?: Config;
  /** Uhr für Tests; liefert Unix-Millisekunden. */
  now?: () => number;
  /** Abweichung der RTC vom Handy in Sekunden, bis setTime kommt. */
  rtcDriftS?: number;
  setupTimeoutS?: number;
}

type Reply = Record<string, unknown>;

const STATUS_ERROR: Record<Exclude<ConfigStatus, 'ok'>, string> = {
  invalid_json: 'invalid_json',
  wrong_type: 'wrong_type',
  unsupported_version: 'unsupported_version',
  invalid: 'invalid',
};

export class MockDevice {
  readonly name: string;
  readonly fw: string;
  config: Config;
  /** true, sobald exitSetup kam */
  setupEnded = false;
  redrawRequested = false;

  private readonly now: () => number;
  private rtcOffsetS: number;
  private lastSync = 0;
  private lastActivityMs: number;
  private readonly setupTimeoutS: number;

  constructor(options: MockDeviceOptions = {}) {
    this.name = options.name ?? 'since-AB12';
    this.fw = options.fw ?? '1.0.0';
    this.config = options.config ?? defaultConfig();
    this.now = options.now ?? (() => Date.now());
    this.rtcOffsetS = options.rtcDriftS ?? -97;
    this.setupTimeoutS = options.setupTimeoutS ?? 600;
    this.lastActivityMs = this.now();
  }

  info(): DeviceInfo {
    return { proto: PROTOCOL_VERSION, fw: this.fw, name: this.name };
  }

  status(): DeviceStatus {
    const nowMs = this.now();
    const elapsedS = Math.floor((nowMs - this.lastActivityMs) / 1000);
    return {
      fw: this.fw,
      name: this.name,
      batteryMv: 3912,
      batteryPercent: 78,
      charging: false,
      rtcOk: true,
      rtcLostPower: false,
      nowUtc: Math.floor(nowMs / 1000) + this.rtcOffsetS,
      lastSync: this.lastSync,
      setupRemainingS: Math.max(0, this.setupTimeoutS - elapsedS),
    };
  }

  /** Verarbeitet eine vollständige Anfrage (UTF-8-JSON) und liefert die Antwort als Objekt. */
  handle(raw: Uint8Array | string): Reply {
    const text = typeof raw === 'string' ? raw : new TextDecoder().decode(raw);
    const size = typeof raw === 'string' ? new TextEncoder().encode(raw).length : raw.length;
    if (size > MAX_MESSAGE_BYTES) return { id: 0, ok: false, error: 'too_large' };

    let req: unknown;
    try {
      req = JSON.parse(text);
    } catch {
      return { id: 0, ok: false, error: 'bad_request' };
    }
    if (typeof req !== 'object' || req === null || Array.isArray(req)) {
      return { id: 0, ok: false, error: 'bad_request' };
    }
    const r = req as Record<string, unknown>;
    const id = r['id'];
    const op = r['op'];
    if (typeof id !== 'number' || !Number.isInteger(id) || typeof op !== 'string') {
      return { id: typeof id === 'number' ? id : 0, ok: false, error: 'bad_request' };
    }
    this.lastActivityMs = this.now();
    const ok = (data: unknown): Reply => ({ id, ok: true, data });
    const fail = (error: string, issues?: unknown): Reply =>
      issues ? { id, ok: false, error, issues } : { id, ok: false, error };

    switch (op) {
      case 'getInfo':
        return ok(this.info());
      case 'getConfig':
        return ok(toJson(this.config, false));
      case 'setConfig': {
        const result = applyJson(r['data'] ?? null, this.config);
        if (result.status !== 'ok') {
          const error = STATUS_ERROR[result.status];
          return error === 'invalid' ? fail(error, result.issues) : fail(error);
        }
        this.config = result.config!;
        return ok(toJson(this.config, false));
      }
      case 'setTime': {
        const epoch = r['epoch'];
        // Wie die Firmware (since_core/api.cpp): Ganzzahl 2024-01-01 … 2100-01-01, sonst bad_request
        if (
          typeof epoch !== 'number' ||
          !Number.isInteger(epoch) ||
          epoch < 1704067200 ||
          epoch > 4102444800
        ) {
          return fail('bad_request');
        }
        this.rtcOffsetS = epoch - Math.floor(this.now() / 1000);
        return ok({ epoch });
      }
      case 'getStatus':
        return ok(this.status());
      case 'redraw':
        this.redrawRequested = true;
        return ok({});
      case 'exitSetup':
        this.setupEnded = true;
        return ok({});
      default:
        return fail('unknown_op');
    }
  }
}
