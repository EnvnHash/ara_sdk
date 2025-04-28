//
//  GLSLOpticalFlowDepth.cpp
//  tav_core
//
//  Created by Sven Hahne on 13/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "GLSLOpticalFlowDepth.h"
#include <GLBase.h>
#include <GeoPrimitives/Quad.h>
#include <Utils/FBO.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/PingPongFbo.h>
#include <Utils/Texture.h>
#include <Utils/UniformBlock.h>

using namespace glm;
using namespace std;

namespace ara {

GLSLOpticalFlowDepth::GLSLOpticalFlowDepth(GLBase* glbase, int width, int height)
    : m_glbase(glbase),
        m_shCol(&glbase->shaderCollector()),
        m_width(width),
        m_height(height),
        m_srcId(0),
        m_lambda(0.1f),
        m_median(3.f),
        m_bright(4.f) {
    m_texShader = m_shCol->getStdTex();
    m_texture   = make_unique<PingPongFbo>(FboInitParams{glbase, m_width, m_height, 1, GL_RGBA16F, GL_TEXTURE_2D, false, 2, 1, GL_CLAMP_TO_EDGE});
    m_texture->clear();
    m_quad = make_unique<Quad>(QuadInitParams{ .color = {0.f, 0.f, 0.f, 0.f} });

    initShaders();
}

void GLSLOpticalFlowDepth::update() {
    m_texture->dst->bind();
    m_texture->dst->clear();

    m_flowShader->begin();
    m_flowShader->setIdentMatrix4fv("m_pvm");

    m_flowShader->setUniform2f("scale", 1.f, 2.f);
    m_flowShader->setUniform2f("offset", 1.f / static_cast<float>(m_width), 1.f / static_cast<float>(m_height));
    m_flowShader->setUniform1f("amp", m_bright);
    m_flowShader->setUniform1f("lambda", m_lambda);
    m_flowShader->setUniform1i("tex0", 0);
    m_flowShader->setUniform1i("tex1", 1);
    m_flowShader->setUniform1i("last", 2);
    m_flowShader->setUniform1f("median", m_median);
    m_flowShader->setUniform1f("maxDepth", 1.f);
    m_flowShader->setUniform1f("maxDist", m_maxDist);
    m_flowShader->setUniform1f("diffAmp", m_diffAmp);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_srcId);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_lastSrcId);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_texture->getSrcTexId());

    m_quad->draw();

    m_texture->dst->unbind();
    m_texture->swap();
}

void GLSLOpticalFlowDepth::initShaders() {
    std::string shdr_Header = "#version 410\n\n";

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                                 layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;

                                 uniform mat4 m_pvm; out vec2 tex_coord; out vec4 col;

                                 void main() {
                                     col         = color;
                                     tex_coord   = texCoord;
                                     gl_Position = m_pvm * position;
                                 });

    vert = shdr_Header + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex0; uniform sampler2D tex1; uniform sampler2D last;

        uniform vec2 scale; uniform vec2 offset; uniform float maxDist; uniform float maxDepth; uniform float amp;
        uniform float lambda; uniform float median; uniform float diffAmp;

        in vec2 tex_coord; in vec4 col; layout(location = 0) out vec4 color; layout(location = 1) out vec4 diff;

        vec4 getColorCoded(float x, float y, vec2 scale) {
            vec2  xout = vec2(max(x, 0.), abs(min(x, 0.))) * scale.x;
            vec2  yout = vec2(max(y, 0.), abs(min(y, 0.))) * scale.y;
            float dirY = 1.0;
            if (yout.x > yout.y) dirY = 0.90;
            return vec4(xout.xy, max(yout.x, yout.y), dirY);
        }

        vec4 getGrayScale(vec4 col) {
            float gray = col.r / maxDepth;
            return vec4(gray, gray, gray, 1.0);
        }

        vec4 texture2DRectGray(sampler2D tex, vec2 coord) { return getGrayScale(texture(tex, coord)); }

        void main() {
            float srcDepth  = texture(tex0, tex_coord).r / maxDepth;
            float lastDepth = texture(tex1, tex_coord).r / maxDepth;

            vec4 a  = texture2DRectGray(tex0, tex_coord);
            vec4 b  = texture2DRectGray(tex1, tex_coord);
            vec2 x1 = vec2(offset.x, 0.);
            vec2 y1 = vec2(0., offset.y);

            // get the difference
            vec4 curdif = b - a;

            // calculate the gradient
            // for X________________
            vec4 gradx = texture2DRectGray(tex1, tex_coord + x1) - texture2DRectGray(tex1, tex_coord - x1);
            gradx += texture2DRectGray(tex0, tex_coord + x1) - texture2DRectGray(tex0, tex_coord - x1);

            // for Y________________
            vec4 grady = texture2DRectGray(tex1, tex_coord + y1) - texture2DRectGray(tex1, tex_coord - y1);
            grady += texture2DRectGray(tex0, tex_coord + y1) - texture2DRectGray(tex0, tex_coord - y1);

            vec4 gradmag = sqrt((gradx * gradx) + (grady * grady) + vec4(lambda));

            vec4 vx = curdif * (gradx / gradmag);
            vec4 vy = curdif * (grady / gradmag);


            color = vec4(pow(abs(vx.r) * 6.0, 1.7) * sign(vx.r) * amp, pow(abs(vy.r) * 6.0, 1.7) * sign(vy.r) * amp,
                         (srcDepth - lastDepth) * amp, 1.0);

            float diffV = pow(abs(srcDepth - lastDepth) * diffAmp, 2.0);
            diff = vec4(diffV, diffV, diffV, 1.0);
        });

    frag = shdr_Header + frag;

    m_flowShader = m_shCol->add("GLSLOpticalFlowDepth", vert.c_str(), frag.c_str());
}

GLuint GLSLOpticalFlowDepth::getResTexId() {
    return m_texture->getSrcTexId();
}

GLuint GLSLOpticalFlowDepth::getDiffTexId() {
    return m_texture->src->getColorImg(1);
}

}  // namespace ara
