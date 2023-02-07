#pragma once

#include "Ray.h"
#include "Math/FPMath2.h"
#include "Math/FPQuat.h"
#include "Physics/Line.h"

namespace ProjectNomad {
    enum class ColliderType { NotInitialized, Box, Sphere, Capsule };

    // Composite type for all supported colliders
    // Directly inspired by Unreal's FCollisionShape. Yay for stumbling on a great way to do this without pointers!
    // Perhaps should just make this a class to make it explicit to go through getters/setters?
    struct Collider {
        ColliderType colliderType = ColliderType::NotInitialized;
        FPVector center;
        FPQuat rotation;

        // TODO: Assert error if half size or rotation is invalid
        // Half size must be positive (>0 in all dimensions)
        // Rotation must be a unit quaternion

        // Data describing shape of each specific collider shape
        // TODO: Would be nice to combine these all into a union. Note that the union at the time (pre-June 21st, 2022)
        //       was causing occasional build issues with UE5 (w/ C++20) and thus scrapped for time being
        fp boxHalfSizeX{0}; // Positive halfwidth extents in each direction per axis from center point
        fp boxHalfSizeY{0};
        fp boxHalfSizeZ{0};
        fp capsuleHalfHeight{0}; // One half of total height of capsule, including rounded ends. Should be >= radius
        fp radius{0};            // Either radius of sphere OR radius of rounded ends of capsule

#pragma region Setters/"Constructors"

        void setBox(const FPVector& newCenter, const FPVector& halfSize) {
            setBox(newCenter, FPQuat::identity(), halfSize);
        }

        void setBox(const FPVector& newCenter, const FPQuat& newRotation, const FPVector& halfSize) {
            colliderType = ColliderType::Box;
            setCenter(newCenter);
            setRotation(newRotation);

            setBoxHalfSize(halfSize);
        }

        void setCapsule(FPVector newCenter, fp newRadius, fp halfHeight) {
            setCapsule(newCenter, FPQuat::identity(), newRadius, halfHeight);
        }

        /**
        * Setup capsule based on "point A" and "point B" positions. ie, where is the base sphere and where is the final
        * sphere centers
        * @param pointA - start/bottom sphere center
        * @param pointB - end/top sphere center
        * @param newRadius - radius of spheres/capsule
        **/
        void setCapsule(FPVector pointA, FPVector pointB, fp newRadius) {
            // Center is halfway position between provided points
            FPVector newCenter = (pointA + pointB) / fp{2};
            
            // Full height = distance between points plus buffer room (radius) on either side of capsule endpoint spheres
            fp fullHeight = FPVector::distance(pointA, pointB) + newRadius *  2;
            
            // Rotation is just direction from A to B, where upwards direction is standard "no rotation" for capsules
            FPVector aToBDir = FPVector::direction(pointA, pointB);
            FPQuat newRotation = FPMath2::dirVectorToQuat(aToBDir, FPVector::up());

            // Finally set values with chosen standard format
            setCapsule(newCenter, newRotation, newRadius, fullHeight / 2);
        }

        void setCapsule(FPVector newCenter, FPQuat newRotation, fp newRadius, fp halfHeight) {
            colliderType = ColliderType::Capsule;
            setCenter(newCenter);
            setRotation(newRotation);

            setCapsuleRadius(newRadius);
            setCapsuleHalfHeight(halfHeight);
        }

        void setSphere(FPVector newCenter, fp newRadius) {
            colliderType = ColliderType::Sphere;
            setCenter(newCenter);
            // No need to set rotation as rotation is useless for sphere

            setSphereRadius(newRadius);
        }

#pragma endregion
#pragma region Is Type Helpers

        bool isNotInitialized() const {
            return colliderType == ColliderType::NotInitialized;
        }

        bool isBox() const {
            return colliderType == ColliderType::Box;
        }

        bool isCapsule() const {
            return colliderType == ColliderType::Capsule;
        }

        bool isSphere() const {
            return colliderType == ColliderType::Sphere;
        }

#pragma endregion
#pragma region Setters/Getters

        void setCenter(const FPVector& newCenter) {
            center = newCenter;
        }

        FPVector getCenter() const {
            return center;
        }

        void setRotation(const FPQuat& newRotation) {
            rotation = newRotation;
        }

        FPQuat getRotation() const {
            return rotation;
        }

