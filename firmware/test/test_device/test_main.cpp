// SPDX-License-Identifier: GPL-3.0-or-later
// Akku, Zeitplan, Bildschirm-Modell und Setup-Daten
#include <unity.h>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

static TimeZone berlin() {
  TimeZone tz;
  TimeZone::parse(kDefaultTz, &tz);
  return tz;
}

static Config eventConfig(DateTime ev) {
  Config c = defaultConfig();
  c.name = "Mila";
  c.occasion = "seit deiner Geburt";
  c.hasEvent = true;
  c.event = ev;
  return c;
}

static Epoch local(int y, int mo, int d, int h, int mi, int s = 0) {
  return berlin().toUtc(DateTime{y, mo, d, h, mi, s});
}

// --- Akku
static void test_battery_percent() {
  TEST_ASSERT_EQUAL(100, batteryPercent(4250));
  TEST_ASSERT_EQUAL(100, batteryPercent(4200));
  TEST_ASSERT_EQUAL(50, batteryPercent(3840));
  TEST_ASSERT_EQUAL(0, batteryPercent(3200));
  int prev = 101;
  for (int mv = 4200; mv >= 3300; mv -= 10) {
    const int p = batteryPercent(mv);
    TEST_ASSERT_TRUE(p <= prev);  // monoton
    prev = p;
  }
}

static void test_battery_level_hysteresis() {
  TEST_ASSERT_TRUE(batteryLevel(3900, BatteryLevel::kOk) == BatteryLevel::kOk);
  TEST_ASSERT_TRUE(batteryLevel(3700, BatteryLevel::kOk) == BatteryLevel::kLow);
  TEST_ASSERT_TRUE(batteryLevel(3450, BatteryLevel::kLow) == BatteryLevel::kCritical);
  TEST_ASSERT_TRUE(batteryLevel(3600, BatteryLevel::kCritical) == BatteryLevel::kCritical);
  TEST_ASSERT_TRUE(batteryLevel(3700, BatteryLevel::kCritical) == BatteryLevel::kLow);
  TEST_ASSERT_TRUE(batteryLevel(100, BatteryLevel::kCritical) == BatteryLevel::kOk);  // kein Akku
}

// --- Zeitplan
static void test_wake_auto_next_day_boundary() {
  const Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  // 23.11.2026 07:00 → nächste Grenze 08:15 heute
  Epoch w = nextWakeUtc(c, local(2026, 11, 23, 7, 0), false);
  TEST_ASSERT_EQUAL_INT64(local(2026, 11, 23, 8, 15) + kWakeMarginS, w);
  // 09:00 → Mitternacht (Meilensteine)
  w = nextWakeUtc(c, local(2026, 11, 23, 9, 0), false);
  TEST_ASSERT_EQUAL_INT64(local(2026, 11, 24, 0, 0) + kWakeMarginS, w);
}

static void test_wake_show_time_hourly() {
  Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  c.showTime = true;
  const Epoch w = nextWakeUtc(c, local(2026, 11, 23, 10, 20), false);
  TEST_ASSERT_EQUAL_INT64(local(2026, 11, 23, 11, 15) + kWakeMarginS, w);
}

static void test_wake_interval_and_caps() {
  Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  c.updateMode = UpdateMode::kInterval;
  c.intervalMinutes = 10;
  const Epoch now = local(2026, 11, 23, 10, 0);
  TEST_ASSERT_EQUAL_INT64(now + 600, nextWakeUtc(c, now, false));
  TEST_ASSERT_EQUAL_INT64(now + kMaxSleepS, nextWakeUtc(c, now, true));
  Config none = defaultConfig();
  TEST_ASSERT_EQUAL_INT64(now + kMaxSleepS, nextWakeUtc(none, now, false));
}

static void test_wake_countdown_hits_event() {
  const Config c = eventConfig({2026, 11, 23, 23, 59, 30});
  const Epoch w = nextWakeUtc(c, local(2026, 11, 23, 23, 0), false);
  TEST_ASSERT_EQUAL_INT64(local(2026, 11, 23, 23, 59, 30) + kWakeMarginS, w);
}

