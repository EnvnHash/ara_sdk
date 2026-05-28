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

#include <util_common.h>
#include <FreeImage.h>

namespace ara::FreeImage {

struct Handle {
    FIBITMAP* bitmap = nullptr;
    FIMULTIBITMAP* multiBitmap = nullptr;
    FREE_IMAGE_FORMAT fif{};
    int32_t width = 0;
    int32_t height = 0;
    int32_t bpp = 0;
    int32_t numChannels = 0;
    uint8_t* pixels = nullptr;
    bool multiPage = false;
};

void                    Initialize();
FIBITMAP*               Load(const std::string& path, FREE_IMAGE_FORMAT* fif = nullptr);
FIBITMAP*               Load(std::vector<uint8_t>& vp, FREE_IMAGE_FORMAT* fif = nullptr);
FIBITMAP*               Load(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif = nullptr);
FIMULTIBITMAP*          LoadMulti(std::vector<uint8_t>& vp, FREE_IMAGE_FORMAT* fif = nullptr);
FIMULTIBITMAP*          LoadMulti(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif);
void                    Load(std::vector<uint8_t>& vp, Handle& hnd);

std::tuple<FREE_IMAGE_FORMAT, FIMEMORY*> LoadPrepare(void* ptr, size_t size, FREE_IMAGE_FORMAT* fif);

void                    InitHandle(Handle& hnd, FIBITMAP* bitmap, FIMULTIBITMAP* multiBitmap);
std::array<uint32_t, 2> GetSize(const std::string& path);
std::array<uint32_t, 2> GetSize(std::vector<uint8_t>& vp);
std::array<uint32_t, 2> GetSizeFromBitmap(FIBITMAP* bitmap);
uint8_t                 GetNumChannels(FIBITMAP* bitmap);
FIBITMAP*               ConvertTo32Bits(FIBITMAP* bitmap);
void                    Save(const std::string& filename, FREE_IMAGE_FORMAT filetype, int w, int h, int nrChan, uint8_t *buf);
void                    vFlip(std::vector<uint8_t>& input, uint32_t width, uint32_t height, uint32_t bpp);
void                    CropAndScale(const std::string& path, const glm::vec2& cropStart, const glm::vec2& cropEnd,
                                     const glm::vec2& destSize, const std::string& destPath); /// cropStart is in pix relative to the srcImage size

static uint8_t*         GetBits(FIBITMAP* bitmap) { return FreeImage_GetBits(bitmap); }
static void             Unload(FIBITMAP* bitmap) { FreeImage_Unload(bitmap); }

}

#endif
