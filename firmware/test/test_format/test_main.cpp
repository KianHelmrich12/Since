// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

static Elapsed ymd(int y, int m, int d, int h = 0, int mi = 0) {
  Elapsed e;
  e.years = y;
  e.months = m;
  e.days = d;
  e.hours = h;
  e.minutes = mi;
  e.totalMonths = y * 12 + m;
  return e;
}

static void test_singular_plural() {
  TEST_ASSERT_EQUAL_STRING("1 Jahr", formatQuantity(1, Unit::kYear).c_str());
  TEST_ASSERT_EQUAL_STRING("2 Jahre", formatQuantity(2, Unit::kYear).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Monat", formatQuantity(1, Unit::kMonth).c_str());
  TEST_ASSERT_EQUAL_STRING("5 Monate", formatQuantity(5, Unit::kMonth).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Woche", formatQuantity(1, Unit::kWeek).c_str());
  TEST_ASSERT_EQUAL_STRING("3 Wochen", formatQuantity(3, Unit::kWeek).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Tag", formatQuantity(1, Unit::kDay).c_str());
  TEST_ASSERT_EQUAL_STRING("0 Tage", formatQuantity(0, Unit::kDay).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Stunde", formatQuantity(1, Unit::kHour).c_str());
  TEST_ASSERT_EQUAL_STRING("2 Minuten", formatQuantity(2, Unit::kMinute).c_str());
}

static void test_dative() {
  const GrammarCase d = GrammarCase::kDative;
  TEST_ASSERT_EQUAL_STRING("3 Jahren", formatQuantity(3, Unit::kYear, d).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Jahr", formatQuantity(1, Unit::kYear, d).c_str());
  TEST_ASSERT_EQUAL_STRING("2 Monaten", formatQuantity(2, Unit::kMonth, d).c_str());
  TEST_ASSERT_EQUAL_STRING("14 Tagen", formatQuantity(14, Unit::kDay, d).c_str());
}

static void test_thousands() {
  TEST_ASSERT_EQUAL_STRING("0", formatThousands(0).c_str());
  TEST_ASSERT_EQUAL_STRING("999", formatThousands(999).c_str());
  TEST_ASSERT_EQUAL_STRING("1.000", formatThousands(1000).c_str());
  TEST_ASSERT_EQUAL_STRING("1.234.567", formatThousands(1234567).c_str());
  TEST_ASSERT_EQUAL_STRING("-1.234", formatThousands(-1234).c_str());
  TEST_ASSERT_EQUAL_STRING("1.234 Tage", formatQuantity(1234, Unit::kDay).c_str());
}

static void test_format_standard() {
  TEST_ASSERT_EQUAL_STRING("3 Jahre · 2 Monate · 14 Tage", formatElapsed(ymd(3, 2, 14)).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Jahr · 1 Monat · 1 Tag", formatElapsed(ymd(1, 1, 1)).c_str());
}

static void test_format_hides_zero() {
  TEST_ASSERT_EQUAL_STRING("2 Jahre · 3 Tage", formatElapsed(ymd(2, 0, 3)).c_str());
  TEST_ASSERT_EQUAL_STRING("5 Monate", formatElapsed(ymd(0, 5, 0)).c_str());
  TEST_ASSERT_EQUAL_STRING("0 Tage", formatElapsed(ymd(0, 0, 0)).c_str());
}

static void test_format_with_hours_and_limit() {
  FormatOptions o;
  o.hours = true;
  o.minutes = true;
  o.maxParts = 2;
  TEST_ASSERT_EQUAL_STRING("3 Tage · 4 Stunden", formatElapsed(ymd(0, 0, 3, 4, 5), o).c_str());
  TEST_ASSERT_EQUAL_STRING("1 Stunde · 1 Minute", formatElapsed(ymd(0, 0, 0, 1, 1), o).c_str());
  TEST_ASSERT_EQUAL_STRING("0 Minuten", formatElapsed(ymd(0, 0, 0), o).c_str());
}

static void test_format_disabled_units_carry() {
  FormatOptions o;
  o.years = false;
  TEST_ASSERT_EQUAL_STRING("26 Monate · 4 Tage", formatElapsed(ymd(2, 2, 4), o).c_str());
  o.months = false;
  Elapsed e = ymd(1, 0, 0);
  e.totalDays = 365;
  TEST_ASSERT_EQUAL_STRING("365 Tage", formatElapsed(e, o).c_str());
}

static void test_format_custom_separator_dative() {
  FormatOptions o;
  o.separator = ", ";
  o.grammarCase = GrammarCase::kDative;
  TEST_ASSERT_EQUAL_STRING("3 Jahren, 2 Monaten, 1 Tag", formatElapsed(ymd(3, 2, 1), o).c_str());
}

static void test_format_dates() {
  TEST_ASSERT_EQUAL_STRING("12.03.2026", formatDate(Date{2026, 3, 12}).c_str());
  TEST_ASSERT_EQUAL_STRING("12.03.2026, 08:15",
                           formatDateTime(DateTime{2026, 3, 12, 8, 15, 30}).c_str());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_singular_plural);
  RUN_TEST(test_dative);
  RUN_TEST(test_thousands);
  RUN_TEST(test_format_standard);
  RUN_TEST(test_format_hides_zero);
  RUN_TEST(test_format_with_hours_and_limit);
  RUN_TEST(test_format_disabled_units_carry);
  RUN_TEST(test_format_custom_separator_dative);
  RUN_TEST(test_format_dates);
  return UNITY_END();
}
