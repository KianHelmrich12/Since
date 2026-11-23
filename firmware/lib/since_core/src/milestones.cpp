// SPDX-License-Identifier: GPL-3.0-or-later
#include "milestones.h"

namespace since {

bool isDayCountMilestone(int64_t days) {
  return days == 100 || days == 500 || (days > 0 && days % 1000 == 0);
}

static bool sameDate(const Date& a, const Date& b) {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

Milestone milestoneOn(const Date& eventDate, const Date& today) {
  Milestone m;
  const int64_t dayDiff = daysFromCivil(today) - daysFromCivil(eventDate);
  if (dayDiff <= 0) return m;

  const int months = (today.year - eventDate.year) * 12 + (today.month - eventDate.month);
  const bool monthDay = months >= 1 && sameDate(addMonthsClamped(eventDate, months), today);

  if (monthDay && months % 12 == 0) {
    m.kind = MilestoneKind::kAnniversary;
    m.value = months / 12;
  } else if (isDayCountMilestone(dayDiff)) {
    m.kind = MilestoneKind::kDayCount;
    m.value = dayDiff;
  } else if (monthDay) {
    m.kind = MilestoneKind::kMonthDay;
    m.value = months;
  }
  return m;
}

}  // namespace since
