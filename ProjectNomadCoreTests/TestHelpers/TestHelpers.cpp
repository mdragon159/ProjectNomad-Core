#include "pchNCT.h"
#include "TestHelpers.h"

#include "Utilities/Singleton.h"

void TestHelpers::assertNear(fp expected, fp actual, fp range) {
    float fExpected = static_cast<float>(expected);
    float fActual = static_cast<float>(actual);
    float fRange = static_cast<float>(range);
    ASSERT_NEAR(fExpected, fActual, fRange);
}

void TestHelpers::expectNear(fp expected, fp actual, fp range) {
    float fExpected = static_cast<float>(expected);
    float fActual = static_cast<float>(actual);
    float fRange = static_cast<float>(range);
    EXPECT_NEAR(fExpected, fActual, fRange);
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

    // Reset logging state as already checked. This allows us to NOT have to "override" verifyNoErrorsLogged calls in
    // relevant tests
    logger.resetLogging();
}

void TestHelpers::verifyNoErrorsLogged(TestLogger& logger) {
    EXPECT_FALSE(logger.didLoggingOccur());
}

void TestHelpers::VerifySingletonLoggingOccured() {
    LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
    EXPECT_TRUE(!logger.getDebugMessages().empty() || !logger.getNetLogMessages().empty());

    // Reset logging state as already checked. This allows us to NOT have to "override" VerifySingleLoggingDidNotOccur calls
    // in relevant tests
    EmptySingletonLogger();
}

void TestHelpers::VerifySingleLoggingDidNotOccur() {
    LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
    EXPECT_TRUE(logger.getDebugMessages().empty() && logger.getNetLogMessages().empty());
}

void TestHelpers::EmptySingletonLogger() {
    LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
    
    while (!logger.getDebugMessages().empty()) {
        logger.getDebugMessages().pop();
    }
    while (!logger.getNetLogMessages().empty()) {
        logger.getDebugMessages().pop();
    }
}
