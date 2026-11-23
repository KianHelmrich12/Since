// SPDX-License-Identifier: GPL-3.0-or-later
#include "ntp_sync.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <esp_task_wdt.h>
#include <sys/time.h>

#include "hw.h"

namespace ntp {

since::Epoch sync(const since::Config& cfg) {
  if (cfg.wifiSsid.empty()) return 0;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    esp_task_wdt_reset();
    delay(100);
  }
  since::Epoch result = 0;
  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.google.com");  // UTC
    const unsigned long t0 = millis();
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && millis() - t0 < 10000) {
      esp_task_wdt_reset();
      delay(100);
    }
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      timeval tv;
      gettimeofday(&tv, nullptr);
      if (tv.tv_sec > 1704067200 && hw::rtcSet(tv.tv_sec)) result = tv.tv_sec;
    }
    esp_sntp_stop();
  }
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  return result;
}

}  // namespace ntp
