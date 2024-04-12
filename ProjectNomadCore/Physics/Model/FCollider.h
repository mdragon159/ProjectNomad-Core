#pragma once

#include "ColliderType.h"
#include "Math/FQuatFP.h"
#include "Line.h"

#if WITH_ENGINE
#include "CoreMinimal.h"
#include "FCollider.generated.h"
#else
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

// Composite type for all supported colliders
// Directly inspired by Unreal's FCollisionShape. Yay for stumbling on a great way to do this without pointers!
// Perhaps should just make this a class to make it explicit to go through getters/setters?
USTRUCT(BlueprintType)
struct THENOMADGAME_API FCollider {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    ColliderType colliderType = ColliderType::NotInitialized;
    UPROPERTY(EditAnywhere)
    FVectorFP center;
    UPROPERTY(EditAnywhere)
    FQuatFP rotation;

    // TODO: Assert error if half size or rotation is invalid
    // Half size must be positive (>0 in all dimensions)
    // Rotation must be a unit quaternion

    // Data describing shape of each specific collider shape
    // TODO: Would be nice to combine these all into a union. Note that the union at the time (pre-June 21st, 2022)
    //       was causing occasional build issues with UE5 (w/ C++20) and thus scrapped for time being
    UPROPERTY(EditAnywhere)
    FFixedPoint boxHalfSizeX{0}; // Positive halfwidth extents in each direction per axis from center point
    UPROPERTY(EditAnywhere)
    FFixedPoint boxHalfSizeY{0};
    UPROPERTY(EditAnywhere)
    FFixedPoint boxHalfSizeZ{0};
    UPROPERTY(EditAnywhere)
    FFixedPoint capsuleHalfHeight{0}; // One half of total height of capsule, including rounded ends. Should be >= radius
    UPROPERTY(EditAnywhere)
    FFixedPoint radius{0};            // Either radius of sphere OR radius of rounded ends of capsule

    static FCollider GetAnyValidCollider(); // Utility function to help with setting valid fallback cases
    
    void SetBox(const FVectorFP& newCenter, const FVectorFP& halfSize);
    void SetBox(const FVectorFP& newCenter, const FQuatFP& newRotation, const FVectorFP& halfSize);

    void SetCapsule(FVectorFP newCenter, FFixedPoint newRadius, FFixedPoint halfHeight);
    /**
    * Setup capsule based on "point A" and "point B" positions. ie, where is the base sphere and where is the final
    * sphere centers
    * @param pointA - start/bottom sphere center
    * @param pointB - end/top sphere center
    * @param newRadius - radius of spheres/capsule
    **/
    void SetCapsule(FVectorFP pointA, FVectorFP pointB, FFixedPoint newRadius);
    void SetCapsule(FVectorFP newCenter, FQuatFP newRotation, FFixedPoint newRadius, FFixedPoint halfHeight);

    void SetSphere(FVectorFP newCenter, FFixedPoint newRadius);
    
    bool IsNotInitialized() const;
    bool IsValid() const;
    bool IsBox() const;
    bool IsCapsule() const;
    bool IsSphere() const;

    void SetCenter(const FVectorFP& newCenter);
    FVectorFP GetCenter() const;

    void SetRotation(const FQuatFP& newRotation);
    FQuatFP GetRotation() const;

    void SetBoxHalfSize(const FVectorFP& newHalfSize);
    FVectorFP GetBoxHalfSize() const;

    void SetCapsuleRadius(const FFixedPoint& newRadius);
    FFixedPoint GetCapsuleRadius() const;

    void SetCapsuleHalfHeight(const FFixedPoint& newHalfHeight);
    FFixedPoint GetCapsuleHalfHeight() const;

    void SetSphereRadius(const FFixedPoint& newRadius);
    FFixedPoint GetSphereRadius() const;


    FVectorFP ToWorldSpaceFromLocal(const FVectorFP& value) const;
    FVectorFP ToWorldSpaceForOriginCenteredValue(const FVectorFP& value) const;

    /// <summary>
    /// Turns a point in world space to collider's local space.
    /// If value represents a direction, then instead use toLocalSpaceForOriginCenteredValue
    /// </summary>
    FVectorFP ToLocalSpaceFromWorld(const FVectorFP& value) const;

    /// <summary>
    /// Turns a direction or point in world space that's centered on origin to collider's local space.
    /// Use this directly with directions. For points, recommended to use toLocalSpaceFromWorld instead
    /// </summary>
    // TODO: Refactor these methods into public toLocalSpaceFromWorldPosition and toLocalSpaceFromWorldDirection
    FVectorFP ToLocalSpaceForOriginCenteredValue(const FVectorFP& value) const;

    // Return more or less rough estimate of bounds on horizontal plane
    FFixedPoint GetHorizontalPlaneBoundsRadius() const;
    FFixedPoint GetVerticalHalfHeightBounds() const;

    void ApplyMultiplier(FFixedPoint multiplier);

    // Utility method for when, say, have a hitbox definition collider that needs to be re-centered before further processing
    FCollider CopyWithNewCenter(const FVectorFP& newCenter) const;

    void CalculateCRC32(uint32_t& resultThusFar) const;

    std::string ToString() const;
    std::string GetTypeAsString() const;


#pragma region Box Specific Functionality

    std::vector<FVectorFP> GetBoxVerticesInWorldCoordinates() const;
    // TODO: Not vastly important due to small fixed size, but preferably don't copy results. Instead pass in a vector to fill with data
    std::vector<FVectorFP> GetBoxNormalsInWorldCoordinates() const;

    bool IsWorldSpacePtWithinBoxIncludingOnSurface(const FVectorFP& point) const;
    bool IsLocalSpacePtWithinBoxIncludingOnSurface(const FVectorFP& localPoint) const;

    bool IsWorldSpacePtWithinBoxExcludingOnSurface(const FVectorFP& point) const;
    bool IsLocalSpacePtWithinBoxExcludingOnSurface(const FVectorFP& localPoint) const;

    /// <summary>
    /// Checks the "faces" a point is touching. This may include more than one "face" if point is on an edge (2 faces)
    /// or vertex (3 faces).
    /// SIDE NOTE: The entire point of this is to see if multiple points lie on the same "face", ie if the points create
    //              a line segment which crosses into the box OR if just on surface. Thus not relying on typical
    //              definition of "face" but rather exactly what faces are being touched, if any.
    /// </summary>
    /// <param name="localPoint">
    /// Point to check against. This is assumed to already be known to NOT be outside box.
    /// (ie, the point is either on surface of box or within box)
    /// </param>
    /// <param name="resultFaces">Results ("faces" that point touches) are added to this vector</param>
    void GetFacesThatLocalSpacePointTouches(const FVectorFP& localPoint, std::vector<FVectorFP>& resultFaces) const;

#pragma endregion
#pragma region Capsule Specific Functionality

    FFixedPoint GetMedialHalfLineLength() const;
    /// <summary>
    /// Gets extreme "center" points of capsule. ie, tip and base of capsule offset by radius along medial line
    /// Visually, points A and B on this picture: https://turanszkij.files.wordpress.com/2020/04/capsule-1.png
    /// </summary>
    /// <returns>Returns medial line extremes of capsule (either end of center line of capsule offset by radius)</returns>        
    ProjectNomad::Line GetCapsuleMedialLineExtremes() const;

#pragma endregion
};

inline std::ostream& operator<<(std::ostream& os, const FCollider& value) {
    os << "Collider<" << value.ToString() << ">";
    return os;
}
