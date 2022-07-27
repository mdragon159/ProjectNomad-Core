#pragma once

// External: Data structures
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <math.h> // TODO: Preferably should not need this with CNL

// External: Other
#include <cstdint>
#include <chrono>
#include <functional>
#include <list>

// External: Directly vended libraries
// TODO: Preferably these should be located only in one place and thus no need for in here
#include <EnTT/entt.hpp>

// Internal: Widely used
// TODO: Darn inter-header dependencies! Can't include all WIDELY used headers (eg, FPVector.h) due to circular dependencies. Need ta use .cpp files