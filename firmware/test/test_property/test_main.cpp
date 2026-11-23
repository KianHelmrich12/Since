// SPDX-License-Identifier: GPL-3.0-or-later
// Eigenschafts- und Fuzz-Tests: viele zufällige (deterministische) Eingaben, geprüft werden
// Invarianten statt Einzelwerte.
#include <unity.h>

#include <cstdint>
#include <string>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

namespace {

struct Rng {
  uint64_t s;
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return static_cast<uint32_t>(s >> 11);
  }
  int range(int lo, int hi) { return lo + static_cast<int>(next() % static_cast<uint32_t>(hi - lo + 1)); }
};

DateTime randomDateTime(Rng& r) {
  DateTime d{r.range(1990, 2060), r.range(1, 12), 1, r.range(0, 23), r.range(0, 59), 0};
  d.day = r.range(1, daysInMonth(d.year, d.month));
  return d;
}

TimeZone berlin() {
  TimeZone tz;
  TimeZone::parse(kDefaultTz, &tz);
  return tz;
}

}  // namespace

// Elapsed rekonstruiert „jetzt“: Ereignis + Jahre/Monate (begrenzt) + Tage ergibt das lokale
// Datum von jetzt bzw. den Vortag (wenn die Uhrzeit des Ereignisses noch nicht erreicht ist).
static void test_elapsed_invariants() {
  Rng r{0x9E3779B97F4A7C15ull};
  const TimeZone tz = berlin();
  for (int i = 0; i < 20000; ++i) {
    const DateTime ev = randomDateTime(r);
    const DateTime nowL = randomDateTime(r);
    const Epoch now = tz.toUtc(nowL);
    const Elapsed e = elapsedSince(ev, now, tz);

    TEST_ASSERT_TRUE(e.months >= 0 && e.months <= 11);
    TEST_ASSERT_TRUE(e.days >= 0 && e.days <= 30);
    TEST_ASSERT_TRUE(e.hours >= 0 && e.hours <= 24);  // 24 nur am Tag der Rückstellung
    TEST_ASSERT_TRUE(e.minutes >= 0 && e.minutes <= 59);
    TEST_ASSERT_TRUE(e.years >= 0);
    TEST_ASSERT_EQUAL_INT64(e.years * 12 + e.months, e.totalMonths);
    TEST_ASSERT_EQUAL_INT64(e.totalDays / 7, e.totalWeeks);
    TEST_ASSERT_EQUAL(tz.toUtc(ev) > now, e.future);

    if (!e.future) {
      // Anker = Ereignisdatum + Monate (begrenzt) + Tage liegt höchstens 1 Tag vor heute
      const Date anchor = addMonthsClamped(tz.toLocal(tz.toUtc(ev)).date(),
                                           static_cast<int>(e.totalMonths));
      const int64_t gap = daysFromCivil(tz.toLocal(now).date()) - (daysFromCivil(anchor) + e.days);
      TEST_ASSERT_TRUE_MESSAGE(gap == 0 || gap == 1, "anchor mismatch");
      // Gesamttage stimmen grob mit der echten Zeit überein (±1 Tag wegen DST/Uhrzeit)
      const int64_t realDays = e.totalSeconds / 86400;
      TEST_ASSERT_TRUE(e.totalDays >= realDays - 1 && e.totalDays <= realDays + 1);
    }
  }
}

// Monotonie: Mit fortschreitender Zeit nimmt die Gesamtzahl der Tage nie ab.
static void test_elapsed_monotonic() {
  const TimeZone tz = berlin();
  const DateTime ev{2024, 2, 29, 23, 30, 0};
  Epoch t = tz.toUtc(ev);
  int64_t lastDays = 0, lastMonths = 0;
  for (int i = 0; i < 3 * 366 * 6; ++i) {  // 3 Jahre in 4-h-Schritten
    t += 4 * 3600;
    const Elapsed e = elapsedSince(ev, t, tz);
    TEST_ASSERT_TRUE(e.totalDays >= lastDays);
    TEST_ASSERT_TRUE(e.totalMonths >= lastMonths);
    lastDays = e.totalDays;
    lastMonths = e.totalMonths;
  }
}

// Der Weckzeitpunkt liegt immer in der Zukunft und höchstens 24 h + Marge entfernt.
static void test_wake_bounds() {
  Rng r{12345};
  const TimeZone tz = berlin();
  for (int i = 0; i < 5000; ++i) {
    Config c = defaultConfig();
    c.hasEvent = true;
    c.event = randomDateTime(r);
    c.showTime = (r.next() & 1) != 0;
    if ((r.next() & 3) == 0) {
      c.updateMode = UpdateMode::kInterval;
      c.intervalMinutes = r.range(10, 1440);
    }
    const Epoch now = tz.toUtc(randomDateTime(r));
    const Epoch w = nextWakeUtc(c, now, false);
    TEST_ASSERT_TRUE(w > now);
    TEST_ASSERT_TRUE(w <= now + kMaxSleepS + kWakeMarginS);
  }
}

