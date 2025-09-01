//
// Created by sven on 25-08-25.
//

#pragma once

#include "Texture.h"

namespace ara {

class ImageSequence {
public:
    ImageSequence() = default;
    void load(const std::filesystem::path& p, GLBase* glbase);
    void loadFromAsset(const std::filesystem::path& p, GLBase* glbase);
    void uploadToTexture(int32_t numFrames, FIMULTIBITMAP* multiBitmap, GLBase* glbase);
    void start();
    int32_t update();
    void setLoopStart(int32_t val) { m_startFrame = val; }
    void setLoopPoint(int32_t val) { m_loopPoint = val; }
    void setLooping(bool val) { m_looping = val; }
    void setFps(int32_t val) { m_fps = val; }
    void setEndCb(const std::function<void(ImageSequence*, int32_t)>& f) { m_endCb = f; }

    int32_t getTexId(int32_t frameNr);
    int32_t getWidth();
    int32_t getHeight();
    int32_t getBitCount();
    int32_t getCurrFrame() { return m_currFrame; }
    int32_t getNumFrames() { return m_textures.size(); }

private:
    std::vector<Texture>                    m_textures;
    bool                                    m_running = false;
    bool                                    m_looping = true;
    std::chrono::system_clock::time_point   m_startTime;
    double                                  m_fps = 15;
    int32_t                                 m_currFrame = 0;
    int32_t                                 m_startFrame = 0;
    int32_t                                 m_loopPoint = 0;
    int32_t                                 m_loopsPassed = 0;
    std::function<void(ImageSequence*, int32_t)> m_endCb;
};

}
