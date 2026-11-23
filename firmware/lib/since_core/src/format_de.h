// SPDX-License-Identifier: GPL-3.0-or-later
// Deutsche Formatierung von Zeitspannen und Datumsangaben.
#pragma once

#include <cstdint>
#include <string>

#include "civil.h"
#include "elapsed.h"

namespace since {

enum class Unit { kYear, kMonth, kWeek, kDay, kHour, kMinute };

// Nominativ: „3 Jahre“ (Standalone-Anzeige). Dativ: „seit 3 Jahren“.
enum class GrammarCase { kNominative, kDative };

// „Jahr“/„Jahre“/„Jahren“ …
const char* unitName(Unit u, int64_t value, GrammarCase gc = GrammarCase::kNominative);

// „1 Jahr“, „2 Jahre“, „1.234 Tage“
std::string formatQuantity(int64_t value, Unit u, GrammarCase gc = GrammarCase::kNominative);

// Tausenderpunkt: 1234567 → „1.234.567“
std::string formatThousands(int64_t value);

struct FormatOptions {
  bool years = true;
  bool months = true;
  bool days = true;
  bool hours = false;
  bool minutes = false;
  int maxParts = 3;                 // höchstens so viele Einheiten (größte zuerst)
  const char* separator = " · ";  // Mittelpunkt
  GrammarCase grammarCase = GrammarCase::kNominative;
};

// „3 Jahre · 2 Monate · 14 Tage“. Nullwerte werden ausgeblendet; sind alle Werte 0,
// wird die kleinste aktivierte Einheit mit 0 angezeigt („0 Tage“).
std::string formatElapsed(const Elapsed& e, const FormatOptions& opt = FormatOptions());

// „12.03.2026“ bzw. „12.03.2026, 08:15“
std::string formatDate(const Date& d);
std::string formatDateTime(const DateTime& dt);

}  // namespace since
