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
