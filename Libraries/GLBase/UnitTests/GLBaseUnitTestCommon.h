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

namespace ara::GLBaseUnitTest{

static std::vector<GLubyte> readBack(const glm::ivec2& size) {
    std::vector<GLubyte> data(size.x * size.y * 4);    // make some space to download
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data.data());    // synchronous, blocking command, no swap() needed

    EXPECT_EQ(postGLError(), GL_NO_ERROR);

    return data;
}

static void createThreads(int nrThreads, std::vector<GLFWWindow>& windows, GLBase* glBase=nullptr) {
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
