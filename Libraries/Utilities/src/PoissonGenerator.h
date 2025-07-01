/**
* \file PoissonGenerator.h
* \brief
*
* Poisson Disk Points Generator
*
* \version 1.6.1
* \date 16/02/2024
* \author Sergey Kosarevsky, 2014-2024
* \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
*/

/*
  Usage example:

     #define POISSON_PROGRESS_INDICATOR 1
     #include "PoissonGenerator.h"
     ...
     PoissonGenerator::DefaultPRNG PRNG;
     const auto Points = PoissonGenerator::generatePoissonPoints( NumPoints, PRNG );
     ...
     const auto Points = PoissonGenerator::generateVogelPoints( NumPoints );
*/

// Fast Poisson Disk Sampling in Arbitrary Dimensions
// http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf

// Implementation based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/

/* Versions history:
*		1.6.1   Feb 16, 2024    Reformatted using .clang-format
*		1.6     May 29, 2023    Added generateHammersleyPoints() to generate Hammersley points
*		1.5     Mar 26, 2022    Added generateJitteredGridPoints() to generate jittered grid points
*		1.4.1   Dec 12, 2021		Replaced default Mersenne Twister and <random> with fast and lightweight LCG
*		1.4     Dec  5, 2021		Added generateVogelPoints() to generate Vogel disk points
*		1.3     Mar 14, 2021		Bugfixes: number of points in the !isCircle mode, incorrect loop boundaries
*		1.2     Dec 28, 2019		Bugfixes; more consistent progress indicator; new command line options in demo app
*		1.1.6   Dec  7, 2019		Removed duplicate seed initialization; fixed warnings
*		1.1.5   Jun 16, 2019		In-class initializers; default ctors; naming, shorter code
*		1.1.4   Oct 19, 2016		POISSON_PROGRESS_INDICATOR can be defined outside of the header file, disabled by default
*		1.1.3a  Jun  9, 2016		Update constructor for DefaultPRNG
*		1.1.3   Mar 10, 2016		Header-only library, no global mutable state
*		1.1.2   Apr  9, 2015		Output a text file with XY coordinates
*		1.1.1   May 23, 2014		Initialize PRNG seed, fixed uninitialized fields
*		1.1     May  7, 2014		Support of density maps
*		1.0     May  6, 2014
*/

#pragma once

#include <util_common.h>
#include <glm/glm.hpp>

namespace PoissonGenerator {

static const char* Version = "1.6.1 (16/02/2024)";

struct Point {
   Point() = default;
   Point(float X, float Y) : x(X), y(Y), m_valid(true) {}
   float x = 0.0f;
   float y = 0.0f;
   bool  m_valid = false;

   [[nodiscard]] constexpr bool isInRectangle() const { return x >= 0 && y >= 0 && x <= 1 && y <= 1; }
   [[nodiscard]] constexpr bool isInCircle() const {
       const float fx = x - 0.5f;
       const float fy = y - 0.5f;
       return (fx * fx + fy * fy) <= 0.25f;
   }

   Point& operator+(const Point& p) { x += p.x; y += p.y; return *this; }
   Point& operator-(const Point& p) { x -= p.x; y -= p.y; return *this; }
};

inline float getDistance(const Point& P1, const Point& P2) {
   return std::sqrt((P1.x - P2.x) * (P1.x - P2.x) + (P1.y - P2.y) * (P1.y - P2.y));
}

static glm::ivec2 imageToGrid(const Point& P, glm::vec2 cellSize) {
   return { static_cast<int>(P.x / cellSize.x), static_cast<int>(P.y / cellSize.y)};
}

struct Grid {
    Grid(int w, int h, float cellSize)
            : m_width(w), m_height(h), m_cellSize(cellSize), m_grid(h, std::vector<Point>(w)) {}

   void insert(const Point& p) {
       const auto g = imageToGrid(p, m_cellSize);
       if (g.x >= 0 && g.x < m_width && g.y >= 0 && g.y < m_height) {
           m_grid[g.y][g.x] = p;
       }
   }

