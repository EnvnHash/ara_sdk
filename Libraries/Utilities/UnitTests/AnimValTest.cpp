//
// Created by sven on 02-04-25.
//

#include <UtilityUnitTestCommon.h>
#include "AnimVal.h"
#ifdef _WIN32
#include "Windows.h"
#endif

namespace ara::UtilitiesTest::AnimValTest {

TEST(AnimValTest, StartAnimation) {
    AnimVal<int> av;
    EXPECT_TRUE(av.stopped());
    av.start(0, 100, 0.2, false);
    EXPECT_FALSE(av.stopped());
}
    
TEST(AnimValTest, StopAnimation) {
    AnimVal<int> av;
    av.start(0, 100, 0.2, false);
    EXPECT_FALSE(av.stopped());
    av.stop();
    EXPECT_TRUE(av.stopped());
}

TEST(AnimValTest, UpdateLinearFunction) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    AnimVal<int> av;
    av.start(0, 100, 1.0, false);

    // Simulate passing time
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    av.update();
    EXPECT_NEAR(av.getVal(), 25, 1); // Expected value at 0.5 seconds

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    av.update();
    EXPECT_NEAR(av.getVal(), 50, 1); // Expected value at 1 second
}
    
TEST(AnimValTest, UpdateTriangularFunction) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

    AnimVal<int> av;
    av.setAnimFunc(AnimFunc::TRI_ONE_P);
    av.start(0, 100, 1.0, false);

    // Simulate passing time
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    av.update();
    EXPECT_NEAR(av.getVal(), 50, 1); // Expected value at 0.25 seconds

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    av.update();
    EXPECT_NEAR(av.getVal(), 100, 1); // Expected value at 0.5 second

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    av.update();
    EXPECT_NEAR(av.getVal(), 50, 1); // Expected value at 0.75 seconds
}

TEST(AnimValTest, EndFunction) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    bool endCalled = false;
    auto endFunc = [&]() { endCalled = true; };
    AnimVal<int> av;
    av.setEndFunc(endFunc);
    av.start(0, 100, 0.5, false);

    // Simulate passing time
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    av.update();
    EXPECT_TRUE(endCalled);
}

TEST(AnimValTest, ResetAnimation) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    AnimVal<int> av;
    av.start(0, 100, 1.0, false);

    // Simulate passing time
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    av.update();
    EXPECT_NEAR(av.getVal(), 25, 1); // Expected value at 0.5 seconds

    av.reset();
    EXPECT_NEAR(av.getVal(), 0, 1); // Reset to start value
}

TEST(AnimValTest, ZeroDuration) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    AnimVal<int> av;
    av.start(0, 100, 0.0, false);
    av.update();
    EXPECT_TRUE(av.stopped());
}

}