//
// Created by hahne on 22.04.2025.
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

#pragma once

#include <gtest/gtest.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <GLBase.h>
#include "threadpool/BS_thread_pool.hpp"

static inline BS::thread_pool g_thread_pool;

namespace ara::GLBaseUnitTest{

static void compareBitmaps(const GLubyte* data1, const GLubyte* data2, uint32_t width, const uint32_t height, uint8_t eps, bool bgra_to_rgba) {
    std::list<std::future<void>> futures;
    const auto tc = g_thread_pool.get_thread_count();
    auto ySlices = height / static_cast<uint32_t>(tc);

    for (uint32_t i=0; i<tc; i++) {
        futures.emplace_back(g_thread_pool.submit_task([&data1, &data2, width, ySlices, i, eps, bgra_to_rgba] {
            auto dataPtr1 = &data1[0];
            auto dataPtr2 = &data2[0];

            const uint32_t offs = ySlices * width * 4 * i;
            dataPtr1 += offs;
            dataPtr2 += offs;

            for (uint32_t y = 0; y < ySlices; y++) {
                for (uint32_t x = 0; x < width; x++, dataPtr2 += 4, dataPtr1 += 4) {
                    // freeimage reads in BGRA format, but textures are supposed to be in RGBA
                    if (bgra_to_rgba) {
                        EXPECT_NEAR(*(dataPtr2+2),  *dataPtr1, eps);
                        EXPECT_NEAR(*(dataPtr2+1),  *(dataPtr1 +1), eps);
                        EXPECT_NEAR(*dataPtr2,      *(dataPtr1 +2), eps);
                    } else {
                        EXPECT_NEAR(*dataPtr2,      *dataPtr1, eps);
                        EXPECT_NEAR(*(dataPtr2+1),  *(dataPtr1 +1), eps);
                        EXPECT_NEAR(*(dataPtr2+2),  *(dataPtr1 +2), eps);
                    }
                    EXPECT_NEAR(*(dataPtr2+3),  *(dataPtr1 +3), eps);
                }
            }
        }));
    }

    for (auto &it : futures) {
        if (it.valid()) {
            it.wait();
        }
    }
}

static void compareBitmapToFile(const std::vector<GLubyte>& data, const std::filesystem::path& p, uint32_t width, const uint32_t height, const uint8_t eps) {
    if (!std::filesystem::exists(p)) {
        LOGE << "compareBitmaps error, couldn't load " << p.string();
        return;
    }
    const auto pBitmap = FreeImage::Load(p.string(), nullptr);
    ASSERT_TRUE(pBitmap);
    const auto data2 = FreeImage_GetBits(pBitmap);

    compareBitmaps(data.data(), data2, width, height, eps, true);

    FreeImage::Unload(pBitmap);
}

static void compareTwoFiles(const std::filesystem::path& path1, const std::filesystem::path& path2, uint8_t eps = 0) {

    const std::array bitmap = { FreeImage::Load(path1.string(), nullptr), FreeImage::Load(path1.string(), nullptr) };
    ASSERT_TRUE(bitmap[0]);
    ASSERT_TRUE(bitmap[1]);

    const auto size = FreeImage::GetSizeFromBitmap(bitmap[0]);

    const std::array data = { FreeImage::GetBits(bitmap[0]),  FreeImage::GetBits(bitmap[1]) };

    compareBitmaps(data[0], data[1], size[0], size[1], eps, false);

    FreeImage::Unload(bitmap[0]);
    FreeImage::Unload(bitmap[1]);
}

static std::vector<GLubyte> readBack(const glm::ivec2& size) {
    std::vector<GLubyte> data(size.x * size.y * 4);    // make some space to download
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data.data());    // synchronous, blocking command, no swap() needed

    EXPECT_EQ(postGLError(), GL_NO_ERROR);

    return data;
}

static void createThreads(const int nrThreads, std::vector<GLFWWindow>& windows, GLBase* glBase=nullptr) {
    // create windows, must be not threaded
    for (int i = 0; i < nrThreads; i++) {
        ASSERT_TRUE(windows[i].init(
            glWinPar{
                .doInit = false,            // don't init glfw, this needs to be done on the main thread only once
                .shift = { 300 * i, 100 },  //  offset relative to OS screen canvas
                .size = { 200, 200 },       // set the window's size
                .scaleToMonitor = false,    // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling
                .shareCont = glBase ? static_cast<void*>(glBase->getGlfwHnd()) : nullptr // share the GLBase context
            })
        );

        if (!glBase) {
            ASSERT_EQ(true, initGLEW());
        }
    }
}

#ifdef _WIN32
static bool checkMemLeak(_CrtMemState& sNew, _CrtMemState& sOld, _CrtMemState& sDiff) {
    _CrtMemCheckpoint(&sNew); //take a snapchot
    if (_CrtMemDifference(&sDiff, &sOld, &sNew)) {
        LOG << "-----------_CrtMemDumpStatistics ---------";
        _CrtMemDumpStatistics(&sDiff);
        OutputDebugString(LPCWSTR("-----------_CrtMemDumpAllObjectsSince ---------"));
        _CrtMemDumpAllObjectsSince(&sOld);
        OutputDebugString(LPCWSTR("-----------_CrtDumpMemoryLeaks ---------"));
        _CrtDumpMemoryLeaks();
        return true;
    } else {
        return false;
    }
}
#endif

}
