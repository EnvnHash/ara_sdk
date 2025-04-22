
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

#ifdef _WIN32
#include <windows.h>

namespace ara::mouse {

static int getAbsMousePos(int &root_x, int &root_y) {
    POINT pos;
    if (GetCursorPos(&pos)) {
        root_x = static_cast<int>(pos.x);
        root_y = static_cast<int>(pos.y);
        return 0;
    }
    return 1;
}
}  // namespace ara::mouse
#endif
