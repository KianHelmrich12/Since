// SPDX-License-Identifier: GPL-3.0-or-later
// Anzeigeformate: Datum als 18.09.2026, 15:55, Komma als Dezimalzeichen,
// geschütztes Leerzeichen vor Einheiten.

import { DateTime } from './config/civil';
import { TimeZone } from './config/tz';

export const NBSP = ' ';

const pad = (n: number) => String(n).padStart(2, '0');

export function formatDateTime(dt: DateTime, withSeconds = false): string {
  const time = `${pad(dt.hour)}:${pad(dt.minute)}${withSeconds ? ':' + pad(dt.second) : ''}`;
  return `${pad(dt.day)}.${pad(dt.month)}.${dt.year}, ${time}`;
}

/** UTC-Epoch in der Lokalzeit einer POSIX-TZ; ungültige TZ fällt auf UTC zurück. */
export function formatEpochInTz(epoch: number, tz: string, withSeconds = false): string {
  const zone = TimeZone.parse(tz) ?? TimeZone.utc();
  return formatDateTime(zone.toLocal(epoch), withSeconds);
}

export function formatVolt(mv: number): string {
  const v = new Intl.NumberFormat('de-DE', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
  return `${v.format(mv / 1000)}${NBSP}V`;
}

export function formatPercent(p: number): string {
  return `${Math.round(p)}${NBSP}%`;
}

/** 512 → "8 min 32 s" */
export function formatDuration(seconds: number): string {
  const s = Math.max(0, Math.round(seconds));
  const min = Math.floor(s / 60);
  const rest = s % 60;
  if (min === 0) return `${rest}${NBSP}s`;
  return `${min}${NBSP}min ${rest}${NBSP}s`;
}

/** Abweichung zwischen zwei Uhren, z. B. "97 s nach" / "3 s vor". */
export function formatOffset(deltaS: number): string {
  if (deltaS === 0) return 'genau';
  const abs = Math.abs(deltaS);
  const amount = abs >= 120 ? `${Math.round(abs / 60)}${NBSP}min` : `${abs}${NBSP}s`;
  return deltaS < 0 ? `${amount} nach` : `${amount} vor`;
}
