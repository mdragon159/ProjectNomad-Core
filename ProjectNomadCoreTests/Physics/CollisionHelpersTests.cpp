#include "pch.h"

#include "Physics/Collider.h"
#include "Physics/CollisionHelpers.h"

using namespace ProjectNomad;

/*
namespace CollisionHelpersTests {
    TEST(getIntersectionDistAlongAxis, whenNoVertices_throwsException) {
        std::vector<FPVector> boxAVertices;
        std::vector<FPVector> boxBVertices;
        FPVector testAxis{0, 0, 1};

        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenADoesNotHaveEnoughVertices_throwsException) {
        FPVector testAxis{0, 0, 1};
        
        Collider collider;
        collider.setBox(FPVector::zero(), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = collider.getBoxVerticesInWorldCoordinates();
        std::vector<FPVector> boxBVertices = collider.getBoxVerticesInWorldCoordinates();

        boxAVertices.pop_back();

        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenBHasTooManyVertices_throwsException) {
        FPVector testAxis{0, 0, 1};
        
        Collider collider;
        collider.setBox(FPVector::zero(), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = collider.getBoxVerticesInWorldCoordinates();
        std::vector<FPVector> boxBVertices = collider.getBoxVerticesInWorldCoordinates();

        boxBVertices.push_back({0, 0, 0});

        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenUsingZeroAxis_throwsException) {
        Collider boxA;
        boxA.setBox(FPVector::zero(), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(-10, -10, -10), FPVector(1, 1, 1));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        FPVector testAxis{0, 0, 0};
        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenUsingXTestAxis_whenSimpleBoxesAreTouching_returnsZeroIntersectionDist) {
        FPVector testAxis(1, 0, 0);

        Collider boxA;
        boxA.setBox(FPVector(-1, -1, -1), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(1, 1, 1), FPVector(1, 1, 1));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();
        
        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_EQ(0, result);
    }

    TEST(getIntersectionDistAlongAxis, whenUsingXTestAxis_whenSimpleBoxesAreIntersecting_returnsPositiveIntersectionDepth) {
        FPVector testAxis(1, 0, 0);

        Collider boxA;
        boxA.setBox(FPVector(-0.9f, -1, -1), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(1, 1, 1), FPVector(1, 1, 1));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_TRUE(result > 0);
    }

    TEST(CollisionHelpers, whenUsingYTestAxis_whenSimpleBoxesAreIntersecting_returnsPositiveIntersectionDepth) {
        FPVector testAxis(0, 1, 0);

        Collider boxA;
        boxA.setBox(FPVector(-1, -0.5f, -1), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(1, 0.5f, 1), FPVector(1, 1, 1));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_TRUE(result > 0);
    }

    TEST(CollisionHelpers, whenUsingZTestAxis_whenSimpleBoxesAreDistant_returnsNegativeIntersectionDepth) {
        FPVector testAxis(0, 0, 1);

        Collider boxA;
        boxA.setBox(FPVector(-1, -1, -2), FPVector(1, 1, 1));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(1, 1, 1), FPVector(1, 1, 1));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_TRUE(result < 0);
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointAheadOfEndOfSegment_returnsExpectedDistance) {
        Line line(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(0, 7, 0);

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = 4;
        ASSERT_NEAR(expectedDistSquared, distSquared, 0.01f);
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointBeforeStartOfSegment_returnsExpectedDistance) {
        Line line(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(-1, -6, 0);

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = 2;
        ASSERT_NEAR(expectedDistSquared, distSquared, 0.01f);
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointToSideOfSegment_returnsExpectedDistance) {
        Line line(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(-1, -5, 0);

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = 1;
        ASSERT_NEAR(expectedDistSquared, distSquared, 0.01f);
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointNearMiddleOfSegmentButNotTouching_returnsExpectedDistance) {
        Line line(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(-1, 0, -1);

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = 2;
        ASSERT_NEAR(expectedDistSquared, distSquared, 0.01f);
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointOnCenterOfSegment_returnsExpectedDistance) {
        Line line(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(0, 0, 0);

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = 0;
        ASSERT_NEAR(expectedDistSquared, distSquared, 0.01f);
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenSegmentDegeneratesIntoPoint_returnsExpectedDistance) {
        FPVector lineLocation(-1, 2, 3);
        Line line(lineLocation, lineLocation);
        FPVector point(0, 0, 0);

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = 14;
        ASSERT_NEAR(expectedDistSquared, distSquared, 0.01f);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenBothSegmentsArePoints_andNotSamePoint_thenReturnsExpectedResults) {
        FPVector firstPt(-1, -1, -1);
        Line firstLine(firstPt, firstPt);
        FPVector secondPt(0, 0, 0);
        Line secondLine(secondPt, secondPt);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(3, distSquared);
        ASSERT_EQ(0, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(firstPt, closestPointOnFirstSegment);
        ASSERT_EQ(secondPt, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenBothSegmentsArePoints_andSamePoint_thenReturnsExpectedResults) {
        FPVector point(-1, -1, -1);
        Line firstLine(point, point);
        Line secondLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(0, distSquared);
        ASSERT_EQ(0, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(point, closestPointOnFirstSegment);
        ASSERT_EQ(point, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenOnlyOneSegmentIsAPoint_andNoOverlap_thenReturnsExpectedResults) {
        Line normalLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(-1, 0, -1);
        Line pointLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            normalLine, pointLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(2, distSquared);
        ASSERT_EQ(0.5f, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(point, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenOnlyOneSegmentIsAPoint_andOverlap_thenReturnsExpectedResults) {
        Line normalLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(0, -2, 0);
        Line pointLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            normalLine, pointLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(0, distSquared);
        ASSERT_EQ(0.3f, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(point, closestPointOnFirstSegment);
        ASSERT_EQ(point, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenFirstSegmentIsAPoint_andOverlap_thenReturnsExpectedResults) {
        Line normalLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        FPVector point(0, -2, 0);
        Line pointLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            pointLine, normalLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(0, distSquared);
        ASSERT_EQ(0, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0.3f, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(point, closestPointOnFirstSegment);
        ASSERT_EQ(point, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenParallelLines_andNoOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        Line secondLine(FPVector(5, 0, 5), FPVector(5, 10, 5));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(50, distSquared);
        ASSERT_EQ(0.5f, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(secondLine.mStart, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenParallelLines_andOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        Line secondLine(FPVector(0, -10, 0), FPVector(0, 10, 0));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(0, distSquared);
        ASSERT_EQ(0, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0.25f, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(firstLine.mStart, closestPointOnFirstSegment);
        ASSERT_EQ(firstLine.mStart, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenNotParallelLines_andOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        Line secondLine(FPVector(5, 0, 0), FPVector(-5, 0, 0));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(0, distSquared);
        ASSERT_EQ(0.5f, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0.5f, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenNotParallelLines_andNoOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(0, -5, 0), FPVector(0, 5, 0));
        Line secondLine(FPVector(5, 0, 10), FPVector(-5, 0, 10));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(100, distSquared);
        ASSERT_EQ(0.5f, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(0.5f, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(FPVector(0, 0, 10), closestPointOnSecondSegment);
    }

    // TODO: getWorldVerticesFromOBB w/ more complex rotations and adjusted center points
    // TODO: More box axis intersection tests, especially with non-basic axes and rotated boxes

    // TODO: ALL TESTS FOR...
    //     static void getClosestPtBetweenPtAndSegment(FPVector segmentStart, FPVector segmentEnd, FPVector point,
}
*/