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

#include <ImageIO/FreeImageHandler.h>

using namespace std;

namespace ara::FreeImage {

void Initialize() {
// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise();
#endif
}

FIBITMAP* Load(const std::string& path, FREE_IMAGE_FORMAT* fif) {
    Initialize();

    FREE_IMAGE_FORMAT f = FIF_UNKNOWN;

    // check the file signature and deduce its format
    // (the second argument is currently not used by FreeImage)
    f = FreeImage_GetFileType(path.c_str(), 0);

    if (f == FIF_UNKNOWN) {
        // no signature ? try to guess the file format from the file extension
        f = FreeImage_GetFIFFromFilename(path.c_str());
    }

    if (fif) {
        *fif = f;
    }

    // check that the plugin has reading capabilities ...
    if ((f != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(f)) {
        // ok, let's load the file
        return FreeImage_Load(f, path.c_str(), 0);
    } else {
        LOGE << "Texture::Error unknown format";
        return nullptr;
    }
}

FIBITMAP* Load(std::vector<uint8_t>& vp, FREE_IMAGE_FORMAT* fif) {
    Initialize();
    return FreeImage::Load(vp.data(), vp.size(), fif);
}

void Load(std::vector<uint8_t>& vp, Handle& hnd) {
    auto bitmap = FreeImage::Load(vp, &hnd.fif);
    InitHandle(hnd, bitmap);
}

FIBITMAP* Load(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif) {
    if (size == 0) {
        LOGE << "FreeImage::Load failed, size of memory to load to is zero";
        return {};
    }
    FreeImage::MemHandler mh(ptr, size);
    FREE_IMAGE_FORMAT f = FreeImage_GetFileTypeFromHandle(mh.io(), (fi_handle)&mh, 0);
    if(fif) {
        *fif = f;
    }
    FIBITMAP* bitmap = nullptr;
    if (mh.memPos < mh.memSize && ((bitmap = FreeImage_LoadFromHandle(f, mh.io(), (fi_handle)&mh, 0)) == nullptr)) {
        LOGE << "FreeImage::Load failed";
        return {};
    }
    return bitmap;
}

void InitHandle(Handle& hnd, FIBITMAP* bitmap) {
    hnd.bitmap = bitmap;
    auto sz = GetSizeFromBitmap(hnd.bitmap);
    hnd.width = static_cast<int32_t>(sz[0]);
    hnd.height = static_cast<int32_t>(sz[1]);
    hnd.pixels = reinterpret_cast<uint8_t *>(hnd.bitmap->data);
    hnd.numChannels = FreeImage::GetNumChannels(hnd.bitmap);
    hnd.bpp = static_cast<int32_t>(FreeImage_GetBPP(hnd.bitmap));
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

uint8_t GetNumChannels(FIBITMAP* bitmap) {
    auto bpp = FreeImage_GetBPP(bitmap);
    auto imageType = FreeImage_GetImageType(bitmap);
    if (imageType == FIT_BITMAP) {
        if (bpp == 8) {
            return 1; // Grayscale
        } else if (bpp == 24) {
            return 3; // RGB
        } else if (bpp == 32) {
            return 4; // RGBA
        }
    } else if (imageType == FIT_FLOAT || imageType == FIT_DOUBLE) {
        return bpp / (8 * sizeof(float)); // For float images
    } else if (imageType == FIT_RGBF || imageType == FIT_RGBAF) {
        return bpp / (8 * sizeof(float)); // For float color images
    }
    return 0;
}

void Save(const std::string& filename, FREE_IMAGE_FORMAT filetype, int w, int h, int nrChan, uint8_t *buf) {
    auto saveBufCont = FreeImage_Allocate(w, h, nrChan * 8);

    if (saveBufCont) {
        std::copy_n(buf, (w * h * nrChan), FreeImage_GetBits(saveBufCont));
        if (!FreeImage_Save(filetype, saveBufCont, filename.c_str())) {
            LOGE << "FreeImage::saveTexToFile2D Error: FreeImage_Save failed";
        }
    }
}

FIBITMAP* ConvertTo32Bits(FIBITMAP* bitmap) {
    auto dib32 = FreeImage_ConvertTo32Bits(bitmap);
    if (!dib32) {
        LOGE << "Failed to convert image to 32-bit";
        FreeImage::Unload(bitmap);
    }
    return dib32;
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