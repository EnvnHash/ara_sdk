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

#ifdef ARA_USE_FREEIMAGE

#include <Utils/ImageIO/FreeImageHandler.h>

namespace ara {

FreeImg_MemHandler::FreeImg_MemHandler(void *ptr, size_t size) {
    memPtr  = ptr;
    memSize = size;
    memPos  = 0;
    fillFreeImageIO(fIO);
}

void FreeImg_MemHandler::fillFreeImageIO(FreeImageIO &io) {
    io.read_proc  = FreeImg_MemHandler::read;
    io.write_proc = FreeImg_MemHandler::write;
    io.seek_proc  = FreeImg_MemHandler::seek;
    io.tell_proc  = FreeImg_MemHandler::tell;
}

unsigned _stdcall FreeImg_MemHandler::read(void *buffer, unsigned size, unsigned count, fi_handle handle) {
    auto h = static_cast<FreeImg_MemHandler *>(handle);
    auto dest = static_cast<uint8_t *>(buffer);
    auto src  = h->getPos();

    for (unsigned c = 0; c < count; c++) {
        std::copy_n(src, size, dest);
        src += size;
        dest += size;
        h->memPos += size;
    }

    return count;
}

unsigned _stdcall FreeImg_MemHandler::write(void *buffer, unsigned size, unsigned count, fi_handle handle) {
    return size;
}

int _stdcall FreeImg_MemHandler::seek(fi_handle handle, long offset, int origin) {
    auto h = static_cast<FreeImg_MemHandler *>(handle);

    if (origin == SEEK_SET) {
        h->memPos = 0;
    } else if (origin == SEEK_CUR) {
        h->memPos = offset;
    }

    return 0;
}

long _stdcall FreeImg_MemHandler::tell(fi_handle handle) {
    auto h = static_cast<FreeImg_MemHandler *>(handle);
    return static_cast<long>(h->memPos);
}

}

#endif