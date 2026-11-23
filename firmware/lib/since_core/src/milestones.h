// SPDX-License-Identifier: GPL-3.0-or-later
// Meilensteine am heutigen lokalen Tag (datumsbasiert, gilt den ganzen Tag).
#pragma once

#include <cstdint>

#include "civil.h"

namespace since {

enum class MilestoneKind { kNone, kAnniversary, kDayCount, kMonthDay };

struct Milestone {
  MilestoneKind kind = MilestoneKind::kNone;
  int64_t value = 0;  // Jahre, Tage bzw. Monate
};

// Priorität: Jahrestag (Geburtstag) > runde Tageszahl > Monatstag.
// - Jahrestag: gleicher Tag/Monat, ≥ 1 Jahr; Ereignis am 29.02. → in Nicht-Schaltjahren
//   am 28.02.
// - Tageszahl: 100, 500 sowie jedes Vielfache von 1000 Tagen
// - Monatstag: n-ter Monat (n ≥ 1, kein voller Jahrestag), Monatsende begrenzt
// Liegt das Ereignis in der Zukunft oder ist heute der Ereignistag selbst: kNone.
Milestone milestoneOn(const Date& eventDate, const Date& today);

bool isDayCountMilestone(int64_t days);

}  // namespace since
