//
// Created by sven on 25-08-25.
//

#include "Utils/ImageSequence.h"
#include "ImageIO/FreeImageHandler.h"
#include "AssetLoader.h"

using namespace std;
using namespace std::chrono;

namespace ara {

void ImageSequence::load(const filesystem::path& p, GLBase* glbase) {
    if (auto mb = reinterpret_cast<FIMULTIBITMAP*>(FreeImage::Load(p.string()))) {
        uploadToTexture(FreeImage_GetPageCount(mb), mb, glbase);
    }
}

void ImageSequence::loadFromAsset(const filesystem::path& p, GLBase* glbase) {
    std::vector<uint8_t> gifData;
    AssetLoader::loadAssetToMem(gifData, p);

    FreeImage::Handle hnd { .multiPage = true };
    FreeImage::Load(gifData, hnd);
    if (hnd.multiBitmap) {
        auto frameCount = FreeImage_GetPageCount((FIMULTIBITMAP*)hnd.multiBitmap);
        LOG << "frameCount " << frameCount;
        uploadToTexture(frameCount, hnd.multiBitmap, glbase);
    }
}

void ImageSequence::uploadToTexture(int32_t numFrames, FIMULTIBITMAP* multiBitmap, GLBase* glbase) {
    m_textures.resize(numFrames);

    for (int32_t i = 0; i < numFrames; ++i) {
        auto frame = FreeImage_LockPage(multiBitmap, i);
        if (!frame) {
            LOGE << "Failed to lock page " << i;
            continue;
        }
        FIBITMAP *rgbaImage = FreeImage_ConvertTo32Bits(frame);
        m_textures[i].setGlbase(glbase);
        m_textures[i].keepBitmap(true);
        m_textures[i].loadFromFib(rgbaImage, GL_TEXTURE_2D, 1, true);
        FreeImage_Unload(rgbaImage);
        FreeImage_UnlockPage(multiBitmap, frame, FALSE);
    }
}

void ImageSequence::start() {
    m_running = true;
    m_currFrame = 0;
    m_loopsPassed = 0;
    m_startFrame = 0;
    m_startTime = system_clock::now();
}

int32_t ImageSequence::update() {
    auto now = system_clock::now();
    auto pos = duration_cast<milliseconds>(now - m_startTime).count() * 0.001 * m_fps;
    m_currFrame = static_cast<int32_t>(pos) + m_startFrame;
    if (m_currFrame >= m_loopPoint && m_looping) {
        m_currFrame = m_startFrame;
        m_startTime = system_clock::now();
        ++m_loopsPassed;
        if (m_endCb) {
            m_endCb(this, m_loopsPassed);
        }
    }

    return std::min(m_currFrame, static_cast<int32_t>(m_textures.size()) -1);
}

int32_t ImageSequence::getTexId(int32_t frameNr) {
    return m_textures.size() >= frameNr ? static_cast<int32_t>(m_textures[frameNr].getId()) : 0;
}

int32_t ImageSequence::getWidth() {
    return m_textures.empty() ? 0 : static_cast<int32_t>(m_textures[0].getWidth());
}

int32_t ImageSequence::getHeight() {
    return m_textures.empty() ? 0 : static_cast<int32_t>(m_textures[0].getHeight());
}

int32_t ImageSequence::getBitCount() {
    return m_textures.empty() ? 0 : static_cast<int32_t>(m_textures[0].getBpp());
}

}