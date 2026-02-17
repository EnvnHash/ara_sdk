//
// Created by sven on 17-06-25.
//
#ifndef __ANDROID__

#include <UtilityUnitTestCommon.h>
#include <PoissonGenerator.h>
#include <ImageIO/FreeImageHandler.h>

using namespace std;
using namespace glm;

namespace hfc::UnitTests::PoissonTest {

static void measureLimitsAndMedium2(const std::vector<PoissonGenerator::Point>& points) {
    std::array<vec2, 2> limits {vec2{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
                               vec2{std::numeric_limits<float>::min(), std::numeric_limits<float>::min()}};

    vec2 medPos = {};
    size_t validPosCntr = 0;

    for (auto &it : points) {
        if (it.x < limits[0].x) {
            limits[0].x = it.x;
        }
        if (it.y < limits[0].y) {
            limits[0].y = it.y;
        }

        if (it.x > limits[1].x) {
            limits[1].x = it.x;
        }
        if (it.y > limits[1].y) {
            limits[1].y = it.y;
        }

        medPos += vec2{ it.x, it.y };
        validPosCntr++;
    }

    EXPECT_LT(limits[0].x, 0.0012f);
    EXPECT_LT(limits[0].y, 0.0012f);
    EXPECT_GT(limits[1].x, 0.99f);
    EXPECT_GT(limits[1].y, 0.99f);

    medPos /= vec2{ validPosCntr, validPosCntr };
    EXPECT_NEAR(medPos.x, 0.5f, 0.01f);
    EXPECT_NEAR(medPos.y, 0.5f, 0.01f);
}

static void checkRadius(std::vector<PoissonGenerator::Point>& randPoints, float minDist, float aspect) {
    bool foundLessThanRadius = false;
    for (auto &it : randPoints) {
        if (ranges::any_of(randPoints, [it, minDist, aspect](auto& item){
                auto dist = glm::distance(vec2(item.x, item.y / aspect), vec2(it.x, it.y / aspect));
                return dist != 0.f && dist < minDist;
            })) {
            foundLessThanRadius = true;
            break;
        }
    }

    EXPECT_FALSE(foundLessThanRadius);
}

TEST(PoissonTest, DistribTest) {
    int numPoints = 100000;
    float minDist = 0.01f;
    auto randPoints = PoissonGenerator::generatePoissonPoints(numPoints, false, 30, minDist);
    measureLimitsAndMedium2(randPoints);
}

TEST(PoissonTest, CheckRad) {
    int numPoints = 10000;
    for (auto minDist : { 0.005f, 0.01f, 0.1f, 0.2f }) {
        auto randPoints = PoissonGenerator::generatePoissonPoints(numPoints, false, 30, minDist);
        checkRadius(randPoints, minDist, 1.f);
    }
}

TEST(PoissonTest, CheckRectangle) {
    int numPoints = 100000;
    float minDist = 0.01f;
    float aspect = 2.24f;
    auto randPoints = PoissonGenerator::generatePoissonPoints(numPoints, false, 30, minDist, aspect);
    measureLimitsAndMedium2(randPoints);
    checkRadius(randPoints, minDist, aspect);
}

}
#endif