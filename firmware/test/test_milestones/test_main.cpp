// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

static void expect(MilestoneKind k, int64_t v, const Milestone& m) {
  TEST_ASSERT_EQUAL(static_cast<int>(k), static_cast<int>(m.kind));
  TEST_ASSERT_EQUAL_INT64(v, m.value);
}

static void test_birthday() {
  expect(MilestoneKind::kAnniversary, 1, milestoneOn({2025, 6, 1}, {2026, 6, 1}));
  expect(MilestoneKind::kAnniversary, 10, milestoneOn({2016, 6, 1}, {2026, 6, 1}));
}

static void test_leap_day_birthday() {
  expect(MilestoneKind::kAnniversary, 1, milestoneOn({2024, 2, 29}, {2025, 2, 28}));
  expect(MilestoneKind::kNone, 0, milestoneOn({2024, 2, 29}, {2025, 3, 1}));
  expect(MilestoneKind::kAnniversary, 4, milestoneOn({2024, 2, 29}, {2028, 2, 29}));
  // Im Schaltjahr nicht schon am 28.02.
  TEST_ASSERT_TRUE(milestoneOn({2024, 2, 29}, {2028, 2, 28}).kind != MilestoneKind::kAnniversary);
}

static void test_day_counts() {
  const Date birth{2026, 1, 1};
  expect(MilestoneKind::kDayCount, 100,
         milestoneOn(birth, civilFromDays(daysFromCivil(birth) + 100)));
  expect(MilestoneKind::kDayCount, 1000,
         milestoneOn(birth, civilFromDays(daysFromCivil(birth) + 1000)));
  expect(MilestoneKind::kNone, 0, milestoneOn(birth, civilFromDays(daysFromCivil(birth) + 101)));
  TEST_ASSERT_TRUE(isDayCountMilestone(500));
  TEST_ASSERT_TRUE(isDayCountMilestone(10000));
  TEST_ASSERT_FALSE(isDayCountMilestone(0));
  TEST_ASSERT_FALSE(isDayCountMilestone(200));
}

static void test_month_day() {
  expect(MilestoneKind::kMonthDay, 1, milestoneOn({2026, 3, 12}, {2026, 4, 12}));
  expect(MilestoneKind::kMonthDay, 14, milestoneOn({2025, 3, 12}, {2026, 5, 12}));
  // 31.01. → Monatstag am 28.02. (begrenzt), dann am 31.03.
  expect(MilestoneKind::kMonthDay, 1, milestoneOn({2026, 1, 31}, {2026, 2, 28}));
  expect(MilestoneKind::kMonthDay, 2, milestoneOn({2026, 1, 31}, {2026, 3, 31}));
  expect(MilestoneKind::kNone, 0, milestoneOn({2026, 1, 31}, {2026, 3, 28}));
}

static void test_priority_and_none() {
  // Event-Tag selbst und Zukunft: kein Meilenstein
  expect(MilestoneKind::kNone, 0, milestoneOn({2026, 3, 12}, {2026, 3, 12}));
  expect(MilestoneKind::kNone, 0, milestoneOn({2026, 3, 12}, {2026, 2, 12}));
  // 12. Monatstag ist ein Jahrestag → Jahrestag gewinnt
  expect(MilestoneKind::kAnniversary, 1, milestoneOn({2025, 3, 12}, {2026, 3, 12}));
  // Tag 100 vor Monatstag (Tag 100 ist nie ein Monatstag; Kontrolle der Reihenfolge)
  expect(MilestoneKind::kDayCount, 100, milestoneOn({2026, 1, 1}, {2026, 4, 11}));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_birthday);
  RUN_TEST(test_leap_day_birthday);
  RUN_TEST(test_day_counts);
  RUN_TEST(test_month_day);
  RUN_TEST(test_priority_and_none);
  return UNITY_END();
}
