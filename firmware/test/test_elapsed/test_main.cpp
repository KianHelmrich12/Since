// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include "since_core.h"

using namespace since;

static TimeZone tz;

void setUp() { TimeZone::parse(kDefaultTz, &tz); }
void tearDown() {}

// „jetzt“ als lokale Berliner Zeit angeben
static Elapsed at(const DateTime& event, const DateTime& nowLocal) {
  return elapsedSince(event, tz.toUtc(nowLocal), tz);
}

static void assertYmd(const Elapsed& e, int y, int m, int d) {
  TEST_ASSERT_EQUAL_MESSAGE(y, e.years, "years");
  TEST_ASSERT_EQUAL_MESSAGE(m, e.months, "months");
  TEST_ASSERT_EQUAL_MESSAGE(d, e.days, "days");
}

static void test_basic_example() {
  const Elapsed e = at({2023, 3, 12, 8, 15, 0}, {2026, 5, 26, 9, 0, 0});
  TEST_ASSERT_FALSE(e.future);
  assertYmd(e, 3, 2, 14);
  TEST_ASSERT_EQUAL(0, e.hours);
  TEST_ASSERT_EQUAL(45, e.minutes);
}

static void test_same_moment_is_zero() {
  const Elapsed e = at({2026, 1, 1, 0, 0, 0}, {2026, 1, 1, 0, 0, 0});
  assertYmd(e, 0, 0, 0);
  TEST_ASSERT_EQUAL_INT64(0, e.totalDays);
  TEST_ASSERT_EQUAL_INT64(0, e.totalSeconds);
}

static void test_before_time_of_day_not_full_day() {
  // Am 13.03. um 08:00 ist der erste Tag (ab 08:15) noch nicht voll.
  const Elapsed e = at({2026, 3, 12, 8, 15, 0}, {2026, 3, 13, 8, 0, 0});
  assertYmd(e, 0, 0, 0);
  TEST_ASSERT_EQUAL(23, e.hours);
  TEST_ASSERT_EQUAL(45, e.minutes);
  TEST_ASSERT_EQUAL_INT64(0, e.totalDays);
}

static void test_birthday_exact_time() {
  const Elapsed before = at({2025, 6, 1, 8, 15, 0}, {2026, 6, 1, 8, 14, 0});
  assertYmd(before, 0, 11, 30);
  const Elapsed after = at({2025, 6, 1, 8, 15, 0}, {2026, 6, 1, 8, 15, 0});
  assertYmd(after, 1, 0, 0);
}

static void test_month_end_jan31() {
  // 31.01. + 1 Monat → 28.02.
  assertYmd(at({2026, 1, 31, 12, 0, 0}, {2026, 2, 28, 12, 0, 0}), 0, 1, 0);
  assertYmd(at({2026, 1, 31, 12, 0, 0}, {2026, 2, 27, 12, 0, 0}), 0, 0, 27);
  assertYmd(at({2026, 1, 31, 12, 0, 0}, {2026, 3, 1, 12, 0, 0}), 0, 1, 1);
  assertYmd(at({2026, 1, 31, 12, 0, 0}, {2026, 3, 31, 12, 0, 0}), 0, 2, 0);
}

static void test_month_end_leap_year() {
  assertYmd(at({2024, 1, 31, 12, 0, 0}, {2024, 2, 29, 12, 0, 0}), 0, 1, 0);
}

static void test_leap_day_birth() {
  const DateTime birth{2024, 2, 29, 10, 0, 0};
  // Nicht-Schaltjahr: Geburtstag am 28.02.
  assertYmd(at(birth, {2025, 2, 27, 12, 0, 0}), 0, 11, 29);
  assertYmd(at(birth, {2025, 2, 28, 12, 0, 0}), 1, 0, 0);
  assertYmd(at(birth, {2025, 3, 1, 12, 0, 0}), 1, 0, 1);
  // Schaltjahr: wieder am 29.02.
  assertYmd(at(birth, {2028, 2, 28, 12, 0, 0}), 3, 11, 30);
  assertYmd(at(birth, {2028, 2, 29, 12, 0, 0}), 4, 0, 0);
}

