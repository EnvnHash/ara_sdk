#pragma once

#include <Utils/Texture.h>

#include "Lights/Light.h"

namespace ara {

class LIProjector : public Light {
public:
    LIProjector(sceneData* sd = nullptr);

    void         setup(bool force = false);
    virtual void draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo = 0);

    std::string* getTexturePath() { return &m_texture_path; }
    void         setTexturePath(std::string& _path) { m_texture_path = _path; }
    void         setReloadTexture(bool _val) { m_reloadTexture = _val; }
    void         setConnDispName(std::string& name) { m_dispname = name; }
    std::string* getConnDispName() { return &m_dispname; }
    void setTexId(int id) { s_colTex = id; }
    int  getTexId() { return s_colTex; }

    float m_aspect;
    float m_throwRatio;
    float m_lensShift[2];

protected:
    bool      m_reloadTexture;
    glm::mat4 m_scale_bias_matrix;

    float m_linearDepthScalar;

    std::string m_texture_path;
    std::string m_dispname;

    // FFMpegDecode*
    // m_ffmpegDecode;
};

}  // namespace ara