        void setBoxHalfSize(const FPVector& newHalfSize) {
            if (!isBox()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            boxHalfSizeX = newHalfSize.x;
            boxHalfSizeY = newHalfSize.y;
            boxHalfSizeZ = newHalfSize.z;
        }

        FPVector getBoxHalfSize() const {
            if (!isBox()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return FPVector::zero();
            }

            return {
                boxHalfSizeX, boxHalfSizeY, boxHalfSizeZ
            };
        }

        void setCapsuleRadius(const fp& newRadius) {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            radius = newRadius;
        }

        fp getCapsuleRadius() const {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return fp{0};
            }

            return radius;
        }

        void setCapsuleHalfHeight(const fp& newHalfHeight) {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            capsuleHalfHeight = newHalfHeight;
        }

        fp getCapsuleHalfHeight() const {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return fp{0};
            }

            return capsuleHalfHeight;
        }

        void setSphereRadius(const fp& newRadius) {
            if (!isSphere()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            radius = newRadius;
        }

        fp getSphereRadius() const {
            if (!isSphere()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return fp{0};
            }

            return radius;
        }

#pragma endregion
#pragma region Shared Utility Functions

        FPVector toWorldSpaceFromLocal(const FPVector& value) const {
            // Essentially reverse of toLocalSpaceFromWorld. Note order of operations!
            FPVector rotatedValue = toWorldSpaceForOriginCenteredValue(value);
            return rotatedValue + center;
        }

        FPVector toWorldSpaceForOriginCenteredValue(const FPVector& value) const {
            // This is its own separate public method as useful for directions
            // (ie, directions are always with origin at 0, just need to rotate em between world vs local)

            // Quick wayyyy later note off top of head:
            // This is assuming <0, 0, 0> is center of world. I think
            return rotation * value;
        }

        /// <summary>
        /// Turns a point in world space to collider's local space.
        /// If value represents a direction, then instead use toLocalSpaceForOriginCenteredValue
        /// </summary>
        FPVector toLocalSpaceFromWorld(const FPVector& value) const {
            FPVector valueAsDisplacementFromBoxOrigin = value - center;
            return toLocalSpaceForOriginCenteredValue(valueAsDisplacementFromBoxOrigin);
        }

        /// <summary>
        /// Turns a direction or point in world space that's centered on origin to collider's local space.
        /// Use this directly with directions. For points, recommended to use toLocalSpaceFromWorld instead
        /// </summary>
        // TODO: Refactor these methods into public toLocalSpaceFromWorldPosition and toLocalSpaceFromWorldDirection
        FPVector toLocalSpaceForOriginCenteredValue(const FPVector& value) const {
            // This is its own separate public method as useful for directions
            // (ie, directions are always with origin at 0, just need to rotate em between world vs local)

            // Quick wayyyy later note off top of head:
            // This is assuming <0, 0, 0> is center of world. TODO: Redo comments on this and prior two functions
            return rotation.inverted() * value;
        }

        // Return more or less rough estimate of bounds on horizontal plane
        fp GetHorizontalPlaneBoundsRadius() const {
            switch (colliderType) {
                case ColliderType::Box:
                    return getBoxHalfSize().x; // TODO: Account for when x and y aren't the same
                case ColliderType::Capsule:
                    return getCapsuleRadius();
                case ColliderType::Sphere:
                    return getSphereRadius();
                default:
                    // Would be nice to log some kinda message here but also not really necessary in context?
                    return fp{0};
            }
        }

        fp GetVerticalHalfHeightBounds() const {
            switch (colliderType) {
                case ColliderType::Box:
                    return getBoxHalfSize().z;
                case ColliderType::Capsule:
                    return getCapsuleHalfHeight();
                case ColliderType::Sphere:
                    return getSphereRadius();
                default:
                    // Would be nice to log some kinda message here but also not really necessary in context?
                    return fp{0};
            }
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            center.CalculateCRC32(resultThusFar);
            rotation.CalculateCRC32(resultThusFar);

            // Only make checksum based on values in use, which depends on collider type.
            // This is (supposedly) necessary as constructors and setters are designed to NOT set other values.
            switch (colliderType) {
                case ColliderType::Box:
                    resultThusFar = CRC::Calculate(&boxHalfSizeX, sizeof(boxHalfSizeX), CRC::CRC_32(), resultThusFar);
                    resultThusFar = CRC::Calculate(&boxHalfSizeY, sizeof(boxHalfSizeY), CRC::CRC_32(), resultThusFar);
                    resultThusFar = CRC::Calculate(&boxHalfSizeZ, sizeof(boxHalfSizeZ), CRC::CRC_32(), resultThusFar);
                    break;

                case ColliderType::Capsule:
                    resultThusFar = CRC::Calculate(&capsuleHalfHeight, sizeof(capsuleHalfHeight), CRC::CRC_32(), resultThusFar);
                    resultThusFar = CRC::Calculate(&radius, sizeof(radius), CRC::CRC_32(), resultThusFar);
                    break;

                case ColliderType::Sphere:
                    resultThusFar = CRC::Calculate(&radius, sizeof(radius), CRC::CRC_32(), resultThusFar);
                    break;

                default:
                    // Would be nice to log some kinda message here but also not really necessary in context?
                    return;
            }
        }

        std::string toString() const {
            switch (colliderType) {
                case ColliderType::NotInitialized:
                    return "<Not Initialized Collider>";

                case ColliderType::Box:
                    return "Box center: {" + center.toString()
                        + "}, rotation: {" + rotation.toString()
                        + "}, halfSize: {" + getBoxHalfSize().toString()
                        + "}";

                case ColliderType::Capsule:
                    return "Capsule center: {" + center.toString()
                        + "}, rotation: {" + rotation.toString()
                        + "}, radius: {" + std::to_string(static_cast<float>(radius))
                        + "}, halfHeight: {" + std::to_string(
                            static_cast<float>(capsuleHalfHeight))
                        + "}";

                case ColliderType::Sphere:
                    return "Sphere center: {" + center.toString()
                        + "}, radius: {" + std::to_string(static_cast<float>(radius))
                        + "}";

                default:
                    return "<Unknown Collider Type. Implement please!>";
            }
        }

        std::string getTypeAsString() const {
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

#pragma endregion

#pragma region Box Specific Functionality

        std::vector<FPVector> getBoxVerticesInWorldCoordinates() const {
            std::vector<FPVector> result;
            FPVector halfSize = getBoxHalfSize();

            // First, the bottom back left and top front right points
            //  ...yeah I do regret these location names. IDEA: Put names somewhere central, like in CollisionHelpers.h
            result.push_back(center + rotation * -halfSize);
            result.push_back(center + rotation * halfSize);

            // Then calculate all the other combinations one by one
            result.push_back(center + rotation * FPVector(-halfSize.x, halfSize.y, -halfSize.z)); // bottom back right
            result.push_back(center + rotation * FPVector(halfSize.x, -halfSize.y, -halfSize.z)); // bottom front left
            result.push_back(center + rotation * FPVector(halfSize.x, halfSize.y, -halfSize.z)); // bottom front right
            result.push_back(center + rotation * FPVector(-halfSize.x, -halfSize.y, halfSize.z)); // top back left
            result.push_back(center + rotation * FPVector(-halfSize.x, halfSize.y, halfSize.z)); // top back right
            result.push_back(center + rotation * FPVector(halfSize.x, -halfSize.y, halfSize.z)); // top front left

            return result;
        }

        // TODO: Not vastly important due to small fixed size, but preferably don't copy results. Instead pass in a vector to fill with data
        std::vector<FPVector> getBoxNormalsInWorldCoordinates() const {
            std::vector<FPVector> results;

            // Use x/y/z axes and rotate as necessary to get world coordinate axes
            // In addition, no need currently for parallel normals (eg, -x and +x)
            results.push_back(toWorldSpaceForOriginCenteredValue(FPVector::forward()));
            results.push_back(toWorldSpaceForOriginCenteredValue(FPVector::right()));
            results.push_back(toWorldSpaceForOriginCenteredValue(FPVector::up()));

            return results;
        }

        bool isWorldSpacePtWithinBoxIncludingOnSurface(const FPVector& point) const {
            FPVector localSpacePt = toLocalSpaceFromWorld(point);
            return isLocalSpacePtWithinBoxIncludingOnSurface(localSpacePt);
        }

        bool isLocalSpacePtWithinBoxIncludingOnSurface(const FPVector& localPoint) const {
            FPVector halfSize = getBoxHalfSize();

            // If outside bounds of an axis, then know for sure not located in the obb
            if (localPoint.x < -halfSize.x || localPoint.x > halfSize.x) return false;
            if (localPoint.y < -halfSize.y || localPoint.y > halfSize.y) return false;
            if (localPoint.z < -halfSize.z || localPoint.z > halfSize.z) return false;

            // Within bounds of all three axes. Thus, point is definitely located within the obb
            return true;
        }

        bool isWorldSpacePtWithinBoxExcludingOnSurface(const FPVector& point) const {
            FPVector localSpacePt = toLocalSpaceFromWorld(point);
            return isLocalSpacePtWithinBoxExcludingOnSurface(localSpacePt);
        }

        bool isLocalSpacePtWithinBoxExcludingOnSurface(const FPVector& localPoint) const {
            // Check if outside box (including surface) entirely
            if (!isLocalSpacePtWithinBoxIncludingOnSurface(localPoint)) {
                return false;
            }
            
            // Check if point on surface
            // Note that this is defined by any coordinate being on (+/-) max extent for that axis
            //      (and already checked that all coordinates are within range of box axes)
            // Easy to see if map out coordinates for each of the 6 faces of a box
            FPVector halfSize = getBoxHalfSize();
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
        void getFacesThatLocalSpacePointTouches(const FPVector& localPoint, std::vector<FPVector>& resultFaces) const {
            FPVector maxExtents = getBoxHalfSize();
            FPVector minExtents = -maxExtents;

            // Approach is very simple:
            // Input point is known to be already on surface of or within box (due to input requirements).
            // Thus, if a coordinate reaches the extents of that axis, then it touches that corresponding face.
            // For example: If point is on top forward right vertex (+x, +y, +z),
            //              then it's touching front (+x), right (+y), AND +z faces!

            // Side note, I stumbled on this solution after writing out the hard way via if statements. Was very obvious
            //      what the "optimal" solutions was after all that work, which seems to be a very common pattern...
            
            if (FPMath::isNear(localPoint.x, maxExtents.x, fp{0.001f})) {
                resultFaces.push_back(FPVector::forward());
            }
            else if (FPMath::isNear(localPoint.x, minExtents.x, fp{0.001f})) {
                resultFaces.push_back(FPVector::backward());
            }

            if (FPMath::isNear(localPoint.y, maxExtents.y, fp{0.001f})) {
                resultFaces.push_back(FPVector::right());
            }
            else if (FPMath::isNear(localPoint.y, minExtents.y, fp{0.001f})) {
                resultFaces.push_back(FPVector::left());
            }

            if (FPMath::isNear(localPoint.z, maxExtents.z, fp{0.001f})) {
                resultFaces.push_back(FPVector::up());
            }
            else if (FPMath::isNear(localPoint.z, minExtents.z, fp{0.001f})) {
                resultFaces.push_back(FPVector::down());
            }
        }

#pragma endregion
#pragma region Capsule Specific Functionality

        fp getMedialHalfLineLength() const {
            return getCapsuleHalfHeight() - getCapsuleRadius();
        }

        /// <summary>
        /// Gets extreme "center" points of capsule. ie, tip and base of capsule offset by radius along medial line
        /// Visually, points A and B on this picture: https://turanszkij.files.wordpress.com/2020/04/capsule-1.png
        /// </summary>
        /// <returns>Returns medial line extremes of capsule (either end of center line of capsule offset by radius)</returns>        
        Line getCapsuleMedialLineExtremes() const {
            fp pointDistanceFromCenter = getMedialHalfLineLength();

            // Calculate rotated directions towards "bottom" and "top"
            // Note that this is dependent on capsule's default orientation being vertical
            FPVector rotatedUpDir = rotation * FPVector::up();
            FPVector rotatedDownDir = rotatedUpDir * fp{-1};

            // Calculate the extreme points then return em
            FPVector pointA = center + rotatedDownDir * pointDistanceFromCenter;
            // Small optimization note: Don't need rotatedDownDir. Just subtract
            FPVector pointB = center + rotatedUpDir * pointDistanceFromCenter;
            return {pointA, pointB};
        }

#pragma endregion
#pragma region Sphere Specific Functionality

        // None, yay simple shape =D

#pragma endregion
    };

    inline std::ostream& operator<<(std::ostream& os, const Collider& value) {
        os << "Collider<" << value.toString() << ">";
        return os;
    }
}
