// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

static void test_core_version_is_set() { TEST_ASSERT_NOT_NULL(coreVersion()); }

static void test_leap_years() {
  TEST_ASSERT_TRUE(isLeapYear(2024));
  TEST_ASSERT_TRUE(isLeapYear(2000));
  TEST_ASSERT_FALSE(isLeapYear(1900));
  TEST_ASSERT_FALSE(isLeapYear(2026));
}

static void test_days_in_month() {
  TEST_ASSERT_EQUAL(29, daysInMonth(2024, 2));
  TEST_ASSERT_EQUAL(28, daysInMonth(2026, 2));
  TEST_ASSERT_EQUAL(31, daysInMonth(2026, 12));
  TEST_ASSERT_EQUAL(30, daysInMonth(2026, 4));
  TEST_ASSERT_EQUAL(0, daysInMonth(2026, 13));
}

static void test_days_from_civil_roundtrip() {
  TEST_ASSERT_EQUAL_INT64(0, daysFromCivil(1970, 1, 1));
  TEST_ASSERT_EQUAL_INT64(-1, daysFromCivil(1969, 12, 31));
  TEST_ASSERT_EQUAL_INT64(20541, daysFromCivil(2026, 3, 29));  // 20454 + 87
  for (int64_t d = -800000; d < 800000; d += 997) {
    TEST_ASSERT_EQUAL_INT64(d, daysFromCivil(civilFromDays(d)));
  }
}

static void test_weekday() {
  TEST_ASSERT_EQUAL(4, weekday(0));                          // Do 01.01.1970
  TEST_ASSERT_EQUAL(0, weekday(daysFromCivil(2026, 3, 29)));  // So
  TEST_ASSERT_EQUAL(1, weekday(daysFromCivil(2026, 11, 23)));  // Mo
}

static void test_seconds_roundtrip() {
  const DateTime dt{2026, 3, 12, 8, 15, 42};
  const DateTime back = fromSeconds(toSeconds(dt));
  TEST_ASSERT_EQUAL(0, compare(dt, back));
  const DateTime neg = fromSeconds(-1);
  TEST_ASSERT_EQUAL(1969, neg.year);
  TEST_ASSERT_EQUAL(23, neg.hour);
  TEST_ASSERT_EQUAL(59, neg.second);
}

static void test_add_months_clamps_month_end() {
  Date d = addMonthsClamped(Date{2026, 1, 31}, 1);
  TEST_ASSERT_EQUAL(2, d.month);
  TEST_ASSERT_EQUAL(28, d.day);
  d = addMonthsClamped(Date{2024, 1, 31}, 1);
  TEST_ASSERT_EQUAL(29, d.day);
  d = addMonthsClamped(Date{2026, 1, 31}, 2);
  TEST_ASSERT_EQUAL(3, d.month);
  TEST_ASSERT_EQUAL(31, d.day);
  d = addMonthsClamped(Date{2026, 3, 15}, -3);
  TEST_ASSERT_EQUAL(2025, d.year);
  TEST_ASSERT_EQUAL(12, d.month);
}

static void test_add_months_leap_day() {
  Date d = addMonthsClamped(Date{2024, 2, 29}, 12);
  TEST_ASSERT_EQUAL(2025, d.year);
  TEST_ASSERT_EQUAL(2, d.month);
  TEST_ASSERT_EQUAL(28, d.day);
  d = addMonthsClamped(Date{2024, 2, 29}, 48);
  TEST_ASSERT_EQUAL(29, d.day);
}

static void test_validation() {
  TEST_ASSERT_TRUE(isValidDate(Date{2024, 2, 29}));
  TEST_ASSERT_FALSE(isValidDate(Date{2026, 2, 29}));
  TEST_ASSERT_FALSE(isValidDate(Date{2026, 0, 1}));
  TEST_ASSERT_FALSE(isValidDateTime(DateTime{2026, 1, 1, 24, 0, 0}));
  TEST_ASSERT_TRUE(isValidDateTime(DateTime{2026, 1, 1, 23, 59, 59}));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_core_version_is_set);
  RUN_TEST(test_leap_years);
  RUN_TEST(test_days_in_month);
  RUN_TEST(test_days_from_civil_roundtrip);
  RUN_TEST(test_weekday);
  RUN_TEST(test_seconds_roundtrip);
  RUN_TEST(test_add_months_clamps_month_end);
  RUN_TEST(test_add_months_leap_day);
  RUN_TEST(test_validation);
  return UNITY_END();
}
