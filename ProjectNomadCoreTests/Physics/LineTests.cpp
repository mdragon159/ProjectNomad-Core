#include "pchNCT.h"
#include "Physics/Line.h"

using namespace ProjectNomad;

/*
namespace LineTests {
    TEST(Line, defaultConstructor_createsSimpleLine) {
        Line line;

        FPVector zeroVec = FPVector::zero();
        ASSERT_EQ(zeroVec, line.mStart);
        ASSERT_EQ(zeroVec, line.mEnd);
    }

    TEST(Line, constructorWithStartEnd_createsExpectedLine) {
        FPVector start(-1, 0.5f, 1000);
        FPVector end(0, 0, -0.25f);
        Line line(start, end);

        ASSERT_EQ(start, line.mStart);
        ASSERT_EQ(end, line.mEnd);
    }

    TEST(Line, getLength_withSimpleLine_returnsExpectedLength) {
        FPVector start(-2, 0, 1);
        FPVector end(2, 0, 1);
        Line line(start, end);

        ASSERT_EQ(4, line.getLength());
    }

    TEST(Line, getLengthSquared_withSimpleLine_returnsExpectedLength) {
        FPVector start(-2, 0, 1);
        FPVector end(2, 0, 1);
        Line line(start, end);

        ASSERT_EQ(16, line.getLengthSquared());
    }
}
*/