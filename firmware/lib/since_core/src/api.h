// SPDX-License-Identifier: GPL-3.0-or-later
// Gemeinsame Geräte-API für Webportal (HTTP) und BLE (RPC). Die Hardware-Zugriffe
// stecken hinter DeviceHooks, damit die Logik auf dem Host testbar bleibt.
#pragma once

#include <string>

#include "civil.h"
#include "config.h"

namespace since {

constexpr int kBleProtocolVersion = 1;

struct DeviceStatus {
  std::string fw;
  std::string name;
  int batteryMv = 0;
  int batteryPercent = 0;
  int charging = -1;  // -1 unbekannt, 0/1
  bool rtcOk = false;
  bool rtcLostPower = false;
  Epoch nowUtc = 0;
  Epoch lastSync = 0;
  int setupRemainingS = 0;
};

class DeviceHooks {
 public:
  virtual ~DeviceHooks() = default;
  virtual Config loadConfig() = 0;
  virtual bool saveConfig(const Config& c) = 0;
  virtual bool setTime(Epoch utc) = 0;  // RTC stellen, Lost-Power-Flag löschen
  virtual DeviceStatus status() = 0;
  virtual void requestRedraw() = 0;
  virtual void requestExitSetup() = 0;
  virtual void touch() {}  // Aktivität (verlängert Setup-Timeout)
};

struct ApiResponse {
  bool ok = true;
  std::string error;  // Fehlercode (siehe docs/ble-protocol.md)
  std::string body;   // JSON: data bei ok, sonst {"error":…,"issues":[…]}
  int httpStatus() const;
};

std::string statusJson(const DeviceStatus& s);

// op: getInfo, getConfig, setConfig, setTime, getStatus, redraw, exitSetup
// params: JSON-Objekt (bei setConfig das Config-Objekt, bei setTime {"epoch":…})
ApiResponse callApi(DeviceHooks& dev, const std::string& op, const std::string& paramsJson);

// BLE-RPC: {"id":…,"op":…, …} → {"id":…,"ok":…,…}
std::string handleRpc(DeviceHooks& dev, const std::string& request);

}  // namespace since
