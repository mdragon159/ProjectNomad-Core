#include "pchNCT.h"

#include "Physics/Collider.h"
#include "Physics/CollisionHelpers.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace CollisionHelpersTests {
    TEST(getIntersectionDistAlongAxis, whenNoVertices_throwsException) {
        std::vector<FPVector> boxAVertices;
        std::vector<FPVector> boxBVertices;
        FPVector testAxis{fp{0}, fp{0}, fp{1}};

        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenADoesNotHaveEnoughVertices_throwsException) {
        FPVector testAxis{fp{0}, fp{0}, fp{1}};
        
        Collider collider;
        collider.setBox(FPVector::zero(), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = collider.getBoxVerticesInWorldCoordinates();
        std::vector<FPVector> boxBVertices = collider.getBoxVerticesInWorldCoordinates();

        boxAVertices.pop_back();

        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenBHasTooManyVertices_throwsException) {
        FPVector testAxis{fp{0}, fp{0}, fp{1}};
        
        Collider collider;
        collider.setBox(FPVector::zero(), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = collider.getBoxVerticesInWorldCoordinates();
        std::vector<FPVector> boxBVertices = collider.getBoxVerticesInWorldCoordinates();

        boxBVertices.push_back({fp{0}, fp{0}, fp{0}});

        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenUsingZeroAxis_throwsException) {
        Collider boxA;
        boxA.setBox(FPVector::zero(), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(fp{-10}, fp{-10}, fp{-10}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        FPVector testAxis{fp{0}, fp{0}, fp{0}};
        ASSERT_DEATH(
            CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis),
            ""
        );
    }

    TEST(getIntersectionDistAlongAxis, whenUsingXTestAxis_whenSimpleBoxesAreTouching_returnsZeroIntersectionDist) {
        FPVector testAxis(fp{1}, fp{0}, fp{0});

        Collider boxA;
        boxA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();
        
        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_EQ(fp{0}, result);
    }

    TEST(getIntersectionDistAlongAxis, whenUsingXTestAxis_whenSimpleBoxesAreIntersecting_returnsPositiveIntersectionDepth) {
        FPVector testAxis(fp{1}, fp{0}, fp{0});

        Collider boxA;
        boxA.setBox(FPVector(fp{-0.9f}, fp{-1}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_TRUE(result > fp{0});
    }

    TEST(CollisionHelpers, whenUsingYTestAxis_whenSimpleBoxesAreIntersecting_returnsPositiveIntersectionDepth) {
        FPVector testAxis(fp{0}, fp{1}, fp{0});

        Collider boxA;
        boxA.setBox(FPVector(fp{-1}, fp{-0.5f}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(fp{1}, fp{0.5f}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_TRUE(result > fp{0});
    }

    TEST(CollisionHelpers, whenUsingZTestAxis_whenSimpleBoxesAreDistant_returnsNegativeIntersectionDepth) {
        FPVector testAxis(fp{0}, fp{0}, fp{1});

        Collider boxA;
        boxA.setBox(FPVector(fp{-1}, fp{-1}, fp{-2}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxAVertices = boxA.getBoxVerticesInWorldCoordinates();

        Collider boxB;
        boxB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        std::vector<FPVector> boxBVertices = boxB.getBoxVerticesInWorldCoordinates();

        fp result = CollisionHelpers::getIntersectionDistAlongAxis(boxAVertices, boxBVertices, testAxis);
        ASSERT_TRUE(result < fp{0});
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointAheadOfEndOfSegment_returnsExpectedDistance) {
        Line line(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{0}, fp{7}, fp{0});

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = fp{4};
        TestHelpers::assertNear(expectedDistSquared, distSquared, fp{0.01f});
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointBeforeStartOfSegment_returnsExpectedDistance) {
        Line line(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{-1}, fp{-6}, fp{0});

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = fp{2};
        TestHelpers::assertNear(expectedDistSquared, distSquared, fp{0.01f});
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointToSideOfSegment_returnsExpectedDistance) {
        Line line(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{-1}, fp{-5}, fp{0});

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = fp{1};
        TestHelpers::assertNear(expectedDistSquared, distSquared, fp{0.01f});
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointNearMiddleOfSegmentButNotTouching_returnsExpectedDistance) {
        Line line(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{-1}, fp{0}, fp{-1});

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = fp{2};
        TestHelpers::assertNear(expectedDistSquared, distSquared, fp{0.01f});
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenPointOnCenterOfSegment_returnsExpectedDistance) {
        Line line(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{0}, fp{0}, fp{0});

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = fp{0};
        TestHelpers::assertNear(expectedDistSquared, distSquared, fp{0.01f});
    }

    TEST(getSquaredDistBetweenPtAndSegment, whenSegmentDegeneratesIntoPoint_returnsExpectedDistance) {
        FPVector lineLocation(fp{-1}, fp{2}, fp{3});
        Line line(lineLocation, lineLocation);
        FPVector point(fp{0}, fp{0}, fp{0});

        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(line, point);

        fp expectedDistSquared = fp{14};
        TestHelpers::assertNear(expectedDistSquared, distSquared, fp{0.01f});
    }

    TEST(getClosestPtsBetweenTwoSegments, whenBothSegmentsArePoints_andNotSamePoint_thenReturnsExpectedResults) {
        FPVector firstPt(fp{-1}, fp{-1}, fp{-1});
        Line firstLine(firstPt, firstPt);
        FPVector secondPt(fp{0}, fp{0}, fp{0});
        Line secondLine(secondPt, secondPt);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{3}, distSquared);
        ASSERT_EQ(fp{0}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(firstPt, closestPointOnFirstSegment);
        ASSERT_EQ(secondPt, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenBothSegmentsArePoints_andSamePoint_thenReturnsExpectedResults) {
        FPVector point(fp{-1}, fp{-1}, fp{-1});
        Line firstLine(point, point);
        Line secondLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{0}, distSquared);
        ASSERT_EQ(fp{0}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(point, closestPointOnFirstSegment);
        ASSERT_EQ(point, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenOnlyOneSegmentIsAPoint_andNoOverlap_thenReturnsExpectedResults) {
        Line normalLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{-1}, fp{0}, fp{-1});
        Line pointLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            normalLine, pointLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{2}, distSquared);
        ASSERT_EQ(fp{0.5f}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(point, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenOnlyOneSegmentIsAPoint_andOverlap_thenReturnsExpectedResults) {
        Line normalLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{0}, fp{-2}, fp{0});
        Line pointLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            normalLine, pointLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{0}, distSquared);
        ASSERT_EQ(fp{0.3f}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0}, timeOfClosestPointForSecondSegment);
        TestHelpers::assertNear(point, closestPointOnFirstSegment, fp{0.01f});
        TestHelpers::assertNear(point, closestPointOnSecondSegment, fp{0.01f});
    }

    TEST(getClosestPtsBetweenTwoSegments, whenFirstSegmentIsAPoint_andOverlap_thenReturnsExpectedResults) {
        Line normalLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector point(fp{0}, fp{-2}, fp{0});
        Line pointLine(point, point);

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            pointLine, normalLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{0}, distSquared);
        ASSERT_EQ(fp{0}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0.3f}, timeOfClosestPointForSecondSegment);
        TestHelpers::assertNear(point, closestPointOnFirstSegment, fp{0.01f});
        TestHelpers::assertNear(point, closestPointOnSecondSegment, fp{0.01f});
    }

    TEST(getClosestPtsBetweenTwoSegments, whenParallelLines_andNoOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        Line secondLine(FPVector(fp{5}, fp{0}, fp{5}), FPVector(fp{5}, fp{10}, fp{5}));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{50}, distSquared);
        ASSERT_EQ(fp{0.5f}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(secondLine.start, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenParallelLines_andOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        Line secondLine(FPVector(fp{0}, fp{-10}, fp{0}), FPVector(fp{0}, fp{10}, fp{0}));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{0}, distSquared);
        ASSERT_EQ(fp{0}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0.25f}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(firstLine.start, closestPointOnFirstSegment);
        ASSERT_EQ(firstLine.start, closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenNotParallelLines_andOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        Line secondLine(FPVector(fp{5}, fp{0}, fp{0}), FPVector(fp{-5}, fp{0}, fp{0}));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{0}, distSquared);
        ASSERT_EQ(fp{0.5f}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0.5f}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnSecondSegment);
    }

    TEST(getClosestPtsBetweenTwoSegments, whenNotParallelLines_andNoOverlap_thenReturnsExpectedResults) {
        Line firstLine(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        Line secondLine(FPVector(fp{5}, fp{0}, fp{10}), FPVector(fp{-5}, fp{0}, fp{10}));

        fp timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment;
        FPVector closestPointOnFirstSegment, closestPointOnSecondSegment;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            firstLine, secondLine,
            timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
            closestPointOnFirstSegment, closestPointOnSecondSegment
        );

        ASSERT_EQ(fp{100}, distSquared);
        ASSERT_EQ(fp{0.5f}, timeOfClosestPointForFirstSegment);
        ASSERT_EQ(fp{0.5f}, timeOfClosestPointForSecondSegment);
        ASSERT_EQ(FPVector::zero(), closestPointOnFirstSegment);
        ASSERT_EQ(FPVector(fp{0}, fp{0}, fp{10}), closestPointOnSecondSegment);
    }

    TEST(getClosestPtBetweenPtAndSegment, whenPointIsStartOfLine_thenReturnsStartingPoint) {
        Line segment(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector testPoint = FPVector{fp{0}, fp{-5}, fp{0}};

        FPVector closestPtResult;
        fp timeOfIntersection;
        CollisionHelpers::getClosestPtBetweenPtAndSegment(segment, testPoint, timeOfIntersection, closestPtResult);

        TestHelpers::expectNear(fp{0}, timeOfIntersection, fp{0.001f});
        TestHelpers::expectNear(testPoint, closestPtResult, fp{0.01f});
    }

    TEST(getClosestPtBetweenPtAndSegment, whenPointIsEndOfLine_thenReturnsEndPoint) {
        Line segment(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector testPoint = FPVector{fp{0}, fp{5}, fp{0}};

        FPVector closestPtResult;
        fp timeOfIntersection;
        CollisionHelpers::getClosestPtBetweenPtAndSegment(segment, testPoint, timeOfIntersection, closestPtResult);

        TestHelpers::expectNear(fp{1}, timeOfIntersection, fp{0.001f});
        TestHelpers::expectNear(testPoint, closestPtResult, fp{0.01f});
    }

    TEST(getClosestPtBetweenPtAndSegment, whenPointIsBeforeStartOfLine_thenReturnsStartPoint) {
        Line segment(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector testPoint = FPVector{fp{1}, fp{-20}, fp{-2}};

        FPVector closestPtResult;
        fp timeOfIntersection;
        CollisionHelpers::getClosestPtBetweenPtAndSegment(segment, testPoint, timeOfIntersection, closestPtResult);

        TestHelpers::expectNear(fp{0}, timeOfIntersection, fp{0.001f});
        TestHelpers::expectNear(segment.start, closestPtResult, fp{0.01f});
    }

    TEST(getClosestPtBetweenPtAndSegment, whenPointIsPastEndOfLine_thenReturnsEndPoint) {
        Line segment(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector testPoint = FPVector{fp{0}, fp{10}, fp{0}};

        FPVector closestPtResult;
        fp timeOfIntersection;
        CollisionHelpers::getClosestPtBetweenPtAndSegment(segment, testPoint, timeOfIntersection, closestPtResult);

        TestHelpers::expectNear(fp{1}, timeOfIntersection, fp{0.001f});
        TestHelpers::expectNear(segment.end, closestPtResult, fp{0.01f});
    }

    TEST(getClosestPtBetweenPtAndSegment, whenPointIsToSideOfLineCenter_thenReturnsCenterPoint) {
        Line segment(FPVector(fp{0}, fp{-5}, fp{0}), FPVector(fp{0}, fp{5}, fp{0}));
        FPVector testPoint = FPVector{fp{0}, fp{0}, fp{5}};

        FPVector closestPtResult;
        fp timeOfIntersection;
        CollisionHelpers::getClosestPtBetweenPtAndSegment(segment, testPoint, timeOfIntersection, closestPtResult);

        TestHelpers::expectNear(fp{0.5f}, timeOfIntersection, fp{0.001f});
        TestHelpers::expectNear(FPVector::zero(), closestPtResult, fp{0.01f});
    }

    // TODO: getWorldVerticesFromOBB w/ more complex rotations and adjusted center points
    // TODO: More box axis intersection tests, especially with non-basic axes and rotated boxes
}