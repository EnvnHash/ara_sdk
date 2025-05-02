#pragma once

#include "Div.h"

namespace ara {

class AssetImageBase;

class Spinner : public Div {
public:
    Spinner();
    Spinner(const std::filesystem::path& filepath, int x, int y, unsigned width, unsigned height);
    ~Spinner() override = default;

    void init() override;
    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;
    void setImgBase(AssetImageBase* imgBase);
    bool draw(uint32_t* objId) override;
    bool drawIndirect(uint32_t* objId) override;
    bool drawFunc(const uint32_t* objId);

    Texture* setImage(const std::filesystem::path& filepath, int mipmapLevels = 8);
    bool     setSpeed(float new_speed) {
        m_speed = new_speed;
        return true;
    }

private:
    Shaders*        m_shader  = nullptr;
    Texture*        m_tex     = nullptr;
    AssetImageBase* m_imgBase = nullptr;

    float    m_refTime   = 0.f;
    float    m_speed     = .1f;
    uint32_t m_tempObjId = 0;
    uint32_t m_dfObjId   = 0;

    std::chrono::time_point<std::chrono::system_clock> m_initTime;
};

}  // namespace ara