// Zwischen zwei Weckzeitpunkten ändert sich der angezeigte Text nicht (auto-Modus):
// Stichprobe kurz vor dem Weckzeitpunkt ergibt denselben Fingerprint wie jetzt.
static void test_wake_not_too_late() {
  Rng r{777};
  const TimeZone tz = berlin();
  for (int i = 0; i < 3000; ++i) {
    Config c = defaultConfig();
    c.name = "X";
    c.hasEvent = true;
    c.event = randomDateTime(r);
    c.showTime = (r.next() & 1) != 0;
    const Epoch now = tz.toUtc(randomDateTime(r));
    const Epoch w = nextWakeUtc(c, now, false);
    ScreenInputs a;
    a.config = c;
    a.nowUtc = now + 1;
    ScreenInputs b = a;
    b.nowUtc = w - kWakeMarginS - 1;
    if (b.nowUtc <= a.nowUtc) continue;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(screenFingerprint(buildScreen(a)),
                                     screenFingerprint(buildScreen(b)),
                                     "Text ändert sich vor dem nächsten Weckzeitpunkt");
  }
}

// Fuzz: zufällige Bytes und verstümmeltes JSON dürfen nie abstürzen und die Config nicht ändern.
static void test_config_fuzz() {
  Rng r{4242};
  const std::string base =
      "{\"schemaVersion\":1,\"name\":\"Mila\",\"event\":\"2023-03-12T08:15\",\"tz\":\"CET-1CEST,"
      "M3.5.0,M10.5.0/3\",\"update\":{\"mode\":\"auto\",\"intervalMinutes\":60},\"wifi\":"
      "{\"ssid\":\"N\",\"password\":\"12345678\"},\"ntp\":true}";
  for (int i = 0; i < 20000; ++i) {
    std::string s = base;
    const int edits = r.range(1, 6);
    for (int k = 0; k < edits; ++k) {
      const size_t pos = r.next() % s.size();
      switch (r.next() % 3) {
        case 0: s[pos] = static_cast<char>(r.next() & 0xFF); break;
        case 1: s.erase(pos, 1); break;
        default: s.insert(pos, 1, static_cast<char>(r.next() & 0xFF)); break;
      }
      if (s.empty()) s = "{";
    }
    Config c = defaultConfig();
    const std::string before = toJson(c, true);
    const ConfigResult res = applyJson(s, &c);
    if (!res.ok()) {
      TEST_ASSERT_EQUAL_STRING(before.c_str(), toJson(c, true).c_str());
    } else {
      TEST_ASSERT_TRUE(validate(c).empty());
      Config again = defaultConfig();
      TEST_ASSERT_TRUE(applyJson(toJson(c, true), &again).ok());  // Round-Trip bleibt gültig
    }
  }
}

// Fuzz: beliebige Frames bringen den Reassembler nie über das Limit und RPC antwortet immer JSON.
static void test_framing_and_rpc_fuzz() {
  class Dev : public DeviceHooks {
   public:
    Config cfg = defaultConfig();
    Config loadConfig() override { return cfg; }
    bool saveConfig(const Config& c) override {
      cfg = c;
      return true;
    }
    bool setTime(Epoch) override { return true; }
    DeviceStatus status() override { return DeviceStatus(); }
    void requestRedraw() override {}
    void requestExitSetup() override {}
  } dev;
  Rng r{99};
  Reassembler re;
  for (int i = 0; i < 20000; ++i) {
    uint8_t buf[64];
    const size_t n = 1 + r.next() % sizeof buf;
    for (size_t k = 0; k < n; ++k) buf[k] = static_cast<uint8_t>(r.next());
    if (re.push(buf, n) == Reassembler::Result::kComplete) {
      const std::string msg = re.take();
      TEST_ASSERT_TRUE(msg.size() <= kMaxMessageBytes);
      const std::string resp = handleRpc(dev, msg);
      TEST_ASSERT_TRUE(!resp.empty() && resp.front() == '{' && resp.back() == '}');
    }
  }
  TEST_ASSERT_TRUE(validate(dev.cfg).empty());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_elapsed_invariants);
  RUN_TEST(test_elapsed_monotonic);
  RUN_TEST(test_wake_bounds);
  RUN_TEST(test_wake_not_too_late);
  RUN_TEST(test_config_fuzz);
  RUN_TEST(test_framing_and_rpc_fuzz);
  return UNITY_END();
}
