#pragma once

#include <stdint.h>
#include <cstdint>

// Unreal container replacements
#include "Utilities/Containers/TArrayWrapper.h"
#include "Utilities/Containers/TMapWrapper.h"

// Unreal numeric type replacements
using uint8 = uint8_t;
using uint32 = uint32_t;
using int64 = std::int64_t;

// Do absolutely nothing for all Unreal-specific macros and flags
#define UENUM(x) /* nothing */
#define UCLASS(...) /* nothing */
#define USTRUCT(...) /* nothing */
#define UPROPERTY(x) /* nothing */
#define GENERATED_BODY() /* nothing */
#define THENOMADGAME_API /* nothing */