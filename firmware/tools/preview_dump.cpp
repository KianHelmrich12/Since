// SPDX-License-Identifier: GPL-3.0-or-later
// Gibt Bildschirm-Modelle für Beispielszenarien aus (eine Zeile pro Feld, Tab-getrennt).
// Wird von tools/preview.py gebaut und aufgerufen.
#include <cstdio>
#include <string>

#include "since_core.h"

using namespace since;

static void dump(const char* id, const ScreenModel& m) {
  auto f = [&](const char* k, const std::string& v) { std::printf("%s\t%s\t%s\n", id, k, v.c_str()); };
  f("kind", std::to_string(static_cast<int>(m.kind)));
  f("title", m.title);
  f("subtitle", m.subtitle);
  f("headline", m.headline);
  f("line1", m.line1);
  f("line2", m.line2);
  f("banner", m.banner);
  f("qr", m.qr);
  f("battery", std::to_string(m.batteryPercent));
  f("warn", m.batteryWarning ? "1" : "0");
}

int main() {
  TimeZone tz;
  TimeZone::parse(kDefaultTz, &tz);
  Config c = defaultConfig();
  c.name = "Mila";
  c.occasion = "seit deiner Geburt";
  c.hasEvent = true;
  c.event = DateTime{2023, 3, 12, 8, 15, 0};

  ScreenInputs in;
  in.config = c;
  in.batteryMv = 3900;
  in.nowUtc = tz.toUtc(DateTime{2026, 5, 26, 9, 0, 0});
  dump("standard", buildScreen(in));

  in.nowUtc = tz.toUtc(DateTime{2026, 3, 12, 9, 0, 0});
  dump("birthday", buildScreen(in));

  in.config.layout = Layout::kMinimal;
  in.nowUtc = tz.toUtc(DateTime{2026, 5, 26, 9, 0, 0});
  dump("minimal", buildScreen(in));

  Config w = c;
  w.name = "Hochzeit Anna & Jörg";
  w.occasion = "Wir sagen Ja";
  w.event = DateTime{2027, 6, 15, 14, 30, 0};
  w.showTime = true;
  in.config = w;
  in.batteryMv = 3700;
  in.battery = BatteryLevel::kLow;
  dump("countdown", buildScreen(in));

  in.config = c;
  in.batteryMv = 3400;
  in.battery = BatteryLevel::kCritical;
  dump("battery", buildScreen(in));

  in.battery = BatteryLevel::kOk;
  in.batteryMv = 3900;
  in.rtcLostPower = true;
  dump("settime", buildScreen(in));

  in.rtcLostPower = false;
  in.setupMode = true;
  in.setup.ssid = "since-AB12";
  in.setup.password = "k7mqp2xv9a";
  in.setup.passkey = "482913";
  dump("setup", buildScreen(in));

  in.setupMode = false;
  in.config = defaultConfig();
  dump("welcome", buildScreen(in));
  return 0;
}
