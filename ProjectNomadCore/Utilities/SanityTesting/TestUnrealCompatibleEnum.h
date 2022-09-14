#pragma once

#if WITH_ENGINE // Use following necessary includes if in Unreal context
#include "CoreMinimal.h"
#include "TestUnrealCompatibleEnum.generated.h"
#else // Otherwise replace Unreal-specific code as appropriate
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

UENUM(BlueprintType)
enum class TestUnrealCompatibleEnum : uint8 {
    INVALID_ID,
    TestValue
};