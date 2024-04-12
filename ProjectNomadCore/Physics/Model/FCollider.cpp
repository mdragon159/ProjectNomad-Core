#include "FCollider.h"

#include "Ray.h"
#include "Math/FPMath2.h"

FCollider FCollider::GetAnyValidCollider() {
    FCollider result;
    result.SetBox(FVectorFP::Zero(), FVectorFP(FFixedPoint(1)));
    return result;
}

void FCollider::SetBox(const FVectorFP& newCenter, const FVectorFP& halfSize) {
    SetBox(newCenter, FQuatFP::identity(), halfSize);
}

void FCollider::SetBox(const FVectorFP& newCenter, const FQuatFP& newRotation, const FVectorFP& halfSize) {
    colliderType = ColliderType::Box;
    SetCenter(newCenter);
    SetRotation(newRotation);

    SetBoxHalfSize(halfSize);
}

void FCollider::SetCapsule(FVectorFP newCenter, FFixedPoint newRadius, FFixedPoint halfHeight) {
    SetCapsule(newCenter, FQuatFP::identity(), newRadius, halfHeight);
}

void FCollider::SetCapsule(FVectorFP pointA, FVectorFP pointB, FFixedPoint newRadius) {
    // Center is halfway position between provided points
    FVectorFP newCenter = (pointA + pointB) / FFixedPoint{2};
        
    // Full height = distance between points plus buffer room (radius) on either side of capsule endpoint spheres
    FFixedPoint fullHeight = FVectorFP::Distance(pointA, pointB) + newRadius *  2;
        
    // Rotation is just direction from A to B, where upwards direction is standard "no rotation" for capsules
    FVectorFP aToBDir = FVectorFP::Direction(pointA, pointB);
    FQuatFP newRotation = ProjectNomad::FPMath2::dirVectorToQuat(aToBDir, FVectorFP::Up());

    // Finally set values with chosen standard format
    SetCapsule(newCenter, newRotation, newRadius, fullHeight / 2);
}

void FCollider::SetCapsule(FVectorFP newCenter, FQuatFP newRotation, FFixedPoint newRadius, FFixedPoint halfHeight) {
    colliderType = ColliderType::Capsule;
    SetCenter(newCenter);
    SetRotation(newRotation);

    SetCapsuleRadius(newRadius);
    SetCapsuleHalfHeight(halfHeight);
}

void FCollider::SetSphere(FVectorFP newCenter, FFixedPoint newRadius) {
    colliderType = ColliderType::Sphere;
    SetCenter(newCenter);
    // No need to set rotation as rotation is useless for sphere

    SetSphereRadius(newRadius);
}

bool FCollider::IsNotInitialized() const {
    return colliderType == ColliderType::NotInitialized;
}

bool FCollider::IsValid() const {
    switch (colliderType) {
        case ColliderType::Box:
            return boxHalfSizeX > FFixedPoint(0) && boxHalfSizeY > FFixedPoint(0) && boxHalfSizeZ > FFixedPoint(0);
        case ColliderType::Capsule:
            return radius > FFixedPoint(0) && capsuleHalfHeight >= radius;
        case ColliderType::Sphere:
            return radius > FFixedPoint(0);
        default:
            return false;
    }
}

bool FCollider::IsBox() const {
    return colliderType == ColliderType::Box;
}

bool FCollider::IsCapsule() const {
    return colliderType == ColliderType::Capsule;
}

bool FCollider::IsSphere() const {
    return colliderType == ColliderType::Sphere;
}

void FCollider::SetCenter(const FVectorFP& newCenter) {
    center = newCenter;
}

FVectorFP FCollider::GetCenter() const {
    return center;
}

void FCollider::SetRotation(const FQuatFP& newRotation) {
    rotation = newRotation;
}

FQuatFP FCollider::GetRotation() const {
    return rotation;
}

void FCollider::SetBoxHalfSize(const FVectorFP& newHalfSize){
    if (!IsBox()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return;
    }

    boxHalfSizeX = newHalfSize.x;
    boxHalfSizeY = newHalfSize.y;
    boxHalfSizeZ = newHalfSize.z;
}

FVectorFP FCollider::GetBoxHalfSize() const {
    if (!IsBox()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return FVectorFP::Zero();
    }

    return {
        boxHalfSizeX, boxHalfSizeY, boxHalfSizeZ
    };
}

