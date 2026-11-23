// SPDX-License-Identifier: GPL-3.0-or-later
#include "screen.h"

#include <cctype>

#include "elapsed.h"
#include "format_de.h"
#include "milestones.h"
#include "setup.h"
#include "tz.h"

namespace since {
namespace {

bool containsBirth(const std::string& s) {
  std::string lower;
  for (char c : s) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return lower.find("geburt") != std::string::npos;
}

std::string milestoneBanner(const Milestone& m, const Config& cfg) {
  switch (m.kind) {
    case MilestoneKind::kAnniversary:
      if (containsBirth(cfg.occasion) || cfg.occasion.empty()) {
        return "Alles Gute zum " + std::to_string(m.value) + ". Geburtstag!";
      }
      return "Heute: " + formatQuantity(m.value, Unit::kYear) + "!";
    case MilestoneKind::kDayCount:
      return "Heute ist Tag " + formatThousands(m.value) + "!";
    case MilestoneKind::kMonthDay:
      return "Heute: " + formatQuantity(m.value, Unit::kMonth) + "!";
    case MilestoneKind::kNone:
    default:
      return "";
  }
}

}  // namespace

ScreenModel buildScreen(const ScreenInputs& in) {
  ScreenModel m;
  const Config& cfg = in.config;
  const bool hasBattery = in.batteryMv >= kBatteryAbsentMv;
  if (hasBattery) {
    m.batteryPercent = batteryPercent(in.batteryMv);
    m.batteryWarning = in.battery != BatteryLevel::kOk;
  }

  if (in.setupMode) {
    m.kind = ScreenKind::kSetup;
    m.title = "Einrichtung";
    m.subtitle = "WLAN: " + in.setup.ssid;
    m.headline = in.setup.password.empty() ? "ohne Passwort" : "Passwort: " + in.setup.password;
    m.line1 = "Dann im Browser: " + in.setup.url;
    m.line2 = in.setup.passkey.empty() ? "" : "App-Passkey: " + in.setup.passkey;
    m.qr = wifiQrPayload(in.setup.ssid, in.setup.password);
    return m;
  }
  if (in.battery == BatteryLevel::kCritical) {
    m.kind = ScreenKind::kBatteryEmpty;
    m.title = "Bitte laden";
    m.subtitle = "Akku fast leer";
    m.line1 = "USB-C anschließen";
    return m;
  }
  if (in.rtcLostPower) {
    m.kind = ScreenKind::kSetTime;
    m.title = "Uhrzeit einstellen";
    m.subtitle = "Die Uhr hat ihre Zeit verloren.";
    m.line1 = "Taste 1 drücken und im Portal";
    m.line2 = "»Uhrzeit vom Handy übernehmen« wählen.";
    return m;
  }
  if (!cfg.hasEvent) {
    m.kind = ScreenKind::kWelcome;
    m.title = "since";
    m.subtitle = "Willkommen!";
    m.line1 = "Zum Einrichten Taste 1 drücken.";
    return m;
  }

  TimeZone tz;
  if (!TimeZone::parse(cfg.tz, &tz)) tz = TimeZone::utc();
  const Elapsed e = elapsedSince(cfg.event, in.nowUtc, tz);

  FormatOptions opt;
  opt.hours = cfg.showTime;
  opt.maxParts = cfg.showTime ? 4 : 3;
  const std::string span = formatElapsed(e, opt);

  m.title = cfg.name;
  m.subtitle = cfg.occasion;
  m.headline = e.future ? "noch " + span : span;
  m.line1 = std::string(e.future ? "bis " : "seit ") + formatDateTime(cfg.event);
  if (!e.future) {
    m.line2 = formatQuantity(e.totalDays, Unit::kDay) + " · " +
              formatQuantity(e.totalWeeks, Unit::kWeek);
  } else {
    m.line2 = "noch " + formatQuantity(e.totalDays, Unit::kDay);
  }

  if (cfg.layout == Layout::kMinimal) {
    m.kind = ScreenKind::kMinimal;
    m.subtitle.clear();
    m.line2.clear();
  } else {
    m.kind = ScreenKind::kStandard;
  }

  if (!e.future) {
    const Date today = tz.toLocal(in.nowUtc).date();
    Milestone ms = milestoneOn(cfg.event.date(), today);
    // Monatstage nur in den ersten zwei Jahren (Babyzeit), sonst zu häufig
    if (ms.kind == MilestoneKind::kMonthDay && ms.value >= 24) ms = Milestone();
    if (ms.kind != MilestoneKind::kNone) {
      m.kind = ScreenKind::kMilestone;
      m.banner = milestoneBanner(ms, cfg);
    }
  }
  return m;
}

uint32_t screenFingerprint(const ScreenModel& m) {
  uint32_t h = 2166136261u;
  auto mix = [&h](const std::string& s) {
    for (unsigned char c : s) {
      h ^= c;
      h *= 16777619u;
    }
    h ^= 0xFF;  // Trenner
    h *= 16777619u;
  };
  mix(std::to_string(static_cast<int>(m.kind)));
  mix(m.title);
  mix(m.subtitle);
  mix(m.headline);
  mix(m.line1);
  mix(m.line2);
  mix(m.banner);
  mix(m.qr);
  mix(std::to_string(m.batteryPercent < 0 ? -1 : (m.batteryPercent + 10) / 20));
  mix(m.batteryWarning ? "w" : "");
  return h;
}

}  // namespace since
