// SPDX-License-Identifier: GPL-3.0-or-later
// since_core – hardwarefreie Kernlogik. Keine Arduino-/ESP-IDF-Header einbinden!
#pragma once

#include "api.h"
#include "battery.h"
#include "civil.h"
#include "config.h"
#include "cycle.h"
#include "elapsed.h"
#include "format_de.h"
#include "framing.h"
#include "milestones.h"
#include "schedule.h"
#include "screen.h"
#include "setup.h"
#include "tz.h"

namespace since {

const char* coreVersion();

}  // namespace since
