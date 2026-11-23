// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

static Epoch utc(int y, int mo, int d, int h, int mi, int s = 0) {
  return toSeconds(DateTime{y, mo, d, h, mi, s});
}

static TimeZone berlin() {
  TimeZone tz;
  TEST_ASSERT_TRUE(TimeZone::parse(kDefaultTz, &tz));
  return tz;
}

static void test_parse_valid() {
  TimeZone tz;
  TEST_ASSERT_TRUE(TimeZone::parse("UTC0", &tz));
  TEST_ASSERT_FALSE(tz.hasDst());
  TEST_ASSERT_TRUE(TimeZone::parse("CET-1CEST,M3.5.0,M10.5.0/3", &tz));
  TEST_ASSERT_TRUE(tz.hasDst());
  TEST_ASSERT_TRUE(TimeZone::parse("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00", &tz));
  TEST_ASSERT_TRUE(TimeZone::parse("<+0530>-5:30", &tz));
  TEST_ASSERT_TRUE(TimeZone::parse("EST5EDT", &tz));  // Default-Regeln
  TEST_ASSERT_TRUE(TimeZone::parse("XXX3YYY,J60/0,300", &tz));
}

static void test_parse_invalid() {
  TimeZone tz;
  TEST_ASSERT_FALSE(TimeZone::parse("", &tz));
  TEST_ASSERT_FALSE(TimeZone::parse("CET", &tz));
  TEST_ASSERT_FALSE(TimeZone::parse("C-1", &tz));
  TEST_ASSERT_FALSE(TimeZone::parse("CET-1CEST,M13.5.0,M10.5.0", &tz));
  TEST_ASSERT_FALSE(TimeZone::parse("CET-1CEST,M3.5.0", &tz));
  TEST_ASSERT_FALSE(TimeZone::parse("CET-1CEST,M3.5.0,M10.5.0/3x", &tz));
  TEST_ASSERT_FALSE(TimeZone::parse("Europe/Berlin", &tz));
}

static void test_berlin_offsets() {
  const TimeZone tz = berlin();
  TEST_ASSERT_EQUAL(3600, tz.offsetAt(utc(2026, 1, 15, 12, 0)));
  TEST_ASSERT_EQUAL(7200, tz.offsetAt(utc(2026, 7, 15, 12, 0)));
}

static void test_berlin_transition_instants() {
  const TimeZone tz = berlin();
  // 29.03.2026 01:00 UTC: Beginn Sommerzeit
  TEST_ASSERT_FALSE(tz.isDstAt(utc(2026, 3, 29, 0, 59, 59)));
  TEST_ASSERT_TRUE(tz.isDstAt(utc(2026, 3, 29, 1, 0, 0)));
  // 25.10.2026 01:00 UTC: Ende Sommerzeit
  TEST_ASSERT_TRUE(tz.isDstAt(utc(2026, 10, 25, 0, 59, 59)));
  TEST_ASSERT_FALSE(tz.isDstAt(utc(2026, 10, 25, 1, 0, 0)));
}

static void test_local_roundtrip() {
  const TimeZone tz = berlin();
  const DateTime local{2026, 3, 12, 8, 15, 0};
  const Epoch u = tz.toUtc(local);
  TEST_ASSERT_EQUAL_INT64(utc(2026, 3, 12, 7, 15), u);
  TEST_ASSERT_EQUAL(0, compare(local, tz.toLocal(u)));
}

static void test_dst_gap_moves_forward() {
  const TimeZone tz = berlin();
  // 02:30 existiert am 29.03.2026 nicht → 03:30 CEST = 01:30 UTC
  TEST_ASSERT_EQUAL_INT64(utc(2026, 3, 29, 1, 30), tz.toUtc(DateTime{2026, 3, 29, 2, 30, 0}));
}

static void test_dst_overlap_takes_first() {
  const TimeZone tz = berlin();
  // 02:30 gibt es am 25.10.2026 zweimal; erste (CEST) = 00:30 UTC
  TEST_ASSERT_EQUAL_INT64(utc(2026, 10, 25, 0, 30), tz.toUtc(DateTime{2026, 10, 25, 2, 30, 0}));
}

static void test_us_and_southern_hemisphere() {
  TimeZone ny;
  TEST_ASSERT_TRUE(TimeZone::parse("EST5EDT,M3.2.0,M11.1.0", &ny));
  TEST_ASSERT_EQUAL(-5 * 3600, ny.offsetAt(utc(2026, 1, 10, 12, 0)));
  TEST_ASSERT_EQUAL(-4 * 3600, ny.offsetAt(utc(2026, 3, 8, 7, 0)));  // 02:00 EST
  TimeZone syd;
  TEST_ASSERT_TRUE(TimeZone::parse("AEST-10AEDT,M10.1.0,M4.1.0/3", &syd));
  TEST_ASSERT_EQUAL(11 * 3600, syd.offsetAt(utc(2026, 1, 10, 0, 0)));
  TEST_ASSERT_EQUAL(10 * 3600, syd.offsetAt(utc(2026, 7, 10, 0, 0)));
}

static void test_fixed_offset_with_minutes() {
  TimeZone tz;
  TEST_ASSERT_TRUE(TimeZone::parse("<+0530>-5:30", &tz));
  TEST_ASSERT_EQUAL(5 * 3600 + 1800, tz.offsetAt(0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_valid);
  RUN_TEST(test_parse_invalid);
  RUN_TEST(test_berlin_offsets);
  RUN_TEST(test_berlin_transition_instants);
  RUN_TEST(test_local_roundtrip);
  RUN_TEST(test_dst_gap_moves_forward);
  RUN_TEST(test_dst_overlap_takes_first);
  RUN_TEST(test_us_and_southern_hemisphere);
  RUN_TEST(test_fixed_offset_with_minutes);
  return UNITY_END();
}
