// SPDX-License-Identifier: GPL-3.0-or-later
// Kurzer NTP-Sync über das konfigurierte WLAN; WLAN ist danach wieder aus.
#pragma once

#include "civil.h"
#include "config.h"

namespace ntp {

// Verbindet (max. ~20 s), holt die Zeit, stellt die RTC. Gibt die neue UTC-Zeit oder 0 zurück.
since::Epoch sync(const since::Config& cfg);

}  // namespace ntp
