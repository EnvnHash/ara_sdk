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

#if defined(ARA_USE_FREEIMAGE) && !defined(__EMSCRIPTEN__)

#ifndef _WIN32
#define _stdcall
#endif

#include <glb_common/glb_common.h>
#include <FreeImage.h>

namespace ara {

class FreeImg_MemHandler {
public:
    FreeImg_MemHandler(void *ptr, size_t size);

    void                  *memPtr  = nullptr;
    size_t                 memSize = 0;
    size_t                 memPos  = 0;
    FreeImageIO            fIO{};

    [[nodiscard]] uint8_t *getPos() const {
        return static_cast<uint8_t *>(memPtr) + memPos;
    }

    static void fillFreeImageIO(FreeImageIO &io);
    FreeImageIO *io() { return &fIO; }

    static unsigned _stdcall read(void *buffer, unsigned size, unsigned count, fi_handle handle);
    static unsigned _stdcall write(void *buffer, unsigned size, unsigned count, fi_handle handle);
    static int _stdcall seek(fi_handle handle, long offset, int origin);
    static long _stdcall tell(fi_handle handle);
};

}

#endif
