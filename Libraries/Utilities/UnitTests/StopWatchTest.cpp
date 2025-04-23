//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <UtilityUnitTestCommon.h>
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