void FCollider::SetCapsuleRadius(const FFixedPoint& newRadius) {
    if (!IsCapsule()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return;
    }

    radius = newRadius;
}

FFixedPoint FCollider::GetCapsuleRadius() const {
    if (!IsCapsule()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return FFixedPoint{0};
    }

    return radius;
}

void FCollider::SetCapsuleHalfHeight(const FFixedPoint& newHalfHeight) {
    if (!IsCapsule()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return;
    }

    capsuleHalfHeight = newHalfHeight;
}

FFixedPoint FCollider::GetCapsuleHalfHeight() const {
    if (!IsCapsule()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return FFixedPoint{0};
    }

    return capsuleHalfHeight;
}

void FCollider::SetSphereRadius(const FFixedPoint& newRadius) {
    if (!IsSphere()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return;
    }

    radius = newRadius;
}

FFixedPoint FCollider::GetSphereRadius() const {
    if (!IsSphere()) {
        // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
        return FFixedPoint{0};
    }

    return radius;
}

FVectorFP FCollider::ToWorldSpaceFromLocal(const FVectorFP& value) const {
    // Essentially reverse of toLocalSpaceFromWorld. Note order of operations!
    FVectorFP rotatedValue = ToWorldSpaceForOriginCenteredValue(value);
    return rotatedValue + center;
}

FVectorFP FCollider::ToWorldSpaceForOriginCenteredValue(const FVectorFP& value) const {
    // This is its own separate public method as useful for directions
    // (ie, directions are always with origin at 0, just need to rotate em between world vs local)

    // Quick wayyyy later note off top of head:
    // This is assuming <0, 0, 0> is center of world. I think
    return rotation * value;
}

FVectorFP FCollider::ToLocalSpaceFromWorld(const FVectorFP& value) const {
    FVectorFP valueAsDisplacementFromBoxOrigin = value - center;
    return ToLocalSpaceForOriginCenteredValue(valueAsDisplacementFromBoxOrigin);
}

FVectorFP FCollider::ToLocalSpaceForOriginCenteredValue(const FVectorFP& value) const {
    // This is its own separate public method as useful for directions
    // (ie, directions are always with origin at 0, just need to rotate em between world vs local)

    // Quick wayyyy later note off top of head:
    // This is assuming <0, 0, 0> is center of world. TODO: Redo comments on this and prior two functions
    return rotation.inverted() * value;
}

FFixedPoint FCollider::GetHorizontalPlaneBoundsRadius() const {
    switch (colliderType) {
        case ColliderType::Box:
            return GetBoxHalfSize().x; // TODO: Account for when x and y aren't the same
        case ColliderType::Capsule:
            return GetCapsuleRadius();
        case ColliderType::Sphere:
            return GetSphereRadius();
        default:
            // Would be nice to log some kinda message here but also not really necessary in context?
                return FFixedPoint{0};
    }
}

FFixedPoint FCollider::GetVerticalHalfHeightBounds() const {
    switch (colliderType) {
        case ColliderType::Box:
            return GetBoxHalfSize().z;
        case ColliderType::Capsule:
            return GetCapsuleHalfHeight();
        case ColliderType::Sphere:
            return GetSphereRadius();
        default:
            // Would be nice to log some kinda message here but also not really necessary in context?
                return FFixedPoint{0};
    }
}

void FCollider::ApplyMultiplier(FFixedPoint multiplier) {
    switch (colliderType) {
        case ColliderType::Box:
            SetBoxHalfSize(GetBoxHalfSize() * multiplier);
        break;
        case ColliderType::Capsule:
            SetCapsuleHalfHeight(GetCapsuleHalfHeight() * multiplier);
        SetCapsuleRadius(GetCapsuleRadius() * multiplier);
        break;
                    
        case ColliderType::Sphere:
            SetSphereRadius(GetSphereRadius() * multiplier);
        break;

        default:
            break; // TODO: Log somehow
    }
}

FCollider FCollider::CopyWithNewCenter(const FVectorFP& newCenter) const {
    FCollider result = *this;
    result.SetCenter(newCenter);
    return result;
}

