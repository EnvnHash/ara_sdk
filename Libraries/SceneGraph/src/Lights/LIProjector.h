#pragma once

#include "Lights/Light.h"

namespace ara {

class LIProjector : public Light {
public:
    explicit LIProjector(sceneData* sd = nullptr);

    void setup(bool force) override;

    std::string* getTexturePath() { return &m_texture_path; }
    std::string* getConnDispName() { return &m_dispname; }
    int          getTexId() const { return s_colTex; }

    void setTexturePath(const std::string& path) { m_texture_path = path; }
    void setReloadTexture(bool val) { m_reloadTexture = val; }
    void setConnDispName(const std::string& name) { m_dispname = name; }
    void setTexId(int id) { s_colTex = id; }

    float       m_aspect = 16.f / 9.f;
    float       m_throwRatio = 0.5f;
    glm::vec2   m_lensShift{};

protected:
    bool        m_reloadTexture = false;
    glm::mat4   m_scale_bias_matrix{};
    float       m_linearDepthScalar = 0;
    std::string m_texture_path;
    std::string m_dispname;
};

}  // namespace ara