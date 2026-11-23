// SPDX-License-Identifier: GPL-3.0-or-later
// Formularmodell der Einstellungsseite und die Umrechnung von/zu ApiConfig/ConfigPatch.

import {
  ApiConfig,
  Config,
  ConfigPatch,
  Layout,
  UpdateMode,
  ValidationIssue,
  applyJson,
  defaultConfig,
} from '../core/config/config';
import { DEFAULT_TZ } from '../core/config/tz';

export interface TzPreset {
  value: string;
  label: string;
}

export const CUSTOM_TZ = 'custom';

export const TZ_PRESETS: readonly TzPreset[] = [
  { value: DEFAULT_TZ, label: 'Deutschland, Österreich, Schweiz' },
  { value: 'GMT0BST,M3.5.0/1,M10.5.0', label: 'Vereinigtes Königreich' },
  { value: 'UTC0', label: 'UTC' },
];

export interface SettingsForm {
  name: string;
  occasion: string;
  /** YYYY-MM-DD, leer = kein Ereignis */
  eventDate: string;
  /** HH:MM */
  eventTime: string;
  tzPreset: string;
  tzCustom: string;
  layout: Layout;
  showTime: boolean;
  updateMode: UpdateMode;
  intervalMinutes: number;
  ssid: string;
  /** leer = Passwort unverändert lassen */
  password: string;
  ntp: boolean;
  portalSecure: boolean;
}

export type FormKey = keyof SettingsForm;

export function emptyForm(): SettingsForm {
  return fromApi(toApi(defaultConfig()));
}

function toApi(c: Config): ApiConfig {
  return {
    schemaVersion: 1,
    name: c.name,
    occasion: c.occasion,
    event: '',
    tz: c.tz,
    layout: c.layout,
    showTime: c.showTime,
    update: { mode: c.updateMode, intervalMinutes: c.intervalMinutes },
    wifi: { ssid: c.wifiSsid, passwordSet: false },
    ntp: c.ntp,
    portalSecure: c.portalSecure,
  };
}

export function fromApi(api: ApiConfig): SettingsForm {
  const preset = TZ_PRESETS.find((p) => p.value === api.tz);
  return {
    name: api.name,
    occasion: api.occasion,
    eventDate: api.event ? api.event.slice(0, 10) : '',
    eventTime: api.event ? api.event.slice(11, 16) : '00:00',
    tzPreset: preset ? preset.value : CUSTOM_TZ,
    tzCustom: api.tz,
    layout: api.layout,
    showTime: api.showTime,
    updateMode: api.update.mode,
    intervalMinutes: api.update.intervalMinutes,
    ssid: api.wifi.ssid,
    password: '',
    ntp: api.ntp,
    portalSecure: api.portalSecure,
  };
}

export function selectedTz(f: SettingsForm): string {
  return f.tzPreset === CUSTOM_TZ ? f.tzCustom.trim() : f.tzPreset;
}

/** Das Dokument für setConfig. Ein leeres Passwort wird weggelassen und bleibt erhalten. */
export function toPatch(f: SettingsForm): ConfigPatch {
  const interval = f.intervalMinutes;
  const update: ConfigPatch['update'] = { mode: f.updateMode };
  // Im Auto-Modus prüft das Gerät das Intervall nicht; ein leeres Feld schicken wir dann nicht.
  if (Number.isInteger(interval)) update.intervalMinutes = interval;
  const wifi: ConfigPatch['wifi'] = { ssid: f.ssid };
  if (f.password !== '') wifi.password = f.password;
  return {
    schemaVersion: 1,
    name: f.name,
    occasion: f.occasion,
    event: f.eventDate ? `${f.eventDate}T${f.eventTime || '00:00'}` : '',
    tz: selectedTz(f),
    layout: f.layout,
    showTime: f.showTime,
    update,
    wifi,
    ntp: f.ntp,
    portalSecure: f.portalSecure,
  };
}

/** Welches Formularfeld zeigt den Fehler zu einem JSON-Pfad? */
export function formKeyFor(field: string, f: SettingsForm): FormKey | null {
  switch (field) {
    case 'name':
      return 'name';
    case 'occasion':
      return 'occasion';
    case 'event':
      return 'eventDate';
    case 'tz':
      return f.tzPreset === CUSTOM_TZ ? 'tzCustom' : 'tzPreset';
    case 'layout':
      return 'layout';
    case 'showTime':
      return 'showTime';
    case 'update.mode':
      return 'updateMode';
    case 'update.intervalMinutes':
      return 'intervalMinutes';
    case 'wifi.ssid':
      return 'ssid';
    case 'wifi.password':
      return 'password';
    case 'ntp':
      return 'ntp';
    case 'portalSecure':
      return 'portalSecure';
    default:
      return null;
  }
}

/**
 * Prüft das Formular mit derselben Logik wie das Gerät: der Patch wird auf die zuletzt
 * gelesene Config angewendet.
 */
export function validateForm(f: SettingsForm, base: Config = defaultConfig()): ValidationIssue[] {
  const issues: ValidationIssue[] = [];
  if (f.updateMode === 'interval' && !Number.isInteger(f.intervalMinutes)) {
    issues.push({ field: 'update.intervalMinutes', code: 'out_of_range' });
  }
  if (f.eventDate && !/^\d{2}:\d{2}$/.test(f.eventTime || '00:00')) {
    issues.push({ field: 'event', code: 'invalid' });
  }
  const r = applyJson(toPatch(f), base);
  for (const i of r.issues) {
    if (!issues.some((x) => x.field === i.field)) issues.push(i);
  }
  return issues;
}

/** ApiConfig vom Gerät → internes Modell, als Basis für validateForm. */
export function configFromApi(api: ApiConfig): Config {
  return applyJson(api, defaultConfig()).config ?? defaultConfig();
}
