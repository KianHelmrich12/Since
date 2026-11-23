// SPDX-License-Identifier: GPL-3.0-or-later
// Wann muss das Gerät das nächste Mal aufwachen? Wann ist ein NTP-Sync fällig?
#pragma once

#include "civil.h"
#include "config.h"

namespace since {

constexpr Epoch kMaxSleepS = 24 * 3600;       // spätestens täglich (Akku, Meilensteine)
constexpr Epoch kWakeMarginS = 5;             // knapp nach der Grenze aufwachen
constexpr Epoch kSyncIntervalS = 7 * 24 * 3600;
constexpr Epoch kSyncRetryS = 24 * 3600;      // nach fehlgeschlagenem Versuch

// Nächster Weckzeitpunkt (UTC), immer > nowUtc.
// - interval: now + intervalMinutes
// - auto: nächste Änderung des angezeigten Texts: Tagesgrenze zur Uhrzeit des Ereignisses
//   (Tage/Monate/Jahre), lokale Mitternacht (Meilensteine), bei showTime die nächste
//   volle Stunde relativ zum Ereignis, beim Countdown der Ereigniszeitpunkt selbst.
// - ohne Ereignis oder Akku kritisch: now + kMaxSleepS
Epoch nextWakeUtc(const Config& cfg, Epoch nowUtc, bool batteryCritical);

bool isSyncDue(const Config& cfg, Epoch nowUtc, Epoch lastSyncUtc, Epoch lastAttemptUtc,
               bool rtcLostPower);

}  // namespace since
