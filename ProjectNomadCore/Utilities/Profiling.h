#pragma once

#if WITH_ENGINE
#include "Stats/Stats2.h"

DECLARE_STATS_GROUP(TEXT("Gameplay Systems"), STATGROUP_Nomad_GSystems, STATCAT_Advanced);

// Custom define to make things a bit easier (and flexible in case want to easily change this in future)
#define MEASURE_SYSTEM_FUNCTION(TextName, StatName) DECLARE_SCOPE_CYCLE_COUNTER(TEXT(TextName), StatName, STATGROUP_Nomad_GSystems)

#else
MEASURE_FUNCTION(...)
#endif