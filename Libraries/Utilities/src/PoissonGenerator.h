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

/*
 parallel version - WIP. does not yet fully work

 #pragma once

#include <vector>
#include <cmath>
#include <random>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <glm/glm.hpp>
#include "threadpool/BS_thread_pool.hpp"

namespace PoissonGenerator {

struct FastRNG {
    uint32_t state;

    FastRNG() {
        state = std::random_device{}();
    }

    inline float next() {
        state = 1664525u * state + 1013904223u;
        return (state >> 8) * (1.0f / 16777216.0f);
    }
};

static thread_local FastRNG rng;

struct Point {
    Point() = default;
    Point(const float X, const float Y) : x(X), y(Y), valid(true) {}
    float x = 0.f;
    float y = 0.f;
    bool valid = false;


    bool inRect() const {
        return x >= 0 && y >= 0 && x <= 1 && y <= 1;
    }

    bool inCircle() const {
        const float dx = x - 0.5f;
        const float dy = y - 0.5f;
        return dx * dx + dy * dy <= 0.25f;
    }
};

struct Grid {
    Grid(const int W, const int H, const float cs, std::vector<Point>* pts)
        : w(W), h(H), cellSize(cs), cells(W * H), points(pts) {
        for (auto& c : cells) c.store(-1);
    }

    void insert(const Point& p, const int idx) {
        const auto gx = static_cast<int>(p.x / cellSize);
        const auto gy = static_cast<int>(p.y / cellSize);
        if (gx >= 0 && gy >= 0 && gx < w && gy < h) {
            cells[index(gx, gy)].store(idx, std::memory_order_relaxed);
        }
    }

    int index(const int x, const int y) const {
        return y * w + x;
    }

    bool near(const Point& p, const float minDist2) const {
        const auto g = glm::ivec2{ static_cast<int>(p.x / cellSize),
                                static_cast<int>(p.y / cellSize) };

        constexpr int D = 2;

        for (int y = g.y - D; y <= g.y + D; ++y) {
            for (int x = g.x - D; x <= g.x + D; ++x) {
                if (x < 0 || y < 0 || x >= w || y >= h) {
                    continue;
                }

                const int idx = cells[index(x, y)].load(std::memory_order_relaxed);
                if (idx < 0) {
                    continue;
                }

                const Point& q = (*points)[idx];
                const float dx = q.x - p.x;
                const float dy = q.y - p.y;

                if (dx * dx + dy * dy < minDist2) {
                    return true;
                }
            }
        }
        return false;
    }

    [[nodiscard]] bool near(const Point& p, glm::vec2 minDist) const {
        if (minDist.x == minDist.y) {
            return near(p, minDist.x);
        } else {
            const auto g = glm::ivec2{ static_cast<int>(p.x / cellSize),
                                    static_cast<int>(p.y / cellSize) };

            constexpr int D = 5;

            for (int x = g.x - D; x <= g.x + D; ++x) {
                for (int y = g.y - D; y <= g.y + D; ++y) {
                    if (x >= 0 && x < w && y >= 0 && y < h) {
                        const int idx = cells[index(x, y)].load(std::memory_order_relaxed);
                        if (idx < 0) {
                            continue;
                        }

                        const Point& P = (*points)[idx];
                        if (P.valid && std::abs(P.x - p.x) < minDist.x && std::abs(P.y - p.y) < minDist.y) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }


    int w, h;
    float cellSize;
    std::vector<std::atomic<int>> cells;
    std::vector<Point>* points;

};

inline Point randomAround(const Point& p, const float minDist) {
    const float r1 = rng.next();
    const float r2 = rng.next();

    const float radius = minDist * (1.f + r1);
    const float angle = 6.283185307f * r2;

    return {
        p.x + radius * std::cos(angle),
        p.y + radius * std::sin(angle)
    };
}

inline Point randomAround(const Point& p, const glm::vec2& minDist) {
    if (minDist.x == minDist.y) {
        return randomAround(p, minDist.x);
    } else {
        const float r1 = rng.next();
        const float r2 = rng.next();

        const glm::vec2 radius = minDist * (r1 + 1.f);
        const float angle = 6.283185307f * r2;

        const float y = std::sin(angle);
        const float ellipticRadius = glm::mix(radius.x, radius.y, std::abs(y));
        return {p.x + ellipticRadius * std::cos(angle),
                p.y + ellipticRadius * y};
    }
}

inline Point popRandom(std::vector<Point>& v) {
    const auto idx = static_cast<int>(rng.next() * v.size());
    const Point p = v[idx];
    v[idx] = v.back();
    v.pop_back();
    return p;
}

static std::vector<Point> generatePoissonPoints(BS::thread_pool<>& pool,
    const uint32_t numPoints,
    const bool isCircle = true,
    const uint32_t k = 30,
    float minDist = -1.f,
    const float aspectRatio = 1.f) {

    if (!numPoints) {
        return {};
    }

    if (minDist < 0.f) {
        minDist = std::sqrt(static_cast<float>(numPoints)) / static_cast<float>(numPoints);
    }

    std::vector<Point> samples;
    std::vector<Point> process;
    samples.reserve(numPoints);

    const float minDist2 = minDist * minDist;

    // create the grid
    const float cellSize = minDist / std::sqrt(2.f);
    const int gridW = static_cast<int>(std::ceil(1.f / cellSize));
    const int gridH = gridW;

    Grid grid(gridW, gridH, cellSize, &samples);

    const glm::vec2 minDistWithAspect { minDist, minDist * aspectRatio };
    const glm::vec2 minDistWithAspect2 { minDist2, minDist2 * aspectRatio };

    // first point
    Point first;
    do {
        first = { rng.next(), rng.next() };
    } while (!(isCircle ? first.inCircle() : first.inRect()));

    samples.emplace_back(first);
    process.emplace_back(first);
    grid.insert(first, 0);

    while (!process.empty() && samples.size() < numPoints) {
        Point p = popRandom(process);

        std::vector<Point> accepted;
        std::mutex accMutex;
        std::list<std::future<void>> futures;

        for (uint32_t i = 0; i < k; ++i) {
            futures.emplace_back(pool.submit_task([&, p]() {
                Point np = randomAround(p, minDistWithAspect);

                if (!(isCircle ? np.inCircle() : np.inRect())) {
                    return;
                }

                if (grid.near(np, minDistWithAspect2)) {
                    return;
                }

                std::lock_guard lock(accMutex);
                accepted.emplace_back(np);
            }));
        }

        for (auto &it : futures) {
            if (it.valid()) {
                it.wait();
            }
        };

        for (auto& pt : accepted) {
            const auto idx = static_cast<int>(samples.size());
            samples.emplace_back(pt);
            process.emplace_back(pt);
            grid.insert(pt, idx);
        }
    }

    return samples;
}

static std::vector<Point> generateVogelPoints(BS::thread_pool<>& pool, const uint32_t n) {
    std::vector<Point> pts(n);

    constexpr float golden = 2.4f;
    std::list<std::future<void>> futures;

    for (uint32_t i = 0; i < n; ++i) {
        futures.emplace_back(pool.submit_task([&, i]() {
            const float r = std::sqrt(i + 0.5f) / std::sqrt(static_cast<float>(n));
            const float theta = i * golden;

            pts[i] = {
                0.5f + r * std::cos(theta),
                0.5f + r * std::sin(theta)
            };
        }));
    }

    for (auto &it : futures) {
        if (it.valid()) {
            it.wait();
        }
    };

    return pts;
}

inline float radicalInverse(uint32_t bits) {
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    return bits * 2.3283064365386963e-10f;
}

static std::vector<Point> generateHammersleyPoints(BS::thread_pool<>& pool, const uint32_t n) {
    std::vector<Point> pts(n);
    std::list<std::future<void>> futures;

    for (uint32_t i = 0; i < n; ++i) {
        futures.emplace_back(pool.submit_task([&, i]() {
            pts[i] = { static_cast<float>(i) / n, radicalInverse(i) };
        }));
    }

    for (auto &it : futures) {
        if (it.valid()) {
            it.wait();
        }
    };

    return pts;
}

static std::vector<uint8_t> generateBitmap(BS::thread_pool<>& pool, const int width, const int height, const std::vector<Point>& pts) {
    std::vector<uint8_t> img(width * height * 3, 0);
    std::list<std::future<void>> futures;

    for (size_t i = 0; i < pts.size(); ++i) {
        futures.emplace_back(pool.submit_task([&, i]() {
            const auto x = static_cast<int>(pts[i].x * width);
            const auto y = static_cast<int>(pts[i].y * height);

            if (x < 0 || y < 0 || x >= width || y >= height) {
                return;
            }

            const int base = 3 * (x + y * width);
            img[base] = img[base + 1] = img[base + 2] = 255;
        }));
    }

    for (auto &it : futures) {
        if (it.valid()) {
            it.wait();
        }
    };

    return img;
}

} // namespace PoissonGenerator

 */