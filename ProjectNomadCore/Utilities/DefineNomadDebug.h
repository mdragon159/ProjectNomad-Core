#pragma once


// For now, always use debug mode.
// Thinking that in future, vast majority of code will be kept with debug flag set in order to maximize feature parity.
// eg, would only use a second flag to hide checks and features rather than to remove them entirely
#define NOMAD_USE_EXTRA_DEBUG_CHECKS 1