// SPDX-License-Identifier: GPL-3.0-or-later
// Gerätezustand im Setup-Modus + DeviceHooks-Implementierung für Portal und BLE.
#pragma once

#include <string>

#include "api.h"
#include "config.h"

namespace device {

constexpr unsigned long kSetupTimeoutMs = 10UL * 60UL * 1000UL;  // ohne Aktivität
constexpr unsigned long kSetupMaxMs = 30UL * 60UL * 1000UL;      // harte Obergrenze (Akku)

struct SetupState {
  std::string name;  // since-XXXX
  std::string password;
  std::string passkey;
  uint32_t passkeyNumeric = 0;
  unsigned long lastActivityMs = 0;
  bool exitRequested = false;
  bool redrawRequested = false;
  bool timeChanged = false;
};

SetupState& setup();

class Hooks : public since::DeviceHooks {
 public:
  since::Config loadConfig() override;
  bool saveConfig(const since::Config& c) override;
  bool setTime(since::Epoch utc) override;
  since::DeviceStatus status() override;
  void requestRedraw() override;
  void requestExitSetup() override;
  void touch() override;
};

Hooks& hooks();

// Zuletzt erfolgreicher / versuchter NTP-Sync (NVS)
since::Epoch lastSync();
since::Epoch lastSyncAttempt();
void storeSync(since::Epoch attempt, bool success);
void clearSyncState();

std::string deviceName();  // aus der WLAN-MAC

}  // namespace device
