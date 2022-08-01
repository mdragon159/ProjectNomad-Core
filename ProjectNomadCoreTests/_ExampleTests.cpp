#include "pchNCT.h"

#include "Math/FixedPoint.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace ExampleTests {
    class ExampleTests : public BaseSimTest {};
    
    TEST_F(ExampleTests, SomethingElse) {
        fp test = fp{1};
        EXPECT_EQ(test, fp{1.f});
        EXPECT_TRUE(true);
    }
}
