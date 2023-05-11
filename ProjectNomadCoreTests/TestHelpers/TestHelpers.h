#pragma once

#include "TestLogger.h"
#include "Math/FPVector.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

// Starting to see why "using namespace" and these includes ain't in the header file... (slowness and finally what "poisoning" namespace means)
// TODO: Replace with forward declaration in cpp file
using namespace ProjectNomad;

static float toFloat(fp value) {
    return static_cast<float>(value);
}

class TestHelpers {
public:
    static void assertNear(fp expected, fp actual, fp range);
    static void expectNear(fp expected, fp actual, fp range);

    static void assertNear(FPVector expected, FPVector actual, fp range);
    static void expectNear(FPVector expected, FPVector actual, fp range);

    static void assertNear(FPQuat expected, FPQuat actual, fp range);
    static void expectNear(FPQuat expected, FPQuat actual, fp range);
    
    static void expectEq(FPVector expected, FPVector actual);

    static void verifyErrorsLogged(TestLogger& logger);
    static void verifyNoErrorsLogged(TestLogger& logger);

    static void VerifySingletonLoggingOccured();
    static void VerifySingleLoggingDidNotOccur();

    static void EmptySingletonLogger();
};

class BaseSimTest : public ::testing::Test {
protected:
    TestLogger testLogger;
    
    void SetUp() override {}

    void TearDown() override {
        TestHelpers::verifyNoErrorsLogged(testLogger);

        TestHelpers::VerifySingleLoggingDidNotOccur();
        TestHelpers::EmptySingletonLogger(); // Don't want state from one test to carry on to next
    }

    // Shortcut method to save a bit of time
    static LoggerSingleton& GetLoggerSingleton() {
        return Singleton<LoggerSingleton>::get();
    }
};