static void test_total_days_and_weeks() {
  const Elapsed e = at({2026, 1, 1, 0, 0, 0}, {2026, 12, 31, 23, 0, 0});
  TEST_ASSERT_EQUAL_INT64(364, e.totalDays);
  TEST_ASSERT_EQUAL_INT64(52, e.totalWeeks);
  TEST_ASSERT_EQUAL_INT64(11, e.totalMonths);
  const Elapsed big = at({2000, 1, 1, 0, 0, 0}, {2026, 1, 1, 0, 0, 0});
  TEST_ASSERT_EQUAL_INT64(9497, big.totalDays);
  TEST_ASSERT_EQUAL(26, big.years);
}

static void test_dst_spring_forward_real_hours() {
  // Nacht der Zeitumstellung (29.03.2026): 00:00 → 12:00 lokal sind nur 11 h
  const Elapsed e = at({2026, 3, 29, 0, 0, 0}, {2026, 3, 29, 12, 0, 0});
  assertYmd(e, 0, 0, 0);
  TEST_ASSERT_EQUAL(11, e.hours);
  TEST_ASSERT_EQUAL_INT64(11 * 3600, e.totalSeconds);
}

static void test_dst_fall_back_real_hours() {
  // 25.10.2026: 00:00 → 12:00 lokal sind 13 h
  const Elapsed e = at({2026, 10, 25, 0, 0, 0}, {2026, 10, 25, 12, 0, 0});
  TEST_ASSERT_EQUAL(13, e.hours);
}

static void test_dst_does_not_shift_days() {
  // Über die Umstellung hinweg zählen Kalendertage weiter um 08:15 Lokalzeit
  const Elapsed e = at({2026, 3, 20, 8, 15, 0}, {2026, 4, 2, 8, 15, 0});
  assertYmd(e, 0, 0, 13);
  TEST_ASSERT_EQUAL(0, e.hours);
  TEST_ASSERT_EQUAL(0, e.minutes);
  TEST_ASSERT_EQUAL_INT64(13, e.totalDays);
}

static void test_event_in_dst_gap() {
  // Ereignis 02:30 am Umstellungstag existiert nicht → wie 03:30 behandelt
  const Elapsed e = at({2026, 3, 29, 2, 30, 0}, {2026, 3, 29, 4, 30, 0});
  TEST_ASSERT_EQUAL(1, e.hours);
  TEST_ASSERT_EQUAL(0, e.minutes);
}

static void test_future_countdown() {
  const Elapsed e = at({2026, 12, 24, 18, 0, 0}, {2026, 11, 23, 12, 0, 0});
  TEST_ASSERT_TRUE(e.future);
  assertYmd(e, 0, 1, 1);
  TEST_ASSERT_EQUAL(6, e.hours);
  TEST_ASSERT_EQUAL_INT64(31, e.totalDays);
}

static void test_future_countdown_years() {
  const Elapsed e = at({2030, 1, 1, 0, 0, 0}, {2026, 11, 23, 0, 0, 0});
  TEST_ASSERT_TRUE(e.future);
  assertYmd(e, 3, 1, 9);
}

static void test_future_one_minute() {
  const Elapsed e = at({2026, 11, 23, 12, 1, 0}, {2026, 11, 23, 12, 0, 0});
  TEST_ASSERT_TRUE(e.future);
  assertYmd(e, 0, 0, 0);
  TEST_ASSERT_EQUAL(0, e.hours);
  TEST_ASSERT_EQUAL(1, e.minutes);
}

static void test_utc_zone() {
  const TimeZone u = TimeZone::utc();
  const Elapsed e =
      elapsedSince({2020, 2, 29, 0, 0, 0}, toSeconds(DateTime{2021, 3, 1, 0, 0, 0}), u);
  assertYmd(e, 1, 0, 1);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_basic_example);
  RUN_TEST(test_same_moment_is_zero);
  RUN_TEST(test_before_time_of_day_not_full_day);
  RUN_TEST(test_birthday_exact_time);
  RUN_TEST(test_month_end_jan31);
  RUN_TEST(test_month_end_leap_year);
  RUN_TEST(test_leap_day_birth);
  RUN_TEST(test_total_days_and_weeks);
  RUN_TEST(test_dst_spring_forward_real_hours);
  RUN_TEST(test_dst_fall_back_real_hours);
  RUN_TEST(test_dst_does_not_shift_days);
  RUN_TEST(test_event_in_dst_gap);
  RUN_TEST(test_future_countdown);
  RUN_TEST(test_future_countdown_years);
  RUN_TEST(test_future_one_minute);
  RUN_TEST(test_utc_zone);
  return UNITY_END();
}
