#pragma once

#if WITH_ENGINE
#include "CoreMinimal.h"
#include "ColliderType.generated.h"
#else
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

UENUM(BlueprintType)
enum class ColliderType : uint8 {
    NotInitialized, Box, Sphere, Capsule
};