// SPDX-License-Identifier: GPL-3.0-or-later
// Gerätekonfiguration: Modell, Defaults, Validierung und JSON-Merge.
// Spiegelt firmware/lib/since_core/src/config.{h,cpp}; dort ist die Referenz.
// Kontrakttests gegen shared/fixtures/config/ halten beide Seiten gleich.

import { DateTime, isValidDateTime } from './civil';
import { DEFAULT_TZ, TimeZone } from './tz';

export const CONFIG_SCHEMA_VERSION = 1;

// Grenzen (Zeichen = Unicode-Codepoints, Bytes = UTF-8)
export const MAX_NAME_CHARS = 32;
export const MAX_OCCASION_CHARS = 64;
export const MAX_TZ_BYTES = 64;
export const MAX_SSID_BYTES = 32;
export const MIN_PASSWORD_BYTES = 8;
export const MAX_PASSWORD_BYTES = 63;
export const MIN_INTERVAL_MINUTES = 10;
export const MAX_INTERVAL_MINUTES = 1440;

export type Layout = 'standard' | 'minimal';
export type UpdateMode = 'auto' | 'interval';

/** Internes Modell, wie `since::Config`. */
export interface Config {
  schemaVersion: number;
  name: string;
  occasion: string;
  /** null = kein Ereignis. Lokale Zeit in `tz`. */
  event: DateTime | null;
  tz: string;
  layout: Layout;
  showTime: boolean;
  updateMode: UpdateMode;
  intervalMinutes: number;
  wifiSsid: string;
  wifiPassword: string;
  ntp: boolean;
  portalSecure: boolean;
}

/** JSON-Form wie bei GET /api/config bzw. getConfig (ohne Passwort). */
export interface ApiConfig {
  schemaVersion: number;
  name: string;
  occasion: string;
  event: string;
  tz: string;
  layout: Layout;
  showTime: boolean;
  update: { mode: UpdateMode; intervalMinutes: number };
  wifi: { ssid: string; password?: string; passwordSet?: boolean };
  ntp: boolean;
  portalSecure: boolean;
}

/** Partielles Dokument für setConfig. Fehlende Felder bleiben auf dem Gerät erhalten. */
export interface ConfigPatch {
  schemaVersion?: number;
  name?: string;
  occasion?: string;
  event?: string;
  tz?: string;
  layout?: Layout;
  showTime?: boolean;
  update?: { mode?: UpdateMode; intervalMinutes?: number };
  wifi?: { ssid?: string; password?: string };
  ntp?: boolean;
  portalSecure?: boolean;
}

export interface ValidationIssue {
  /** JSON-Pfad, z. B. "wifi.password" */
  field: string;
  /** z. B. "too_long", "invalid", "length" */
  code: string;
}

export type ConfigStatus = 'ok' | 'invalid_json' | 'wrong_type' | 'unsupported_version' | 'invalid';

export interface ConfigResult {
  status: ConfigStatus;
  issues: ValidationIssue[];
  /** Die neue Config, nur bei status "ok". */
  config?: Config;
}

export function defaultConfig(): Config {
  return {
    schemaVersion: CONFIG_SCHEMA_VERSION,
    name: '',
    occasion: '',
    event: null,
    tz: DEFAULT_TZ,
    layout: 'standard',
    showTime: false,
    updateMode: 'auto',
    intervalMinutes: 60,
    wifiSsid: '',
    wifiPassword: '',
    ntp: false,
    portalSecure: true,
  };
}

const encoder = new TextEncoder();

/** Länge in UTF-8-Bytes. */
export function utf8Bytes(s: string): number {
  return encoder.encode(s).length;
}

/**
 * Anzahl Unicode-Codepoints; -1 bei ungültigem Text. Ein JS-String ist UTF-16, ungültig
 * ist darin nur ein einzelnes Surrogat, das sich nicht als UTF-8 kodieren lässt.
 */
export function utf8Length(s: string): number {
  let count = 0;
  for (const ch of s) {
    const cp = ch.codePointAt(0) ?? 0;
    if (cp >= 0xd800 && cp <= 0xdfff) return -1;
    ++count;
  }
  return count;
}

const ISO_LOCAL = /^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2})(?::(\d{2}))?$/;

/** "YYYY-MM-DDTHH:MM" (Sekunden optional) → DateTime, sonst null. */
export function parseIsoLocal(s: string): DateTime | null {
  const m = ISO_LOCAL.exec(s);
  if (!m) return null;
  const dt: DateTime = {
    year: Number(m[1]),
    month: Number(m[2]),
    day: Number(m[3]),
    hour: Number(m[4]),
    minute: Number(m[5]),
    second: m[6] === undefined ? 0 : Number(m[6]),
  };
  return isValidDateTime(dt) ? dt : null;
}

