//
//  GLSLOpticalFlow.cpp
//  tav_core
//
//  Created by Sven Hahne on 13/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "GLSLOpticalFlow.h"
#include <GLBase.h>
#include <Utils/FBO.h>
#include <Utils/PingPongFbo.h>
#include <Shaders/ShaderCollector.h>

using namespace glm;
using namespace std;

namespace ara {

GLSLOpticalFlow::GLSLOpticalFlow(GLBase* glbase, int _m_width, int _height)
    : m_glbase(glbase),
        m_shCol(&glbase->shaderCollector()),
        m_width(_m_width),
        m_height(_height) {
    initShader(m_shCol);
    m_texShader = m_shCol->getStdTex();

    m_texture = make_unique<PingPongFbo>(FboInitParams{
        .glbase = glbase,
        .width = m_width,
        .height = m_height,
        .type = GL_RGBA16F,
        .wrapMode = GL_CLAMP_TO_EDGE
    });
    m_quad    = make_unique<Quad>(QuadInitParams{ .color = {0.f, 0.f, 0.f, 0.f} });
}

void GLSLOpticalFlow::initShader(ShaderCollector* shCol) {
    //------ Position Shader -----------------------------------------

    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                  layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color; uniform mat4 m_pvm;

                  out vec2 tex_coord; out vec4 col;

                  void main() {
                      col         = color;
                      tex_coord   = texCoord;
                      gl_Position = m_pvm * position;
                  });

    vert = "// Optical Flow Shader pos tex vertex shader\n" + shdr_Header + vert;

    //------ Frag Shader -----------------------------------------
    // Optical Flow Shader
    // negative werte werden hier aus irgendwelchen gründen nicht gespeichert...
    // deshalb vorzeichen kodieren, z = vorzeichen x, w = vorzeichen y (0.0 =
    // negativ, 1.0 = positiv)

    shdr_Header      = "#version 410 core\n#pragma optimize(on)\n";
    std::string frag = STRINGIFY(
        uniform sampler2D tex0; uniform sampler2D tex1; uniform vec2 scale; uniform vec2 offset; uniform float amp;
        uniform float m_lambda; uniform float m_median;

        in vec2 tex_coord; in vec4 col; layout(location = 0) out vec4 color;

        vec4 getColorCoded(float x, float y, vec2 scale) {
            vec2  xout = vec2(max(x, 0.), abs(min(x, 0.))) * scale.x;
            vec2  yout = vec2(max(y, 0.), abs(min(y, 0.))) * scale.y;
            float dirY = 1.0;
            if (yout.x > yout.y) dirY = 0.90;
            return vec4(xout.xy, max(yout.x, yout.y), dirY);
        }

        vec4 getGrayScale(vec4 col) {
            float gray = dot(vec3(col.x, col.y, col.z), vec3(0.3, 0.59, 0.11));
            return vec4(gray, gray, gray, 1.0);
        }

        vec4 texture2DRectGray(sampler2D tex, vec2 coord) { return getGrayScale(texture(tex, coord)); }

        void main() {
            vec4 a  = texture2DRectGray(tex0, tex_coord);
            vec4 b  = texture2DRectGray(tex1, tex_coord);
            vec2 x1 = vec2(offset.x, 0.0);
            vec2 y1 = vec2(0.0, offset.y);

            // get the difference
            vec4 curdif = b - a;

            // calculate the gradient
            // for X________________
            vec4 gradx = texture2DRectGray(tex1, tex_coord + x1) - texture2DRectGray(tex1, tex_coord - x1);
            gradx += texture2DRectGray(tex0, tex_coord + x1) - texture2DRectGray(tex0, tex_coord - x1);

            // for Y________________
            vec4 grady = texture2DRectGray(tex1, tex_coord + y1) - texture2DRectGray(tex1, tex_coord - y1);
            grady += texture2DRectGray(tex0, tex_coord + y1) - texture2DRectGray(tex0, tex_coord - y1);

            vec4 gradmag = sqrt((gradx * gradx) + (grady * grady) + vec4(m_lambda));

            vec4 vx = curdif * (gradx / gradmag);
            vec4 vy = curdif * (grady / gradmag);

            //    color = vec4(vx.r, vy.r, 0.0, 1.0) * 3.0;
            color = vec4(pow(abs(vx.r) * 6.0, 1.7) * sign(vx.r) * amp, pow(abs(vy.r) * 6.0, 1.7) * sign(vy.r) * amp,
                         0.0, 1.0);
        });

    frag = "// Optical Flow Shader pos tex shader\n" + shdr_Header + frag;

    m_flowShader = shCol->add("StandardOpticalFlow", vert, frag);
}

void GLSLOpticalFlow::update(GLint tex1, GLint tex2, float fdbk) {
    glEnable(GL_BLEND);

    m_texture->m_dst->bind();
    if (m_isInited < 2) {
        m_texture->m_dst->clear();
        ++m_isInited;
    } else {
        m_texture->m_dst->clearAlpha(fdbk, 0.f);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    m_flowShader->begin();

    if (m_pvm_ptr == nullptr) {
        m_flowShader->setIdentMatrix4fv("m_pvm");
    } else {
        m_flowShader->setUniformMatrix4fv("m_pvm", m_pvm_ptr);
    }

    m_flowShader->setUniform2f("scale", 1.f, 2.f);
    m_flowShader->setUniform2f("offset", 1.f / static_cast<float>(m_width), 1.f / static_cast<float>(m_height));
    m_flowShader->setUniform1f("amp", m_bright);
    m_flowShader->setUniform1f("m_lambda", m_lambda);
    m_flowShader->setUniform1i("tex0", 0);
    m_flowShader->setUniform1i("tex1", 1);
    m_flowShader->setUniform1f("m_median", m_median);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex2);

    m_quad->draw();

    m_texture->m_dst->unbind();
    m_texture->swap();
}

GLuint GLSLOpticalFlow::getResTexId() const {
    return m_texture->getSrcTexId();
}


}  // namespace ara
