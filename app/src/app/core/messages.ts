// SPDX-License-Identifier: GPL-3.0-or-later
// Deutsche Texte für Validierungs- und Fehlercodes aus docs/ble-protocol.md.

import {
  MAX_INTERVAL_MINUTES,
  MAX_NAME_CHARS,
  MAX_OCCASION_CHARS,
  MAX_PASSWORD_BYTES,
  MAX_SSID_BYTES,
  MAX_TZ_BYTES,
  MIN_INTERVAL_MINUTES,
  MIN_PASSWORD_BYTES,
  ValidationIssue,
} from './config/config';
import { DeviceError } from './protocol/protocol';

const ISSUE_TEXT: Record<string, string> = {
  'name:too_long': `Der Name hat mehr als ${MAX_NAME_CHARS} Zeichen. Kürze ihn.`,
  'name:invalid_utf8': 'Der Name enthält ungültige Zeichen. Gib ihn neu ein.',
  'occasion:too_long': `Der Anlass hat mehr als ${MAX_OCCASION_CHARS} Zeichen. Kürze ihn.`,
  'occasion:invalid_utf8': 'Der Anlass enthält ungültige Zeichen. Gib ihn neu ein.',
  'event:invalid': 'Dieses Datum gibt es nicht. Prüfe Tag, Monat und Uhrzeit (Jahr 1900 bis 2999).',
  'tz:too_long': `Die Zeitzone ist länger als ${MAX_TZ_BYTES} Bytes. Wähle eine Vorgabe.`,
  'tz:invalid': 'Das ist keine gültige POSIX-Zeitzone. Beispiel: CET-1CEST,M3.5.0,M10.5.0/3.',
  'layout:invalid': 'Dieses Layout kennt das Gerät nicht. Wähle Standard oder Minimal.',
  'update.mode:invalid': 'Diesen Aktualisierungsmodus kennt das Gerät nicht.',
  'update.intervalMinutes:out_of_range': `Das Intervall muss zwischen ${MIN_INTERVAL_MINUTES} und ${MAX_INTERVAL_MINUTES} Minuten liegen.`,
  'wifi.ssid:too_long': `Die SSID ist länger als ${MAX_SSID_BYTES} Bytes. Prüfe den Netzwerknamen.`,
  'wifi.password:length': `Das Passwort braucht ${MIN_PASSWORD_BYTES} bis ${MAX_PASSWORD_BYTES} Zeichen. Leer lassen, um es nicht zu ändern.`,
  'ntp:requires_wifi': 'NTP braucht ein WLAN. Trage zuerst die SSID ein.',
  'schemaVersion:unsupported':
    'Das Gerät kennt diese Config-Version nicht. Bring App und Firmware auf denselben Stand.',
};

const CODE_TEXT: Record<string, string> = {
  too_long: 'Der Wert ist zu lang.',
  invalid: 'Der Wert ist ungültig.',
  invalid_utf8: 'Der Wert enthält ungültige Zeichen.',
  out_of_range: 'Der Wert liegt außerhalb des erlaubten Bereichs.',
  length: 'Die Länge passt nicht.',
  requires_wifi: 'Dafür braucht das Gerät ein WLAN.',
  wrong_type:
    'Das Gerät erwartet hier einen anderen Datentyp. Bring App und Firmware auf denselben Stand.',
  unsupported: 'Das Gerät unterstützt diesen Wert nicht.',
};

export function issueMessage(issue: ValidationIssue): string {
  return (
    ISSUE_TEXT[`${issue.field}:${issue.code}`] ??
    CODE_TEXT[issue.code] ??
    `Ungültiger Wert (${issue.code}).`
  );
}

const ERROR_TEXT: Record<string, string> = {
  bad_request:
    'Das Gerät hat die Anfrage nicht verstanden. Bring App und Firmware auf denselben Stand.',
  unknown_op: 'Das Gerät kennt diese Funktion nicht. Aktualisiere die Firmware im Portal.',
  invalid_json: 'Das Gerät konnte die Einstellungen nicht lesen. Versuche es erneut.',
  wrong_type: 'Ein Feld hat den falschen Datentyp. Bring App und Firmware auf denselben Stand.',
  unsupported_version:
    'Das Gerät kennt diese Config-Version nicht. Bring App und Firmware auf denselben Stand.',
  invalid: 'Einige Werte sind ungültig. Korrigiere die markierten Felder.',
  too_large: 'Die Nachricht ist größer als 4096 Bytes. Kürze Name oder Anlass.',
  rtc_error: 'Die Uhr des Geräts (RTC) antwortet nicht. Prüfe die Verbindung zur DS3231.',
  timeout:
    'Das Gerät hat nicht innerhalb von 10 Sekunden geantwortet. Prüfe, ob der Setup-Modus noch läuft, und verbinde neu.',
  disconnected: 'Die Verbindung ist abgebrochen. Starte den Setup-Modus mit KEY0 und verbinde neu.',
  not_connected: 'Kein Gerät verbunden. Suche zuerst ein Gerät.',
  bad_response: 'Das Gerät hat eine unerwartete Antwort geschickt. Versuche es erneut.',
  ble_unavailable:
    'Bluetooth ist nicht verfügbar. Schalte Bluetooth ein und erlaube der App den Zugriff auf Geräte in der Nähe.',
};

export function errorMessage(err: unknown): string {
  if (err instanceof DeviceError) {
    return ERROR_TEXT[err.code] ?? `Das Gerät meldet einen Fehler (${err.code}).`;
  }
  const text = err instanceof Error ? err.message : String(err);
  return `Das hat nicht geklappt: ${text}`;
}
