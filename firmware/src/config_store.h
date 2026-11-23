// SPDX-License-Identifier: GPL-3.0-or-later
// Persistenz der Config im NVS (Preferences). Gespeichert wird das JSON aus since_core
// inklusive WLAN-Passwort; es verlässt das Gerät nie über die API.
#pragma once

#include "config.h"

namespace since {

// Lädt die Config. Fehlt sie oder ist sie ungültig, werden die Defaults zurückgegeben
// (und *loaded = false).
Config loadConfig(bool* loaded = nullptr);
bool saveConfig(const Config& c);
bool eraseConfig();

}  // namespace since
