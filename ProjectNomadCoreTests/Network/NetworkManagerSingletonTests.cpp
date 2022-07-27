#include "pchNCT.h"

#include "TestHelpers/TestHelpers.h"
#include "Network/NetworkManagerSingleton.h"

using namespace TopDownSimLibrary;

namespace NetworkManagerSingletonTests {
    class NetworkManagerSingletonTestsBase : public ::testing::Test {
    protected:
        SimContext* simContext = nullptr;

        void SetUp() override {
            simContext = TestHelpers::createSimContext();
        }

        void TearDown() override {
            TestHelpers::cleanUpSimContext(simContext);
        }
    };

    class NetworkManagerSingletonTests : public NetworkManagerSingletonTestsBase {
        void TearDown() override {
            TestHelpers::verifyNoErrorsLogged(simContext);
        }
    };

    TEST_F(NetworkManagerSingletonTests, initialize_justCallIt) {
        NetworkManagerSingleton networkManager(*simContext);
        networkManager.Initialize();
    }
}