static void test_wake_over_dst_change() {
  const Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  // Samstag vor der Umstellung, 09:00 → Mitternacht, dann 08:15 CEST am Sonntag
  const Epoch now = local(2026, 3, 29, 0, 10);
  TEST_ASSERT_EQUAL_INT64(local(2026, 3, 29, 8, 15) + kWakeMarginS, nextWakeUtc(c, now, false));
}

static void test_wake_event_time_in_dst_gap() {
  // Fund aus dem Review: Ereignis 02:02 Lokalzeit, Umstellung 02:00→03:00 (Neuseeland)
  TimeZone nz;
  TEST_ASSERT_TRUE(TimeZone::parse("NZST-12NZDT,M9.5.0,M4.1.0/3", &nz));
  Config c = eventConfig({2020, 1, 1, 2, 2, 0});
  c.tz = "NZST-12NZDT,M9.5.0,M4.1.0/3";
  const Epoch now = nz.toUtc(DateTime{2024, 9, 29, 1, 0, 0});
  const Epoch jump = nz.toUtc(DateTime{2024, 9, 29, 3, 0, 0});  // Zeitpunkt des Sprungs
  TEST_ASSERT_EQUAL_INT64(jump + kWakeMarginS, nextWakeUtc(c, now, false));
}

static void test_sync_due() {
  Config c = defaultConfig();
  const Epoch now = 1795000000;
  TEST_ASSERT_FALSE(isSyncDue(c, now, 0, 0, false));  // NTP aus
  c.ntp = true;
  c.wifiSsid = "Netz";
  TEST_ASSERT_TRUE(isSyncDue(c, now, 0, 0, false));
  TEST_ASSERT_FALSE(isSyncDue(c, now, now - 3600, now - 3600, false));
  TEST_ASSERT_TRUE(isSyncDue(c, now, now - kSyncIntervalS, now - kSyncIntervalS, false));
  // fehlgeschlagener Versuch vor 1 h → warten
  TEST_ASSERT_FALSE(isSyncDue(c, now, now - kSyncIntervalS - 10, now - 3600, false));
  // aber bei verlorener RTC-Zeit sofort
  TEST_ASSERT_TRUE(isSyncDue(c, now, now - kSyncIntervalS - 10, now - 3600, true));
}

// --- Bildschirm
static ScreenInputs inputs(const Config& c, Epoch now) {
  ScreenInputs in;
  in.config = c;
  in.nowUtc = now;
  in.batteryMv = 3900;
  return in;
}

static void test_screen_standard() {
  const Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  const ScreenModel m = buildScreen(inputs(c, local(2026, 5, 26, 9, 0)));
  TEST_ASSERT_TRUE(m.kind == ScreenKind::kStandard);
  TEST_ASSERT_EQUAL_STRING("Mila", m.title.c_str());
  TEST_ASSERT_EQUAL_STRING("3 Jahre · 2 Monate · 14 Tage", m.headline.c_str());
  TEST_ASSERT_EQUAL_STRING("seit 12.03.2023, 08:15", m.line1.c_str());
  TEST_ASSERT_EQUAL_STRING("1.171 Tage · 167 Wochen", m.line2.c_str());
  TEST_ASSERT_EQUAL(63, m.batteryPercent);
  TEST_ASSERT_FALSE(m.batteryWarning);
}

static void test_screen_birthday_and_minimal() {
  Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  ScreenModel m = buildScreen(inputs(c, local(2026, 3, 12, 7, 0)));
  TEST_ASSERT_TRUE(m.kind == ScreenKind::kMilestone);
  TEST_ASSERT_EQUAL_STRING("Alles Gute zum 3. Geburtstag!", m.banner.c_str());
  c.occasion = "verheiratet";
  m = buildScreen(inputs(c, local(2026, 3, 12, 9, 0)));
  TEST_ASSERT_EQUAL_STRING("Heute: 3 Jahre!", m.banner.c_str());
  c.layout = Layout::kMinimal;
  // Minimal-Layout am Jahrestag: bleibt Minimal, bekommt aber das Banner
  m = buildScreen(inputs(c, local(2026, 3, 12, 9, 0)));
  TEST_ASSERT_TRUE(m.kind == ScreenKind::kMinimal);
  TEST_ASSERT_EQUAL_STRING("Heute: 3 Jahre!", m.banner.c_str());
  m = buildScreen(inputs(c, local(2026, 3, 20, 9, 0)));
  TEST_ASSERT_TRUE(m.kind == ScreenKind::kMinimal);
  TEST_ASSERT_TRUE(m.line2.empty());
}

