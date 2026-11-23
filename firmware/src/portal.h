// SPDX-License-Identifier: GPL-3.0-or-later
// Setup-Hotspot mit Captive Portal, Web-UI aus LittleFS, REST-API und OTA-Upload.
#pragma once

namespace portal {

bool start();  // Hotspot + DNS + Webserver; WLAN-Daten aus device::setup()
void loop();
void stop();
bool rebootRequested();  // nach OTA oder Werksreset

}  // namespace portal
