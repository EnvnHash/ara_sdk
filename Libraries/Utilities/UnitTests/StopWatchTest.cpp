//
// Created by sven on 02-04-25.
//

#include <utility_unit_test_common.h>
#include "StopWatch.h"
#ifdef _WIN32
#include "Windows.h"
#endif

namespace ara {

class StopWatchTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    StopWatch sw;
};

TEST_F(StopWatchTest, SetStart) {
    EXPECT_FALSE(sw.startSet);
    sw.setStart();
    EXPECT_TRUE(sw.startSet);
}

TEST_F(StopWatchTest, SetEnd) {
    sw.setStart();
    sw.setEnd();
    EXPECT_FALSE(sw.startSet);
    EXPECT_GT(sw.dt, 0.0);
}

TEST_F(StopWatchTest, CalculateElapsedTime) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    sw.setStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sw.setEnd();
    EXPECT_NEAR(sw.getDt(), 500.0, 10); // Expected value within a tolerance of 10 ms
}

TEST_F(StopWatchTest, PrintTime) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    sw.setStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sw.setEnd();
    EXPECT_NO_THROW(sw.print("Elapsed"));
}

TEST_F(StopWatchTest, EdgeCaseNoStartSet) {
    EXPECT_FALSE(sw.startSet);
    sw.setEnd();
    EXPECT_EQ(sw.dt, 0.0); // No valid duration without a start time
}

}  // namespace ara
