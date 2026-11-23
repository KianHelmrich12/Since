// SPDX-License-Identifier: GPL-3.0-or-later
// Geräte-Emulator: spielt die Weck-Zyklen der Firmware im Zeitraffer durch.
// Nutzt dieselbe Logik wie src/main.cpp (since::runCycle); simuliert werden RTC, Akku
// (Ladung → Spannung), Tasten und der Energieverbrauch. Gebaut/gestartet von tools/sim.py.
//
// Ausgabe (stdout, Tab-getrennt):
//   wake  <utc> <local> <mv> <level> <draw> <reason>
//   frame <n> <key> <value>          (Bildschirm-Modell jeder Neuzeichnung, bis --frames)
//   summary <key> <value>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "since_core.h"

using namespace since;

namespace {

struct Params {
  std::string configPath;
  DateTime start{2026, 1, 1, 12, 0, 0};
  double days = 30;
  bool untilEmpty = false;
  double capacityMah = 2000;
  double startPercent = 100;
  double sleepUa = 20;           // Ruhestrom gesamt (Ziel < 50 µA, MANUELL messen)
  double wakeMs = 300;           // Booten, RTC, ADC ohne Zeichnen
  double wakeMa = 40;
  double drawMs = 4500;          // Vollbild-Refresh 7,5″ UC8179
  double drawMa = 35;
  double selfDischargePctMonth = 2;  // LiPo-Selbstentladung
  int frames = 12;
  std::map<Epoch, std::string> presses;  // UTC → key1/key2
};

// Umkehrung von batteryPercent(): Prozent → Ruhespannung (Bisektion über die Kennlinie)
int millivoltsForPercent(double pct) {
  int lo = 3000, hi = 4200;
  while (lo < hi) {
    const int mid = (lo + hi) / 2;
    if (batteryPercent(mid) < pct) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

std::string iso(const DateTime& d) { return formatIsoLocal(d); }

void dumpFrame(int n, const ScreenModel& m) {
  auto f = [&](const char* k, const std::string& v) {
    std::printf("frame\t%d\t%s\t%s\n", n, k, v.c_str());
  };
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

[[noreturn]] void usage(const char* msg) {
  std::fprintf(stderr, "sim: %s\n", msg);
  std::exit(2);
}

}  // namespace

int main(int argc, char** argv) {
  Params p;
  std::vector<std::pair<DateTime, std::string>> pressLocal;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&]() -> std::string {
      if (i + 1 >= argc) usage(("Wert fehlt für " + a).c_str());
      return argv[++i];
    };
    if (a == "--config") p.configPath = val();
    else if (a == "--start") {
      if (!parseIsoLocal(val(), &p.start)) usage("--start erwartet YYYY-MM-DDTHH:MM");
    } else if (a == "--days") p.days = std::atof(val().c_str());
    else if (a == "--until-empty") p.untilEmpty = true;
    else if (a == "--capacity-mah") p.capacityMah = std::atof(val().c_str());
    else if (a == "--start-percent") p.startPercent = std::atof(val().c_str());
    else if (a == "--sleep-ua") p.sleepUa = std::atof(val().c_str());
    else if (a == "--draw-ms") p.drawMs = std::atof(val().c_str());
    else if (a == "--draw-ma") p.drawMa = std::atof(val().c_str());
    else if (a == "--frames") p.frames = std::atoi(val().c_str());
    else if (a == "--press") {  // 2026-03-12T10:00=key1
      const std::string v = val();
      const size_t eq = v.find('=');
      DateTime t{};
      if (eq == std::string::npos || !parseIsoLocal(v.substr(0, eq), &t)) usage("--press ZEIT=key1|key2");
      pressLocal.push_back({t, v.substr(eq + 1)});
    } else usage(("unbekannte Option " + a).c_str());
  }

  Config cfg = defaultConfig();
  if (!p.configPath.empty()) {
    std::ifstream f(p.configPath);
    std::stringstream ss;
    ss << f.rdbuf();
    const ConfigResult r = applyJson(ss.str(), &cfg);
    if (!r.ok()) usage("Config ungültig");
  }
  TimeZone tz;
  if (!TimeZone::parse(cfg.tz, &tz)) tz = TimeZone::utc();
  for (const auto& pr : pressLocal) p.presses[tz.toUtc(pr.first)] = pr.second;

  const double capacityMas = p.capacityMah * 3600.0;  // mA·s
  double chargeMas = capacityMas * p.startPercent / 100.0;
  const double selfPerS = capacityMas * p.selfDischargePctMonth / 100.0 / (30.0 * 86400.0);

  const Epoch t0 = tz.toUtc(p.start);
  const Epoch tEnd = p.untilEmpty ? t0 + 20LL * 365 * 86400 : t0 + static_cast<Epoch>(p.days * 86400);
  Epoch now = t0;
  uint32_t lastFp = 0;
  BatteryLevel level = BatteryLevel::kOk;
  bool force = true;  // Einschalten
  std::string reason = "power_on";
  long wakes = 0, draws = 0;
  int frame = 0;
  double emptyAtDays = -1;
  double sleepMas = 0, wakeMas = 0, drawMas = 0, selfMas = 0;

  while (now < tEnd) {
    const int mv = millivoltsForPercent(100.0 * chargeMas / capacityMas);
    if (reason == "key2") cfg.layout = cfg.layout == Layout::kStandard ? Layout::kMinimal : Layout::kStandard;
    if (reason == "key1") force = true;

    CycleInputs ci;
    ci.config = cfg;
    ci.nowUtc = now;
    ci.batteryMv = mv;
    ci.previousLevel = level;
    ci.forceRedraw = force;
    ci.lastFingerprint = lastFp;
    const CycleResult r = runCycle(ci);
    level = r.level;
    lastFp = r.fingerprint;
    ++wakes;

    double used = p.wakeMs / 1000.0 * p.wakeMa;
    wakeMas += used;
    if (r.draw) {
      ++draws;
      drawMas += p.drawMs / 1000.0 * p.drawMa;
      used += p.drawMs / 1000.0 * p.drawMa;
      if (frame < p.frames) dumpFrame(frame++, r.model);
    }
    std::printf("wake\t%lld\t%s\t%d\t%d\t%d\t%s\n", static_cast<long long>(now),
                iso(tz.toLocal(now)).c_str(), mv, static_cast<int>(level), r.draw ? 1 : 0, reason.c_str());
    if (level == BatteryLevel::kCritical && emptyAtDays < 0) {
      emptyAtDays = static_cast<double>(now - t0) / 86400.0;
      if (p.untilEmpty) break;
    }

    // Schlafen bis zum Alarm oder bis zu einem früheren Tastendruck
    Epoch next = r.nextWakeUtc;
    reason = "rtc_alarm";
    auto it = p.presses.upper_bound(now);
    if (it != p.presses.end() && it->first < next) {
      next = it->first;
      reason = it->second;
    }
    const double dt = static_cast<double>(next - now);
    const double s = p.sleepUa / 1000.0 * dt;
    const double sd = selfPerS * dt;
    sleepMas += s;
    selfMas += sd;
    chargeMas -= used + s + sd;
    if (chargeMas < 0) chargeMas = 0;
    force = false;
    now = next;
  }

  const double simDays = static_cast<double>(now - t0) / 86400.0;
  const double perDayMah = (sleepMas + wakeMas + drawMas + selfMas) / 3600.0 / (simDays > 0 ? simDays : 1);
  auto sum = [](const char* k, const std::string& v) { std::printf("summary\t%s\t%s\n", k, v.c_str()); };
  char b[64];
  std::snprintf(b, sizeof b, "%.1f", simDays); sum("days", b);
  sum("wakes", std::to_string(wakes));
  sum("draws", std::to_string(draws));
  std::snprintf(b, sizeof b, "%.2f", wakes / (simDays > 0 ? simDays : 1)); sum("wakes_per_day", b);
  std::snprintf(b, sizeof b, "%.3f", perDayMah); sum("mah_per_day", b);
  std::snprintf(b, sizeof b, "%.3f", sleepMas / 3600.0 / (simDays > 0 ? simDays : 1)); sum("sleep_mah_per_day", b);
  std::snprintf(b, sizeof b, "%.3f", (wakeMas + drawMas) / 3600.0 / (simDays > 0 ? simDays : 1)); sum("active_mah_per_day", b);
  std::snprintf(b, sizeof b, "%.3f", selfMas / 3600.0 / (simDays > 0 ? simDays : 1)); sum("selfdischarge_mah_per_day", b);
  std::snprintf(b, sizeof b, "%.1f", 100.0 * chargeMas / capacityMas); sum("battery_percent_end", b);
  std::snprintf(b, sizeof b, "%.0f", emptyAtDays); sum("empty_after_days", emptyAtDays < 0 ? "-" : b);
  // Hochrechnung bis „Bitte laden“ (~3 %), linear aus dem Verbrauch
  std::snprintf(b, sizeof b, "%.0f", capacityMas * (p.startPercent - 3) / 100.0 / 3600.0 / perDayMah);
  sum("estimated_runtime_days", b);
  return 0;
}
