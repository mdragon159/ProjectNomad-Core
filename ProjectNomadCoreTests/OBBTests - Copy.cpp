#include "pchNCT.h"
#include "../SimLibrary/Physics/Sphere.h"

using namespace TopDownSimLibrary;

namespace SphereTests {
  TEST(Sphere, defaultConstructor_createsExpectedSphere) {
    Sphere sphere;

    FPVector defaultVector;
    ASSERT_EQ(defaultVector, sphere.mCenter);
    ASSERT_EQ(1, sphere.mRadius);
  }
  
  TEST(Sphere, parameterizedConstructor_whenGivenSomeValues_createsExpectedSphere) {
    FPVector center(-1, 0, 25.5);
    Sphere sphere(center, 32);

    ASSERT_EQ(center, sphere.mCenter);
    ASSERT_EQ(32, sphere.mRadius);
  }
}