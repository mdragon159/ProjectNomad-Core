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

        // TODO: Not vastly important due to small fixed size, but don't copy vector on return. Pass in a vector to fill with data
        std::vector<FPVector> getBoxNormalsInWorldCoordinates() const {
            std::vector<FPVector> results;

            // Use x/y/z axes and rotate as necessary to get world coordinate axes
            // In addition, no need currently for parallel normals (eg, -x and +x)
            results.push_back(toWorldSpaceForOriginCenteredValue(FPVector::forward()));
            results.push_back(toWorldSpaceForOriginCenteredValue(FPVector::right()));
            results.push_back(toWorldSpaceForOriginCenteredValue(FPVector::up()));

            return results;
        }

        void getAllBoxFaceNormalsInLocalSpace(std::vector<FPVector>& results) const {
            results.push_back(FPVector::forward());
            results.push_back(FPVector::right());
            results.push_back(FPVector::up());

            results.push_back(FPVector::backward());
            results.push_back(FPVector::left());
            results.push_back(FPVector::down());
        }

        bool isWorldSpacePtWithinBox(const FPVector& point) const {
            FPVector localSpacePt = toLocalSpaceFromWorld(point);
            return isLocalSpacePtWithinBox(localSpacePt);
        }

        bool isLocalSpacePtWithinBox(const FPVector& localPoint) const {
            FPVector halfSize = getBoxHalfSize();

            // If outside bounds of an axis, then know for sure not located in the obb
            if (localPoint.x < -halfSize.x || localPoint.x > halfSize.x) return false;
            if (localPoint.y < -halfSize.y || localPoint.y > halfSize.y) return false;
            if (localPoint.z < -halfSize.z || localPoint.z > halfSize.z) return false;

            // Within bounds of all three axes. Thus, point is definitely located within the obb
            return true;
        }

        FPVector getPushFaceNormalForPtInWorldSpace(const FPVector& worldPoint) const {
            FPVector localSpaceResultDir = getPushFaceNormalForPtInLocalSpace(toLocalSpaceFromWorld(worldPoint));
            return toWorldSpaceForOriginCenteredValue(localSpaceResultDir);
        }

        // Collision resolution purpose: Have a point in space within box and want to find which direction to push it,
        //                                  such that it's the smallest direction to push the point out of the box
        FPVector getPushFaceNormalForPtInLocalSpace(const FPVector& localPoint) const {
            // Approach: Get which side of box center the point is on, then "project" that direction towards box face
            //              to find best face to push point out to
            //  1. Get direction from box center (origin in local space) to given point
            //  2. Dot direction with all normals and choose one highest value

            // Explicitly cover edge case just 'cuz arbitrarily want to define base case
            // Note that this isn't strictly necessary as loop approach should elegantly "fail"
            if (localPoint == FPVector::zero()) {
                // Default to wanting to push point towards "front" of box
                // Is this most appropriate behavior (eg, player gets stuck exactly in center of box)? Nah but dunno if care about that edge case much
                return FPVector::forward();
            }

            // Get direction from box center (origin) to local point
            // TODO: Normalization is MAYBE unnecessary, as this is all relative comparisons
            FPVector toPointDir = localPoint.normalized();

            // Get all normals to test with
            // TODO: Perhaps check edges too in future, but don't think that sort of accuracy is worth atm
            std::vector<FPVector> allFaceNormals;
            getAllBoxFaceNormalsInLocalSpace(allFaceNormals);

            // TODO: !!!!!!!!!!!
            // Need to take into account extents! Boxes are NOT squares, so need to account for that!

            // 1. Get all normals x distance to face plane
            // 2. Get normal with smallest distance to face plane
            // 3. Return normal with point when moved to face plane
            
            // Get normal which best points towards the provided input location
            FPVector resultDirection;
            fp bestDirectionSoFarValue = FPMath::maxLimit() * fp{-1};
            for (const auto& curNormal : allFaceNormals) {
                // Get value which represents how much this direction points towards provided input location
                fp curPointDirValue = toPointDir.dot(curNormal);
                
                // Remember if best normal thus far
                if (curPointDirValue > bestDirectionSoFarValue) {
                    resultDirection = curNormal;
                    bestDirectionSoFarValue = curPointDirValue;
                }
            }

            return resultDirection;
        }

        void pushPtToBoxFaceInWorldSpace(const FPVector& inPoint, FPVector& outPushedPoint, FPVector& outPushDir) {
            FPVector localSpaceInput = toLocalSpaceFromWorld(inPoint);

            FPVector localResultPt;
            FPVector localResultDir;
            pushPtToBoxFaceInLocalSpace(localSpaceInput, localResultPt, localResultDir);

            outPushedPoint = toWorldSpaceFromLocal(localResultPt);
            outPushDir = toWorldSpaceForOriginCenteredValue(localResultDir);
        }

        // Collision resolution purpose: Have a point in space within box and want to find which direction to push it,
        //                                  such that it's the smallest direction to push the point out of the box
        void pushPtToBoxFaceInLocalSpace(const FPVector& inPoint, FPVector& outPushedPoint, FPVector& outPushDir) {
            // Get best direction to push given point out to a face
            outPushDir = getPushFaceNormalForPtInLocalSpace(inPoint);

            // Figure out where the input point would be if it were actually pushed directly to a face
            // TODO: Not a fan of buncha if statements but too lazy to figure out some clean math with max points for this 
            outPushedPoint = inPoint;               // Pushing point to face in local space just means setting an axis to that axis's max distance
            FPVector maxExtents = getBoxHalfSize(); // For clarity that half size in local space = maximum extents on each axis
            if (outPushDir == FPVector::up()) {
                outPushedPoint.z = maxExtents.z;
            }
            else if (outPushDir == FPVector::down()) {
                outPushedPoint.z = maxExtents.z * fp{-1};
            }
            else if (outPushDir == FPVector::right()) {
                outPushedPoint.y = maxExtents.y;
            }
            else if (outPushDir == FPVector::left()) {
                outPushedPoint.y = maxExtents.y * fp{-1};
            }
            else if (outPushDir == FPVector::forward()) {
                outPushedPoint.x = maxExtents.x;
            }
            else if (outPushDir == FPVector::backward()) {
                outPushedPoint.x = maxExtents.x * fp{-1};
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
