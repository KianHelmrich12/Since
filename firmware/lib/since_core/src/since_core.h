// SPDX-License-Identifier: GPL-3.0-or-later
// since_core – hardwarefreie Kernlogik. Keine Arduino-/ESP-IDF-Header einbinden!
#pragma once

#include "civil.h"
#include "config.h"
#include "elapsed.h"
#include "format_de.h"
#include "milestones.h"
#include "tz.h"

namespace since {

const char* coreVersion();

}  // namespace since
