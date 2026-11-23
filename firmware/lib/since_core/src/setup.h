// SPDX-License-Identifier: GPL-3.0-or-later
// Setup-Modus: Gerätename, zufällige Zugangsdaten, WLAN-QR-Code.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace since {

// "since-AB12" aus den letzten beiden MAC-Bytes
std::string deviceName(const uint8_t mac[6]);

// Zufallsquelle: liefert gleichverteilte 32-bit-Werte (Gerät: esp_random()).
using RandomFn = std::function<uint32_t()>;

// 10 Zeichen aus einem Alphabet ohne Verwechsler (kein 0/O, 1/l/I)
std::string randomWifiPassword(const RandomFn& rnd);
// 6 Ziffern, führende Nullen erhalten
std::string randomPasskey(const RandomFn& rnd, uint32_t* numeric = nullptr);

// "WIFI:T:WPA;S:<ssid>;P:<pwd>;;" mit Escaping von \ ; , : "
std::string wifiQrPayload(const std::string& ssid, const std::string& password);

}  // namespace since
