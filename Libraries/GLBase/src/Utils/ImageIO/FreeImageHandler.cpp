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

using namespace std;

namespace ara::FreeImage {

void Initialize() {
// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise();
#endif
}

FIBITMAP* Load(const std::string& path, int flag) {
    Initialize();

    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

    // check the file signature and deduce its format
    // (the second argument is currently not used by FreeImage)
    fif = FreeImage_GetFileType(path.c_str(), flag);

    if (fif == FIF_UNKNOWN) {
        // no signature ? try to guess the file format from the file extension
        fif = FreeImage_GetFIFFromFilename(path.c_str());
    }

    // check that the plugin has reading capabilities ...
    if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
        // ok, let's load the file
        return FreeImage_Load(fif, path.c_str(), flag);
    } else {
        LOGE << "Texture::Error unknown format";
        return nullptr;
    }
}

FIBITMAP* Load(std::vector<uint8_t>& vp) {
    Initialize();
    return  FreeImage::Load(vp.data(), vp.size());
}

FIBITMAP* Load(void* ptr, size_t size) {
    if (size == 0) {
        LOGE << "FreeImage::Load failed, size of memory to load to is zero";
        return {};
    }
    FreeImage::MemHandler mh(ptr, size);
    FREE_IMAGE_FORMAT  fif = FreeImage_GetFileTypeFromHandle(mh.io(), (fi_handle)&mh, 0);
    FIBITMAP* bitmap = nullptr;
    if (mh.memPos < mh.memSize && ((bitmap = FreeImage_LoadFromHandle(fif, mh.io(), (fi_handle)&mh, 0)) == nullptr)) {
        LOGE << "FreeImage::Load failed";
        return {};
    }
    return bitmap;
}

std::array<uint32_t, 2> GetSize(const std::string& path) {
    auto bitmap = FreeImage::Load(path, 0);
    auto sz = GetSizeFromBitmap(bitmap);
    FreeImage_Unload(bitmap);
    return sz;
}

std::array<uint32_t, 2> GetSize(std::vector<uint8_t>& vp) {
    auto bitmap = FreeImage::Load(vp);
    auto sz = GetSizeFromBitmap(bitmap);
    FreeImage_Unload(bitmap);
    return sz;
}

std::array<uint32_t, 2> GetSizeFromBitmap(FIBITMAP* bitmap) {
    return { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap) };
}

MemHandler::MemHandler(void *ptr, size_t size) {
    memPtr  = ptr;
    memSize = size;
    memPos  = 0;
    fillFreeImageIO(fIO);
}

void MemHandler::fillFreeImageIO(FreeImageIO &io) {
    io.read_proc  = MemHandler::read;
    io.write_proc = MemHandler::write;
    io.seek_proc  = MemHandler::seek;
    io.tell_proc  = MemHandler::tell;
}

unsigned MemHandler::read(void *buffer, unsigned size, unsigned count, fi_handle handle) {
    auto h = static_cast<MemHandler *>(handle);
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

unsigned MemHandler::write(void *buffer, unsigned size, unsigned count, fi_handle handle) {
    return size;
}

int MemHandler::seek(fi_handle handle, long offset, int origin) {
    auto h = static_cast<MemHandler *>(handle);

    if (origin == SEEK_SET) {
        h->memPos = 0;
    } else if (origin == SEEK_CUR) {
        h->memPos = offset;
    }

    return 0;
}

long MemHandler::tell(fi_handle handle) {
    auto h = static_cast<MemHandler *>(handle);
    return static_cast<long>(h->memPos);
}

}

#endif