const pad = (n: number, w = 2) => String(n).padStart(w, '0');

/** DateTime → "YYYY-MM-DDTHH:MM", Sekunden nur wenn ≠ 0 (wie die Firmware). */
export function formatIsoLocal(dt: DateTime): string {
  const base = `${pad(dt.year, 4)}-${pad(dt.month)}-${pad(dt.day)}T${pad(dt.hour)}:${pad(dt.minute)}`;
  return dt.second ? `${base}:${pad(dt.second)}` : base;
}

/** Alle Zeichen in den Display-Fonts vorhanden? (druckbares Latin-1, wie die Firmware) */
export function isDisplayable(s: string): boolean {
  for (const ch of s) {
    const cp = ch.codePointAt(0)!;
    if (cp < 0x20 || (cp >= 0x7f && cp < 0xa0) || cp > 0xff) return false;
  }
  return true;
}

export function validate(c: Config): ValidationIssue[] {
  const issues: ValidationIssue[] = [];
  const nameLen = utf8Length(c.name);
  if (nameLen < 0) issues.push({ field: 'name', code: 'invalid_utf8' });
  else if (nameLen > MAX_NAME_CHARS) issues.push({ field: 'name', code: 'too_long' });
  else if (!isDisplayable(c.name)) issues.push({ field: 'name', code: 'unsupported_char' });

  const occLen = utf8Length(c.occasion);
  if (occLen < 0) issues.push({ field: 'occasion', code: 'invalid_utf8' });
  else if (occLen > MAX_OCCASION_CHARS) issues.push({ field: 'occasion', code: 'too_long' });
  else if (!isDisplayable(c.occasion)) issues.push({ field: 'occasion', code: 'unsupported_char' });

  if (c.event !== null && !isValidDateTime(c.event))
    issues.push({ field: 'event', code: 'invalid' });

  if (utf8Bytes(c.tz) > MAX_TZ_BYTES) issues.push({ field: 'tz', code: 'too_long' });
  else if (TimeZone.parse(c.tz) === null) issues.push({ field: 'tz', code: 'invalid' });

  if (
    c.updateMode === 'interval' &&
    (c.intervalMinutes < MIN_INTERVAL_MINUTES || c.intervalMinutes > MAX_INTERVAL_MINUTES)
  ) {
    issues.push({ field: 'update.intervalMinutes', code: 'out_of_range' });
  }

  if (utf8Bytes(c.wifiSsid) > MAX_SSID_BYTES) issues.push({ field: 'wifi.ssid', code: 'too_long' });
  const pwLen = utf8Bytes(c.wifiPassword);
  if (pwLen !== 0 && (pwLen < MIN_PASSWORD_BYTES || pwLen > MAX_PASSWORD_BYTES)) {
    issues.push({ field: 'wifi.password', code: 'length' });
  }
  if (c.ntp && c.wifiSsid === '') issues.push({ field: 'ntp', code: 'requires_wifi' });
  return issues;
}

type JsonObject = Record<string, unknown>;

function isObject(v: unknown): v is JsonObject {
  return typeof v === 'object' && v !== null && !Array.isArray(v);
}

/** Wie ArduinoJson `is<int>()`: ganze Zahl im int32-Bereich. */
function isInt(v: unknown): v is number {
  return typeof v === 'number' && Number.isInteger(v) && v >= -2147483648 && v <= 2147483647;
}

function isNull(v: unknown): boolean {
  return v === undefined || v === null;
}

class Reader {
  wrongTypeSeen = false;
  readonly issues: ValidationIssue[] = [];

  wrongType(field: string): void {
    this.wrongTypeSeen = true;
    this.issues.push({ field, code: 'wrong_type' });
  }
  invalid(field: string): void {
    this.issues.push({ field, code: 'invalid' });
  }
  str(v: unknown, field: string, set: (s: string) => void): void {
    if (isNull(v)) return;
    if (typeof v !== 'string') return this.wrongType(field);
    set(v);
  }
  bool(v: unknown, field: string, set: (b: boolean) => void): void {
    if (isNull(v)) return;
    if (typeof v !== 'boolean') return this.wrongType(field);
    set(v);
  }
  int(v: unknown, field: string, set: (n: number) => void): void {
    if (isNull(v)) return;
    if (!isInt(v)) return this.wrongType(field);
    set(v);
  }
}

/**
 * Wendet ein (auch partielles) JSON-Dokument auf `cfg` an: fehlende Felder bleiben
 * erhalten, ein fehlendes wifi.password lässt das Passwort unverändert. `cfg` selbst wird
 * nie verändert; bei Erfolg steht die neue Config in `result.config`.
 * `json` ist ein JSON-Text oder ein bereits geparster Wert.
 */
