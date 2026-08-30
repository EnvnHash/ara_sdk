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
    if (f != FIF_UNKNOWN && FreeImage_FIFSupportsReading(f)) {
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

// expansion is towards right top corner
void ExpandImage(const std::filesystem::path& path, const int32_t newWidth, const int32_t newHeight) {
    if (!path.empty() && filesystem::exists(path)) {
        auto fif = FreeImage_GetFileType(path.c_str(), 0);

        if (auto* oldBitmap = Load(path.string(), &fif)) {
            if (auto* newBitmap = FreeImage_AllocateT(FIT_BITMAP, newWidth, newHeight, 32)) {
                // FreeImage coordinates are bottom-up, but our grid is top-down (PaintImage sectionPos)
                // Actually, sectionPos in PaintImage is used for UVs.
                // We need to copy old content.
                const auto oldHeight = FreeImage_GetHeight(oldBitmap);

                // Copy oldBitmap to newBitmap.
                // Since both are 32-bit and we want to keep positions consistent:
                // In a top-left origin system, old content is at (0,0) to (oldWidth, oldHeight).
                // In FreeImage (bottom-left), old content is at (0, newHeight - oldHeight) to (oldWidth, newHeight).
                FreeImage_Paste(newBitmap, oldBitmap, 0, newHeight - oldHeight, 255);

                FreeImage_GetHeight(oldBitmap);

                if (FreeImage_Save(fif, newBitmap, path.c_str(), 0)) {
                    LOG << "Expanded segmented image to " << newWidth << "x" << newHeight;
                } else {
                    LOGE << "Failed to save expanded segmented image";
                }
                FreeImage_Unload(newBitmap);
            }
            FreeImage_Unload(oldBitmap);
        }
    }
}

std::tuple<FREE_IMAGE_FORMAT, FIMEMORY*> LoadPrepare(void* ptr, const size_t size, FREE_IMAGE_FORMAT* fif) {
    if (size == 0) {
        LOGE << "FreeImage::Load failed, size of memory to load to is zero";
        return {};
    }

    auto mem = FreeImage_OpenMemory(static_cast<BYTE*>(ptr), static_cast<DWORD>(size));
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

FIBITMAP* Load(void* ptr, const size_t size, FREE_IMAGE_FORMAT* fif) {
    const auto r = LoadPrepare(ptr, size, fif);
    FIBITMAP* bitmap = nullptr;
    if ((bitmap = FreeImage_LoadFromMemory(std::get<FREE_IMAGE_FORMAT>(r), std::get<FIMEMORY*>(r), 0)) == nullptr) {
        LOGE << "Failed to load image from memory";
    }
    FreeImage_CloseMemory(std::get<FIMEMORY*>(r));
    return bitmap;
}

FIMULTIBITMAP* LoadMulti(void* ptr, const size_t size, FREE_IMAGE_FORMAT* fif) {
    const auto r = LoadPrepare(ptr, size, fif);

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

void Save(const std::string& filename, const FREE_IMAGE_FORMAT filetype, const int w, const int h, const int nrChan, uint8_t *buf) {
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

void vFlip(std::vector<uint8_t>& input, const uint32_t width, const uint32_t height, const uint32_t bpp) {
    Initialize();

    const auto bitmap = FreeImage_ConvertFromRawBits(input.data(), width, height, width, bpp, 0xFF, 0xFF, 0xFF);
    if (!bitmap) {
        throw std::runtime_error("Failed to create FIBITMAP from input data.");
    }

    FreeImage_FlipVertical(bitmap);

    const uint8_t* flipped_data = FreeImage_GetBits(bitmap);
    std::copy_n(flipped_data, input.size(), input.begin());

    FreeImage_Unload(bitmap);
}

void CropAndScale(const std::string& path, const glm::vec2& cropStart, const glm::vec2& cropEnd,
                      const glm::vec2& destSize, const std::string& destPath) {
        FREE_IMAGE_FORMAT fmt{};
        auto* src = Load(path, &fmt);
        if (!src) {
            LOGE << "CropAndScale could not load: " << path;
            return;
        }

        const glm::vec2 srcSize{
            static_cast<float>(FreeImage_GetWidth(src)),
            static_cast<float>(FreeImage_GetHeight(src))
        };

        auto clampCoord = [](const float v, const float maxVal) {
            return std::clamp(static_cast<int32_t>(std::round(v)), 0, static_cast<int32_t>(maxVal));
        };

        const int32_t left = clampCoord(cropStart.x, srcSize.x);
        const int32_t top = clampCoord(cropStart.y, srcSize.y);
        const int32_t right = clampCoord(cropEnd.x, srcSize.x);
        const int32_t bottom = clampCoord(cropEnd.y, srcSize.y);

        if (right <= left || bottom <= top) {
            LOGE << "SceneBackgroundEditor::cropImage invalid crop rect";
            Unload(src);
            return;
        }

        auto* cropped = FreeImage_Copy(src, left, top, right, bottom);
        if (!cropped) {
            LOGE << "SceneBackgroundEditor::cropImage crop failed";
            Unload(src);
            return;
        }

        auto* scaled = FreeImage_Rescale(
            cropped,
            static_cast<int32_t>(std::round(destSize.x)),
            static_cast<int32_t>(std::round(destSize.y)),
            FILTER_LANCZOS3
        );

        if (scaled) {
            FIBITMAP* rgba_scaled = nullptr;
            const bool is_rgba = FreeImage_GetBPP(scaled) == 32;

            if (!is_rgba) {
                rgba_scaled = FreeImage_ConvertTo32Bits(scaled);
                FreeImage_SetTransparent(rgba_scaled, true);
                const auto b = FreeImage_GetBits(rgba_scaled);
                b[FI_RGBA_ALPHA] = 254; // dirty hack to avoid libpng stripping away the alpha channel

            } else {
                rgba_scaled = scaled;
            }

            if (rgba_scaled) {
                if (!FreeImage_Save(FIF_PNG, rgba_scaled, destPath.c_str(), 0)) {
                    LOGE << "SceneBackgroundEditor::cropImage failed to save: " << destPath;
                }
                if (!is_rgba) {
                    Unload(rgba_scaled);
                }
            }
            Unload(scaled);
        } else {
            LOGE << "SceneBackgroundEditor::cropImage crop/rescale failed";
        }

        Unload(cropped);
        Unload(src);
    }
}

#endif