static void test_screen_countdown() {
  const Config c = eventConfig({2026, 12, 24, 18, 0, 0});
  const ScreenModel m = buildScreen(inputs(c, local(2026, 11, 23, 12, 0)));
  TEST_ASSERT_EQUAL_STRING("noch 1 Monat · 1 Tag", m.headline.c_str());
  TEST_ASSERT_EQUAL_STRING("bis 24.12.2026, 18:00", m.line1.c_str());
}

static void test_screen_special_states() {
  const Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  ScreenInputs in = inputs(c, local(2026, 5, 26, 9, 0));
  in.battery = BatteryLevel::kCritical;
  TEST_ASSERT_TRUE(buildScreen(in).kind == ScreenKind::kBatteryEmpty);
  in.battery = BatteryLevel::kOk;
  in.rtcLostPower = true;
  TEST_ASSERT_TRUE(buildScreen(in).kind == ScreenKind::kSetTime);
  in.setupMode = true;
  in.setup.ssid = "since-AB12";
  in.setup.password = "abc23456xy";
  const ScreenModel s = buildScreen(in);
  TEST_ASSERT_TRUE(s.kind == ScreenKind::kSetup);
  TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:since-AB12;P:abc23456xy;;", s.qr.c_str());
  TEST_ASSERT_TRUE(buildScreen(inputs(defaultConfig(), 0)).kind == ScreenKind::kWelcome);
}

static void test_screen_fingerprint() {
  const Config c = eventConfig({2023, 3, 12, 8, 15, 0});
  ScreenInputs in = inputs(c, local(2026, 5, 26, 9, 0));
  const uint32_t a = screenFingerprint(buildScreen(in));
  in.nowUtc += 3600;  // gleicher Tag, gleicher Text
  TEST_ASSERT_EQUAL_UINT32(a, screenFingerprint(buildScreen(in)));
  in.batteryMv = 3880;  // kleine Schwankung
  TEST_ASSERT_EQUAL_UINT32(a, screenFingerprint(buildScreen(in)));
  in.nowUtc += 24 * 3600;
  TEST_ASSERT_NOT_EQUAL(a, screenFingerprint(buildScreen(in)));
}

// --- Setup
static void test_device_name_and_qr() {
  const uint8_t mac[6] = {0x10, 0x20, 0x30, 0x40, 0xab, 0x0c};
  TEST_ASSERT_EQUAL_STRING("since-AB0C", deviceName(mac).c_str());
  TEST_ASSERT_EQUAL_STRING("WIFI:T:nopass;S:a\\;b;;", wifiQrPayload("a;b", "").c_str());
  TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:x;P:p\\:w\\\\;;", wifiQrPayload("x", "p:w\\").c_str());
}

static void test_random_credentials() {
  uint32_t state = 12345;
  RandomFn rnd = [&state]() { return state = state * 1664525u + 1013904223u; };
  const std::string pw = randomWifiPassword(rnd);
  TEST_ASSERT_EQUAL(10, pw.size());
  TEST_ASSERT_TRUE(pw.find_first_of("01lIO") == std::string::npos);
  uint32_t num = 0;
  const std::string pk = randomPasskey([]() { return 42u; }, &num);
  TEST_ASSERT_EQUAL_STRING("000042", pk.c_str());
  TEST_ASSERT_EQUAL_UINT32(42, num);
}

// --- Framing
static void test_framing_roundtrip() {
  std::string msg;
  for (int i = 0; i < 1000; ++i) msg += static_cast<char>('a' + i % 26);
  for (size_t fs : {2u, 20u, 243u, 244u, 2000u}) {
    const auto frames = splitFrames(msg, fs);
    Reassembler r;
    for (size_t i = 0; i < frames.size(); ++i) {
      const auto res =
          r.push(reinterpret_cast<const uint8_t*>(frames[i].data()), frames[i].size());
      TEST_ASSERT_TRUE(i + 1 == frames.size() ? res == Reassembler::Result::kComplete
                                              : res == Reassembler::Result::kIncomplete);
    }
    TEST_ASSERT_EQUAL_STRING(msg.c_str(), r.take().c_str());
  }
  const auto single = splitFrames("{}", 100);
  TEST_ASSERT_EQUAL(1, single.size());
  TEST_ASSERT_EQUAL(0x03, static_cast<uint8_t>(single[0][0]));
  TEST_ASSERT_EQUAL(1, splitFrames("", 100).size());
}

