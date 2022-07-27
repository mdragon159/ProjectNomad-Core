#include "pchNCT.h"
#include "Physics/Ray.h"

using namespace ProjectNomad;

/*
namespace RayTests {
    TEST(Ray, defaultConstructor_createsSimpleRay) {
        Ray ray;

        ASSERT_EQ(FPVector::zero(), ray.mOrigin);
        ASSERT_EQ(FPVector(0, 0, 1), ray.mDirection);
    }

    TEST(Ray, ctorWithOriginDirection_withSimpleRay_createsExpectedRay) {
        FPVector origin(0.5f, -1, 2);
        FPVector dir(0, -1, 0);
        Ray ray(origin, dir);

        ASSERT_EQ(origin, ray.mOrigin);
        ASSERT_EQ(dir, ray.mDirection);
    }

    TEST(Ray, ctorWithOriginDirection_withNotNormalDirection_createsExpectedRay) {
        FPVector origin(0.5f, -1, 2);
        FPVector dir(0, 0, 25);
        Ray ray(origin, dir);

        ASSERT_EQ(origin, ray.mOrigin);
        ASSERT_EQ(FPVector(0, 0, 1), ray.mDirection);
    }

    TEST(Ray, fromPoints_withSimplePoints_createsExpectedRay) {
        FPVector from(-1, 0, 0);
        FPVector to(200, 0, 0);
        Ray ray = Ray::fromPoints(from, to);

        ASSERT_EQ(from, ray.mOrigin);
        ASSERT_EQ(FPVector(1, 0, 0), ray.mDirection);
    }
}
*/