void FCollider::CalculateCRC32(uint32_t& resultThusFar) const {
    center.CalculateCRC32(resultThusFar);
    rotation.CalculateCRC32(resultThusFar);

    // Only make checksum based on values in use, which depends on collider type.
    // This is (supposedly) necessary as constructors and setters are designed to NOT set other values.
    switch (colliderType) {
        case ColliderType::Box:
            boxHalfSizeX.CalculateCRC32(resultThusFar);
        boxHalfSizeY.CalculateCRC32(resultThusFar);
        boxHalfSizeZ.CalculateCRC32(resultThusFar);
        break;

        case ColliderType::Capsule:
            capsuleHalfHeight.CalculateCRC32(resultThusFar);
        radius.CalculateCRC32(resultThusFar);
        break;

        case ColliderType::Sphere:
            radius.CalculateCRC32(resultThusFar);
        break;

        case ColliderType::NotInitialized:
            default:
                // Would be nice to log some kinda message here but also not really necessary in context?
                return;
    }
}

std::string FCollider::ToString() const {
    switch (colliderType) {
        case ColliderType::NotInitialized:
            return "<Not Initialized Collider>";

        case ColliderType::Box:
            return "Box center: {" + center.ToString()
                + "}, rotation: {" + rotation.ToString()
                + "}, halfSize: {" + GetBoxHalfSize().ToString()
                + "}";

        case ColliderType::Capsule:
            return "Capsule center: {" + center.ToString()
                + "}, rotation: {" + rotation.ToString()
                + "}, radius: {" + radius.ToString()
                + "}, halfHeight: {" + capsuleHalfHeight.ToString()
                + "}";

        case ColliderType::Sphere:
            return "Sphere center: {" + center.ToString()
                + "}, radius: {" + radius.ToString()
                + "}";

        default:
            return "<Unknown Collider Type. Implement please!>";
    }
}

std::string FCollider::GetTypeAsString() const {
    switch (colliderType) {
        case ColliderType::NotInitialized:
            return "<Not Initialized Collider>";

        case ColliderType::Box:
            return "Box";

        case ColliderType::Capsule:
            return "Capsule";

        case ColliderType::Sphere:
            return "Sphere";

        default:
            return "<Unknown Collider Type. Implement please!>";
    }
}

std::vector<FVectorFP> FCollider::GetBoxVerticesInWorldCoordinates() const {
    std::vector<FVectorFP> result;
    FVectorFP halfSize = GetBoxHalfSize();

    // First, the bottom back left and top front right points
    //  ...yeah I do regret these location names. IDEA: Put names somewhere central, like in CollisionHelpers.h
    result.push_back(center + rotation * -halfSize);
    result.push_back(center + rotation * halfSize);

    // Then calculate all the other combinations one by one
    result.push_back(center + rotation * FVectorFP(-halfSize.x, halfSize.y, -halfSize.z)); // bottom back right
    result.push_back(center + rotation * FVectorFP(halfSize.x, -halfSize.y, -halfSize.z)); // bottom front left
    result.push_back(center + rotation * FVectorFP(halfSize.x, halfSize.y, -halfSize.z)); // bottom front right
    result.push_back(center + rotation * FVectorFP(-halfSize.x, -halfSize.y, halfSize.z)); // top back left
    result.push_back(center + rotation * FVectorFP(-halfSize.x, halfSize.y, halfSize.z)); // top back right
    result.push_back(center + rotation * FVectorFP(halfSize.x, -halfSize.y, halfSize.z)); // top front left

    return result;
}

std::vector<FVectorFP> FCollider::GetBoxNormalsInWorldCoordinates() const {
    std::vector<FVectorFP> results;

    // Use x/y/z axes and rotate as necessary to get world coordinate axes
    // In addition, no need currently for parallel normals (eg, -x and +x)
    results.push_back(ToWorldSpaceForOriginCenteredValue(FVectorFP::Forward()));
    results.push_back(ToWorldSpaceForOriginCenteredValue(FVectorFP::Right()));
    results.push_back(ToWorldSpaceForOriginCenteredValue(FVectorFP::Up()));

    return results;
}

bool FCollider::IsWorldSpacePtWithinBoxIncludingOnSurface(const FVectorFP& point) const {
    FVectorFP localSpacePt = ToLocalSpaceFromWorld(point);
    return IsLocalSpacePtWithinBoxIncludingOnSurface(localSpacePt);
}

bool FCollider::IsLocalSpacePtWithinBoxIncludingOnSurface(const FVectorFP& localPoint) const {
    FVectorFP halfSize = GetBoxHalfSize();

    // If outside bounds of an axis, then know for sure not located in the obb
    if (localPoint.x < -halfSize.x || localPoint.x > halfSize.x) return false;
    if (localPoint.y < -halfSize.y || localPoint.y > halfSize.y) return false;
    if (localPoint.z < -halfSize.z || localPoint.z > halfSize.z) return false;

    // Within bounds of all three axes. Thus, point is definitely located within the obb
    return true;
}

