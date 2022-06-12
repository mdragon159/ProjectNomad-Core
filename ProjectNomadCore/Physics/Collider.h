#pragma once

#include "Ray.h"
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

        // Avoid needing to use pointers via dangerous union type, see following for quick explanation regarding union:
        // https://stackoverflow.com/a/3071899/3735890
        // This allows us to not have redundant/unused data across all types, but need to be careful with usage
        // Oof, also union with 3 floats which Unreal does -_- Not using FPVector as not sure if composite types works well here
        union SharedColliderData {
            struct {
                fp halfSizeX{0}; // Positive halfwidth extents in each direction per axis from center point
                fp halfSizeY{0};
                fp halfSizeZ{0};
            } Box;

            struct {
                fp radius{0}; // Radius of rounded ends of capsule
                fp halfHeight{0}; // One half of total height of capsule, including rounded ends. Should be >= radius
            } Capsule;

            struct {
                fp radius{0};
            } Sphere;

            // Using a named union and explicit constructors due to fp not having a default constructor
            // See following for more info: https://www.codeproject.com/Answers/1187708/Having-problems-with-union-forcing-initiation-of-a#answer1
            SharedColliderData() : Box(), Capsule(), Sphere() {}
        };

        SharedColliderData sharedCollisionData;

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

        void setCapsule(FPVector newCenter, fp radius, fp halfHeight) {
            setCapsule(newCenter, FPQuat::identity(), radius, halfHeight);
        }

        void setCapsule(FPVector newCenter, FPQuat newRotation, fp radius, fp halfHeight) {
            colliderType = ColliderType::Capsule;
            setCenter(newCenter);
            setRotation(newRotation);

            setCapsuleRadius(radius);
            setCapsuleHalfHeight(halfHeight);
        }

        void setSphere(FPVector newCenter, fp radius) {
            colliderType = ColliderType::Sphere;
            setCenter(newCenter);
            // No need to set rotation as rotation is useless for sphere

            setSphereRadius(radius);
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

            sharedCollisionData.Box.halfSizeX = newHalfSize.x;
            sharedCollisionData.Box.halfSizeY = newHalfSize.y;
            sharedCollisionData.Box.halfSizeZ = newHalfSize.z;
        }

        FPVector getBoxHalfSize() const {
            if (!isBox()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return FPVector::zero();
            }

            return {
                sharedCollisionData.Box.halfSizeX, sharedCollisionData.Box.halfSizeY, sharedCollisionData.Box.halfSizeZ
            };
        }

        void setCapsuleRadius(const fp& newRadius) {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            sharedCollisionData.Capsule.radius = newRadius;
        }

        fp getCapsuleRadius() const {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return fp{0};
            }

            return sharedCollisionData.Capsule.radius;
        }

        void setCapsuleHalfHeight(const fp& newHalfHeight) {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            sharedCollisionData.Capsule.halfHeight = newHalfHeight;
        }

        fp getCapsuleHalfHeight() const {
            if (!isCapsule()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return fp{0};
            }

            return sharedCollisionData.Capsule.halfHeight;
        }

        void setSphereRadius(const fp& newRadius) {
            if (!isSphere()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return;
            }

            sharedCollisionData.Sphere.radius = newRadius;
        }

        fp getSphereRadius() const {
            if (!isSphere()) {
                // TODO: Resolve circular dependencies (ie, use .cpp files) and take in SimContext for check and logging
                return fp{0};
            }

            return sharedCollisionData.Sphere.radius;
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
                        + "}, radius: {" + std::to_string(static_cast<float>(sharedCollisionData.Capsule.radius))
                        + "}, halfHeight: {" + std::to_string(
                            static_cast<float>(sharedCollisionData.Capsule.halfHeight))
                        + "}";

                case ColliderType::Sphere:
                    return "Sphere center: {" + center.toString()
                        + "}, radius: {" + std::to_string(static_cast<float>(sharedCollisionData.Sphere.radius))
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
