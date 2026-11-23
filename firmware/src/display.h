// SPDX-License-Identifier: GPL-3.0-or-later
// Rendering des Bildschirm-Modells auf das 7,5″-E-Paper (800×480, UC8179, GxEPD2).
#pragma once

#include "screen.h"

namespace display {

// Zeichnet das Modell (Vollbild-Refresh) und legt das Panel danach in Hibernate.
// Erwartet eingeschaltete Versorgung (hw::railOn()).
void show(const since::ScreenModel& m);

}  // namespace display