bool FCollider::IsWorldSpacePtWithinBoxExcludingOnSurface(const FVectorFP& point) const {
    FVectorFP localSpacePt = ToLocalSpaceFromWorld(point);
    return IsLocalSpacePtWithinBoxExcludingOnSurface(localSpacePt);
}

bool FCollider::IsLocalSpacePtWithinBoxExcludingOnSurface(const FVectorFP& localPoint) const {
    // Check if outside box (including surface) entirely
    if (!IsLocalSpacePtWithinBoxIncludingOnSurface(localPoint)) {
        return false;
    }
        
    // Check if point on surface
    // Note that this is defined by any coordinate being on (+/-) max extent for that axis
    //      (and already checked that all coordinates are within range of box axes)
    // Easy to see if map out coordinates for each of the 6 faces of a box
    FVectorFP halfSize = GetBoxHalfSize();
    if (localPoint.x == -halfSize.x || localPoint.x == halfSize.x) {
        return false;
    }
    if (localPoint.y == -halfSize.y || localPoint.y == halfSize.y) {
        return false;
    }
    if (localPoint.z == -halfSize.z || localPoint.z == halfSize.z) {
        return false;
    }

    // Confirmed not entirely outside box nor on surface of box. Thus only other possibility is that it MUST be inside box
    return true;
}

void FCollider::GetFacesThatLocalSpacePointTouches(const FVectorFP& localPoint, std::vector<FVectorFP>& resultFaces) const {
    FVectorFP maxExtents = GetBoxHalfSize();
    FVectorFP minExtents = -maxExtents;

    // Approach is very simple:
    // Input point is known to be already on surface of or within box (due to input requirements).
    // Thus, if a coordinate reaches the extents of that axis, then it touches that corresponding face.
    // For example: If point is on top forward right vertex (+x, +y, +z),
    //              then it's touching front (+x), right (+y), AND +z faces!

    // Side note, I stumbled on this solution after writing out the hard way via if statements. Was very obvious
    //      what the "optimal" solutions was after all that work, which seems to be a very common pattern...
        
    if (ProjectNomad::FPMath::isNear(localPoint.x, maxExtents.x, FFixedPoint{0.001f})) {
        resultFaces.push_back(FVectorFP::Forward());
    }
    else if (ProjectNomad::FPMath::isNear(localPoint.x, minExtents.x, FFixedPoint{0.001f})) {
        resultFaces.push_back(FVectorFP::Backward());
    }

    if (ProjectNomad::FPMath::isNear(localPoint.y, maxExtents.y, FFixedPoint{0.001f})) {
        resultFaces.push_back(FVectorFP::Right());
    }
    else if (ProjectNomad::FPMath::isNear(localPoint.y, minExtents.y, FFixedPoint{0.001f})) {
        resultFaces.push_back(FVectorFP::Left());
    }

    if (ProjectNomad::FPMath::isNear(localPoint.z, maxExtents.z, FFixedPoint{0.001f})) {
        resultFaces.push_back(FVectorFP::Up());
    }
    else if (ProjectNomad::FPMath::isNear(localPoint.z, minExtents.z, FFixedPoint{0.001f})) {
        resultFaces.push_back(FVectorFP::Down());
    }
}

FFixedPoint FCollider::GetMedialHalfLineLength() const {
    return GetCapsuleHalfHeight() - GetCapsuleRadius();
}

ProjectNomad::Line FCollider::GetCapsuleMedialLineExtremes() const {
    FFixedPoint pointDistanceFromCenter = GetMedialHalfLineLength();

    // Calculate rotated directions towards "bottom" and "top"
    // Note that this is dependent on capsule's default orientation being vertical
    FVectorFP rotatedUpDir = rotation * FVectorFP::Up();
    FVectorFP rotatedDownDir = rotatedUpDir * FFixedPoint{-1};

    // Calculate the extreme points then return em
    FVectorFP pointA = center + rotatedDownDir * pointDistanceFromCenter;
    // Small optimization note: Don't need rotatedDownDir. Just subtract
    FVectorFP pointB = center + rotatedUpDir * pointDistanceFromCenter;
    return {pointA, pointB};
}