export function applyJson(json: string | unknown, cfg: Config): ConfigResult {
  let doc: unknown = json;
  if (typeof json === 'string') {
    try {
      doc = JSON.parse(json);
    } catch {
      return { status: 'invalid_json', issues: [] };
    }
  }
  if (!isObject(doc)) return { status: 'invalid_json', issues: [] };
  const root = doc;

  let version = CONFIG_SCHEMA_VERSION;
  if (!isNull(root['schemaVersion'])) {
    if (!isInt(root['schemaVersion'])) {
      return { status: 'wrong_type', issues: [{ field: 'schemaVersion', code: 'wrong_type' }] };
    }
    version = root['schemaVersion'];
  }
  // Für v1 gibt es noch keine Migrationen; ältere oder neuere Versionen werden abgelehnt.
  if (version !== CONFIG_SCHEMA_VERSION) {
    return {
      status: 'unsupported_version',
      issues: [{ field: 'schemaVersion', code: 'unsupported' }],
    };
  }

  const next: Config = { ...cfg, event: cfg.event ? { ...cfg.event } : null };
  const r = new Reader();
  r.str(root['name'], 'name', (s) => (next.name = s));
  r.str(root['occasion'], 'occasion', (s) => (next.occasion = s));

  const ev = root['event'];
  if (!isNull(ev)) {
    if (typeof ev !== 'string') {
      r.wrongType('event');
    } else if (ev === '') {
      next.event = null;
    } else {
      const dt = parseIsoLocal(ev);
      if (dt) next.event = dt;
      else r.invalid('event');
    }
  }

  r.str(root['tz'], 'tz', (s) => (next.tz = s));

  const layout = root['layout'];
  if (!isNull(layout)) {
    if (typeof layout !== 'string') r.wrongType('layout');
    else if (layout === 'standard' || layout === 'minimal') next.layout = layout;
    else r.invalid('layout');
  }

  r.bool(root['showTime'], 'showTime', (b) => (next.showTime = b));

  const update = root['update'];
  if (!isNull(update)) {
    if (!isObject(update)) {
      r.wrongType('update');
    } else {
      const mode = update['mode'];
      if (!isNull(mode)) {
        if (typeof mode !== 'string') r.wrongType('update.mode');
        else if (mode === 'auto' || mode === 'interval') next.updateMode = mode;
        else r.invalid('update.mode');
      }
      r.int(update['intervalMinutes'], 'update.intervalMinutes', (n) => (next.intervalMinutes = n));
    }
  }

  const wifi = root['wifi'];
  if (!isNull(wifi)) {
    if (!isObject(wifi)) {
      r.wrongType('wifi');
    } else {
      r.str(wifi['ssid'], 'wifi.ssid', (s) => (next.wifiSsid = s));
      if (!isNull(wifi['password'])) {
        r.str(wifi['password'], 'wifi.password', (s) => (next.wifiPassword = s));
      } else if (next.wifiSsid !== cfg.wifiSsid) {
        // Neues (oder kein) Netz ohne neues Passwort: das alte gehört nicht dazu (wie die Firmware).
        next.wifiPassword = '';
      }
    }
  }

  r.bool(root['ntp'], 'ntp', (b) => (next.ntp = b));
  r.bool(root['portalSecure'], 'portalSecure', (b) => (next.portalSecure = b));

  if (r.wrongTypeSeen) return { status: 'wrong_type', issues: r.issues };
  if (r.issues.length > 0) return { status: 'invalid', issues: r.issues };
  const issues = validate(next);
  if (issues.length > 0) return { status: 'invalid', issues };
  next.schemaVersion = CONFIG_SCHEMA_VERSION;
  return { status: 'ok', issues: [], config: next };
}

/**
 * includeSecrets=false (API): statt des Passworts nur "passwordSet".
 * includeSecrets=true: vollständig.
 */
export function toJson(c: Config, includeSecrets: boolean): ApiConfig {
  return {
    schemaVersion: CONFIG_SCHEMA_VERSION,
    name: c.name,
    occasion: c.occasion,
    event: c.event ? formatIsoLocal(c.event) : '',
    tz: c.tz,
    layout: c.layout,
    showTime: c.showTime,
    update: { mode: c.updateMode, intervalMinutes: c.intervalMinutes },
    wifi: includeSecrets
      ? { ssid: c.wifiSsid, password: c.wifiPassword }
      : { ssid: c.wifiSsid, passwordSet: c.wifiPassword !== '' },
    ntp: c.ntp,
    portalSecure: c.portalSecure,
  };
}