   [[nodiscard]] bool isInNeighbourhood(const Point& point, glm::vec2 minDist) const {
        if (minDist.x == minDist.y) {
            return isInNeighbourhood(point, minDist.x);
        } else {
            const auto g = imageToGrid(point, m_cellSize);
            constexpr int D = 5;

            for (int x = g.x - D; x <= g.x + D; ++x) {
                for (int y = g.y - D; y <= g.y + D; ++y) {
                    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
                        const Point P = m_grid[y][x];
                        if (P.m_valid && std::abs(P.x - point.x) < minDist.x && std::abs(P.y - point.y) < minDist.y) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
   }

    [[nodiscard]] bool isInNeighbourhood(const Point& point, float minDist) const {
        const auto g = imageToGrid(point, m_cellSize);
        constexpr int D = 5;

        for (int x = g.x - D; x <= g.x + D; ++x) {
            for (int y = g.y - D; y <= g.y + D; ++y) {
                if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
                    const Point P = m_grid[y][x];
                    if (P.m_valid && getDistance(P, point) < minDist) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

   int                             m_width{}, m_height{};
   glm::vec2                       m_cellSize{};
   std::vector<std::vector<Point>> m_grid;
};

static Point popRandom(std::vector<Point>& points) {
   const int idx = static_cast<int>(ara::getRandF(0.f, static_cast<float>(points.size()) - 1));
   const Point p = points[idx];
   points.erase(points.begin() + idx);
   return p;
}

static Point generateRandomPointAround(const Point& p, float minDist) {
    const float R1 = ara::getRandF(0.f, 1.f);
    const float R2 = ara::getRandF(0.f, 1.f);

    const float radius = minDist * (R1 + 1.0f);
    const float angle = 2.f * static_cast<float>(M_PI) * R2;

    return {p.x + radius * std::cos(angle), p.y + radius * std::sin(angle)};
}

static Point generateRandomPointAround(const Point& p, const glm::vec2& minDist) {
   if (minDist.x == minDist.y) {
        return generateRandomPointAround(p, minDist.x);
   } else {
       const float R1 = ara::getRandF(0.f, 1.f);
       const float R2 = ara::getRandF(0.f, 1.f);

       const glm::vec2 radius = minDist * (R1 + 1.f);
       const float angle = 2.f * static_cast<float>(M_PI) * R2;

       const float y = std::sin(angle);
       float ellipticRadius = glm::mix(radius.x, radius.y, std::abs(y));
       return {p.x + ellipticRadius * std::cos(angle),
               p.y + ellipticRadius * y};
   }
}


/**
  Return a vector of generated points

  NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
  Circle  - 'true' to fill a circle, 'false' to fill a rectangle
  MinDist - minimal distance estimator, use negative value for default
**/
static std::vector<Point> generatePoissonPoints(uint32_t numPoints,
                                        bool isCircle = true,
                                        uint32_t newPointsCount = 30,
                                        float minDist = -1.0f,
                                        float aspectRatio = 1.f) {
   // if we want to generate a Poisson square shape, multiply the estimate number of points by PI/4 due to reduced shape area
   if (!isCircle) {
       constexpr double Pi_4 = 0.785398163397448309616; // PI/4
       numPoints = static_cast<int>(Pi_4 * numPoints);
   }

   if (minDist < 0.0f) {
       minDist = std::sqrt(static_cast<float>(numPoints)) / static_cast<float>(numPoints);
   }

   std::vector<Point> samplePoints;
   std::vector<Point> processList;

   if (!numPoints) {
       return samplePoints;
   }

   // create the grid
    const float cellSize = minDist / std::sqrt(2.f);
    const int gridW = static_cast<int>(std::ceil(1.f / cellSize));
    const int gridH = static_cast<int>(std::ceil(1.f / cellSize));

   Grid grid(gridW, gridH, cellSize);

   glm::vec2 minDistWithAspect { minDist, minDist * aspectRatio };

   Point firstPoint;
   do {
       firstPoint = Point(ara::getRandF(0.f, 1.f), ara::getRandF(0.f, 1.f));
   } while (!(isCircle ? firstPoint.isInCircle() : firstPoint.isInRectangle()));

   // update containers
   processList.emplace_back(firstPoint);
   samplePoints.emplace_back(firstPoint);
   grid.insert(firstPoint);

   // generate new points for each point in the queue
   while (!processList.empty() && samplePoints.size() <= numPoints) {
       const Point point = popRandom(processList);
       for (uint32_t i = 0; i < newPointsCount; ++i) {
           const Point newPoint = generateRandomPointAround(point, minDistWithAspect);
           const bool canFitPoint = isCircle ? newPoint.isInCircle() : newPoint.isInRectangle();

           if (canFitPoint && !grid.isInNeighbourhood(newPoint, minDistWithAspect)) {
               processList.emplace_back(newPoint);
               samplePoints.emplace_back(newPoint);
               grid.insert(newPoint);
           }
       }
   }

   return samplePoints;
}

static Point sampleVogelDisk(uint32_t idx, uint32_t numPoints, float phi) {
   constexpr float kGoldenAngle = 2.4f;

   const float r = sqrtf(float(idx) + 0.5f) / sqrtf(float(numPoints));
   const float theta = static_cast<float>(idx) * kGoldenAngle + phi;

   return {r * cosf(theta), r * sinf(theta)};
}

/**
  Return a vector of generated points
**/
static std::vector<Point> generateVogelPoints(uint32_t numPoints, bool isCircle = true, float phi = 0.0f, Point center = Point(0.5f, 0.5f)) {
   std::vector<Point> samplePoints;
   samplePoints.reserve(numPoints);

   const uint32_t numSamples = isCircle ? 4 * numPoints : numPoints;

   for (uint32_t i = 0; i != numPoints; i++) {
       const Point p = sampleVogelDisk(i, numSamples, phi * 3.141592653f / 180.0f) + center;
       samplePoints.push_back(p);
   }

   return samplePoints;
}

/**
  Return a vector of generated points
**/
static std::vector<Point> generateJitteredGridPoints(uint32_t numPoints,
                                             bool isCircle = false,
                                             float jitterRadius = 0.004f,
                                             Point center = Point(0.5f, 0.5f)) {
   std::vector<Point> samplePoints;
   samplePoints.reserve(numPoints);

   const auto gridSize = uint32_t(sqrt(numPoints));

   for (uint32_t x = 0; x != gridSize; x++) {
       for (uint32_t y = 0; y != gridSize; y++) {
           Point p;
           do {
               const Point offs = generateRandomPointAround(Point(0, 0), jitterRadius) - center + Point(0.5f, 0.5f);
               p = Point(static_cast<float>(x) / static_cast<float>(gridSize), static_cast<float>(y) / static_cast<float>(gridSize)) + offs;
               // generate a new point until it is within the boundaries
           } while (!p.isInRectangle());

           if (isCircle && !p.isInCircle()){
               continue;
           }

           samplePoints.push_back(p);
       }
   }

   return samplePoints;
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
static float radicalInverse_VdC(uint32_t bits) {
   bits = (bits << 16u) | (bits >> 16u);
   bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
   bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
   bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
   bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
   return float(float(bits) * 2.3283064365386963e-10); // / 0x100000000
}

static Point hammersley2d(uint32_t i, uint32_t N) {
   return {float(i) / float(N), radicalInverse_VdC(i)};
}

/**
  Return a vector of generated points
**/
static std::vector<Point> generateHammersleyPoints(uint32_t numPoints) {
   std::vector<Point> samplePoints;
   samplePoints.reserve(numPoints);

   for (uint32_t i = 0; i != numPoints; i++) {
       Point p = hammersley2d(i, numPoints);
       samplePoints.push_back(p);
   }
   return samplePoints;
}

static std::vector<uint8_t> generateBitmap(int width, int height, std::vector<Point>& points) {
    // prepare BGR image
    std::vector<uint8_t> Img;
    Img.resize(3 * width * height);

    for (auto i = points.begin(); i != points.end(); i++) {
        int x = int(i->x * width);
        int y = int(i->y * height);
        if (x < 0 || y < 0 || x >= width || y >= height) {
            continue;
        }
        int Base = 3 * (x + y * width);
        Img[Base + 0] = Img[Base + 1] = Img[Base + 2] = 255;
    }
    return Img;
}

} // namespace PoissonGenerator
