//
// Created by sven on 02-04-25.
//

#include <utility_unit_test_common.h>

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
