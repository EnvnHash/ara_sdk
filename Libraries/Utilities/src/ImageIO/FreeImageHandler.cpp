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

    // check the file signature and deduce its format
    // (the second argument is currently not used by FreeImage)
    auto f = FreeImage_GetFileType(path.c_str(), 0);
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
    return Load(vp.data(), vp.size(), fif);
}

void Load(std::vector<uint8_t>& vp, Handle& hnd) {
    if (hnd.multiPage) {
        const auto bitmap = LoadMulti(vp, &hnd.fif);
        InitHandle(hnd, nullptr, bitmap);
    } else {
        const auto bitmap = Load(vp, &hnd.fif);
        InitHandle(hnd, bitmap, nullptr);
    }
}

FIMULTIBITMAP* LoadMulti(std::vector<uint8_t>& vp, FREE_IMAGE_FORMAT* fif) {
    Initialize();
    return LoadMulti(vp.data(), vp.size(), fif);
}

std::tuple<FREE_IMAGE_FORMAT, FIMEMORY*> LoadPrepare(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif) {
    if (size == 0) {
        LOGE << "FreeImage::Load failed, size of memory to load to is zero";
        return {};
    }

    auto mem = FreeImage_OpenMemory(static_cast<BYTE*>(ptr), size);
    if (mem == nullptr) {
        LOGE << "Failed to create memory object";
        return {};
    }

    auto f = FreeImage_GetFileTypeFromMemory(mem, 0);
    if (fif) {
        *fif = f;
    }
    return { fif ? *fif : f, mem };
}

FIBITMAP* Load(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif) {
    auto r = LoadPrepare(ptr, size, fif);
    FIBITMAP* bitmap = nullptr;
    if ((bitmap = FreeImage_LoadFromMemory(std::get<FREE_IMAGE_FORMAT>(r), std::get<FIMEMORY*>(r), 0)) == nullptr) {
        LOGE << "Failed to load image from memory";
    }
    FreeImage_CloseMemory(std::get<FIMEMORY*>(r));
    return bitmap;
}

FIMULTIBITMAP* LoadMulti(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif) {
    auto r = LoadPrepare(ptr, size, fif);

    FIMULTIBITMAP* bitmap = nullptr;
    if ((bitmap = FreeImage_LoadMultiBitmapFromMemory(std::get<FREE_IMAGE_FORMAT>(r), std::get<FIMEMORY*>(r), 0)) == nullptr) {
        LOGE << "Failed to load image from memory";
    }

    //FreeImage_CloseMemory(std::get<FIMEMORY*>(r));
    return bitmap;
}

void InitHandle(Handle& hnd, FIBITMAP* bitmap, FIMULTIBITMAP* multiBitmap) {
    hnd.bitmap = bitmap;
    hnd.multiBitmap = multiBitmap;
    hnd.multiPage = multiBitmap != nullptr;
    if (hnd.multiPage) {
        hnd.pixels = static_cast<uint8_t *>(hnd.multiBitmap->data);
    } else {
        hnd.pixels = static_cast<uint8_t *>(hnd.bitmap->data);
    }
    const auto sz = GetSizeFromBitmap(hnd.bitmap);
    hnd.width = static_cast<int32_t>(sz[0]);
    hnd.height = static_cast<int32_t>(sz[1]);

    hnd.numChannels = FreeImage::GetNumChannels(hnd.bitmap);
    hnd.bpp = static_cast<int32_t>(FreeImage_GetBPP(hnd.bitmap));
}

std::array<uint32_t, 2> GetSize(const std::string& path) {
    const auto bitmap = FreeImage::Load(path, nullptr);
    const auto sz = GetSizeFromBitmap(bitmap);
    FreeImage_Unload(bitmap);
    return sz;
}

std::array<uint32_t, 2> GetSize(std::vector<uint8_t>& vp) {
    const auto bitmap = FreeImage::Load(vp);
    const auto sz = GetSizeFromBitmap(bitmap);
    FreeImage_Unload(bitmap);
    return sz;
}

std::array<uint32_t, 2> GetSizeFromBitmap(FIBITMAP* bitmap) {
    return { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap) };
}

uint8_t GetNumChannels(FIBITMAP* bitmap) {
    const auto bpp = FreeImage_GetBPP(bitmap);
    if (const auto imageType = FreeImage_GetImageType(bitmap); imageType == FIT_BITMAP) {
        if (bpp == 8) {
            return 1; // Grayscale
        } else if (bpp == 24) {
            return 3; // RGB
        } else if (bpp == 32) {
            return 4; // RGBA
        }
    } else if (imageType == FIT_FLOAT || imageType == FIT_DOUBLE || imageType == FIT_RGBF || imageType == FIT_RGBAF) {
        return static_cast<uint8_t>(bpp / (8 * sizeof(float)));
    }
    return 0;
}

void Save(const std::string& filename, FREE_IMAGE_FORMAT filetype, int w, int h, int nrChan, uint8_t *buf) {
    if (const auto saveBufCont = FreeImage_Allocate(w, h, nrChan * 8)) {
        std::copy_n(buf, (w * h * nrChan), FreeImage_GetBits(saveBufCont));
        if (!FreeImage_Save(filetype, saveBufCont, filename.c_str())) {
            LOGE << "FreeImage::saveTexToFile2D Error: FreeImage_Save failed";
        }
    }
}

FIBITMAP* ConvertTo32Bits(FIBITMAP* bitmap) {
    const auto dib32 = FreeImage_ConvertTo32Bits(bitmap);
    if (!dib32) {
        LOGE << "Failed to convert image to 32-bit";
        Unload(bitmap);
    }
    return dib32;
}

}

#endif