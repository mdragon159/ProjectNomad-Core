#include "pch.h"
#include "TestHelpers.h"

void TestHelpers::assertNear(fp expected, fp actual, fp range) {
    ASSERT_NEAR(static_cast<float>(expected), static_cast<float>(actual), static_cast<float>(range));
}

void TestHelpers::expectNear(fp expected, fp actual, fp range) {
    EXPECT_NEAR(static_cast<float>(expected), static_cast<float>(actual), static_cast<float>(range));
}

void TestHelpers::assertNear(FPVector expected, FPVector actual, fp range) {
    ASSERT_NEAR(static_cast<float>(expected.x), static_cast<float>(actual.x), static_cast<float>(range));
    ASSERT_NEAR(static_cast<float>(expected.y), static_cast<float>(actual.y), static_cast<float>(range));
    ASSERT_NEAR(static_cast<float>(expected.z), static_cast<float>(actual.z), static_cast<float>(range));
}

void TestHelpers::expectNear(FPVector expected, FPVector actual, fp range) {
    EXPECT_NEAR(static_cast<float>(expected.x), static_cast<float>(actual.x), static_cast<float>(range));
    EXPECT_NEAR(static_cast<float>(expected.y), static_cast<float>(actual.y), static_cast<float>(range));
    EXPECT_NEAR(static_cast<float>(expected.z), static_cast<float>(actual.z), static_cast<float>(range));
}

void TestHelpers::expectEq(FPVector expected, FPVector actual) {
    expectNear(expected, actual, fp{0});
}


void TestHelpers::verifyErrorsLogged(TestLogger& logger) {
    EXPECT_TRUE(logger.didLoggingOccur());
}

void TestHelpers::verifyNoErrorsLogged(TestLogger& logger) {
    EXPECT_FALSE(logger.didLoggingOccur());
}