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

namespace ara {

    class UtilityTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Setup the utility with some initial parameters (if needed)
        }

        void TearDown() override {
            // Clear any state after each test
        }
    };

    TEST_F(UtilityTest, GenerateUUID) {
        std::string uuid1 = generateUUID();
        std::string uuid2 = generateUUID();

        EXPECT_TRUE(!uuid1.empty());
        EXPECT_TRUE(!uuid2.empty());
        EXPECT_NE(uuid1, uuid2);
    }

    TEST_F(UtilityTest, GetRandomFloat) {
        float min = 0.0f;
        float max = 1.0f;
        float randF1 = getRandF(min, max);
        float randF2 = getRandF(min, max);

        EXPECT_GE(randF1, min);
        EXPECT_LE(randF1, max);
        EXPECT_GE(randF2, min);
        EXPECT_LE(randF2, max);
        EXPECT_NE(randF1, randF2); // Random values should be different
    }

    TEST_F(UtilityTest, InterpolVal) {
        float array[] = {0.0f, 1.0f, 2.0f, 3.0f};
        int arraySize = sizeof(array) / sizeof(array[0]);

        EXPECT_NEAR(interpolVal(0.5f, arraySize, array), 0.5f, 0.001);
        EXPECT_NEAR(interpolVal(1.5f, arraySize, array), 1.5f, 0.001);
        EXPECT_NEAR(interpolVal(2.75f, arraySize, array), 2.75f, 0.001);
    }

    TEST_F(UtilityTest, InterpolVal2) {
        float array[] = {0.0f, 1.0f, 2.0f, 3.0f};
        int arraySize = sizeof(array) / sizeof(array[0]);

        EXPECT_NEAR(interpolVal(0.5f, arraySize, array), 0.5f, 0.001);
        EXPECT_NEAR(interpolVal(1.5f, arraySize, array), 1.5f, 0.001);
        EXPECT_NEAR(interpolVal(2.75f, arraySize, array), 2.75f, 0.001);
    }

    TEST_F(UtilityTest, InterpolValVector) {
        std::vector<float> array = {0.0f, 1.0f, 2.0f, 3.0f};
        int arraySize = static_cast<int>(array.size());

        EXPECT_NEAR(interpolVal(0.25f, arraySize, &array), 0.75f, 0.001);
        EXPECT_NEAR(interpolVal(0.5f, arraySize, &array), 1.5f, 0.001);
        EXPECT_NEAR(interpolVal(0.75f, arraySize, &array), 2.25f, 0.001);
    }

}  // namespace ara