static void test_framing_errors() {
  Reassembler r;
  const uint8_t mid[] = {0x00, 'x'};
  TEST_ASSERT_TRUE(r.push(mid, 2) == Reassembler::Result::kError);  // ohne FIRST
  const uint8_t first[] = {0x01, 'a'};
  const uint8_t last[] = {0x02, 'b'};
  const uint8_t first2[] = {0x01, 'c'};
  r.push(first, 2);
  r.push(first2, 2);  // FIRST verwirft
  TEST_ASSERT_TRUE(r.push(last, 2) == Reassembler::Result::kComplete);
  TEST_ASSERT_EQUAL_STRING("cb", r.take().c_str());
  std::string big(kMaxMessageBytes + 10, 'x');
  const auto frames = splitFrames(big, 244);
  Reassembler r2;
  Reassembler::Result res = Reassembler::Result::kIncomplete;
  for (const auto& f : frames) {
    res = r2.push(reinterpret_cast<const uint8_t*>(f.data()), f.size());
    if (res == Reassembler::Result::kError) break;
  }
  TEST_ASSERT_TRUE(res == Reassembler::Result::kError);
  TEST_ASSERT_TRUE(r2.tooLarge());
}

// --- Weck-Zyklus (gemeinsam mit dem Emulator)
static void test_cycle_draws_only_on_change() {
  CycleInputs ci;
  ci.config = eventConfig({2023, 3, 12, 8, 15, 0});
  ci.nowUtc = local(2026, 5, 26, 9, 0);
  ci.batteryMv = 3900;
  CycleResult r = runCycle(ci);
  TEST_ASSERT_TRUE(r.draw);
  ci.lastFingerprint = r.fingerprint;
  ci.nowUtc = r.nextWakeUtc - 3600;  // gleicher Text
  r = runCycle(ci);
  TEST_ASSERT_FALSE(r.draw);
  ci.forceRedraw = true;
  TEST_ASSERT_TRUE(runCycle(ci).draw);
}

static void test_cycle_brownout_and_lost_power() {
  CycleInputs ci;
  ci.config = eventConfig({2023, 3, 12, 8, 15, 0});
  ci.nowUtc = local(2026, 5, 26, 9, 0);
  ci.batteryMv = 3900;
  ci.brownout = true;
  CycleResult r = runCycle(ci);
  TEST_ASSERT_FALSE(r.draw);
  TEST_ASSERT_TRUE(r.level == BatteryLevel::kCritical);
  TEST_ASSERT_EQUAL_INT64(ci.nowUtc + kMaxSleepS, r.nextWakeUtc);
  ci.brownout = false;
  ci.rtcLostPower = true;
  r = runCycle(ci);
  TEST_ASSERT_TRUE(r.model.kind == ScreenKind::kSetTime);
  TEST_ASSERT_EQUAL_INT64(ci.nowUtc + kMaxSleepS, r.nextWakeUtc);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_battery_percent);
  RUN_TEST(test_battery_level_hysteresis);
  RUN_TEST(test_wake_auto_next_day_boundary);
  RUN_TEST(test_wake_show_time_hourly);
  RUN_TEST(test_wake_interval_and_caps);
  RUN_TEST(test_wake_countdown_hits_event);
  RUN_TEST(test_wake_over_dst_change);
  RUN_TEST(test_wake_event_time_in_dst_gap);
  RUN_TEST(test_sync_due);
  RUN_TEST(test_screen_standard);
  RUN_TEST(test_screen_birthday_and_minimal);
  RUN_TEST(test_screen_countdown);
  RUN_TEST(test_screen_special_states);
  RUN_TEST(test_screen_fingerprint);
  RUN_TEST(test_device_name_and_qr);
  RUN_TEST(test_random_credentials);
  RUN_TEST(test_framing_roundtrip);
  RUN_TEST(test_framing_errors);
  RUN_TEST(test_cycle_draws_only_on_change);
  RUN_TEST(test_cycle_brownout_and_lost_power);
  return UNITY_END();
}
