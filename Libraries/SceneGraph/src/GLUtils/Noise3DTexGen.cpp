//
//  3DNoiseTexgen.cpp
//
//  Created by Sven Hahne on 18/5/15.
//
//  Generator for a 3D-Texture with perlin noise

#include "Noise3DTexGen.h"

#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {

Noise3DTexGen::Noise3DTexGen(sceneData* scd, bool color, int nrOctaves, int width, int height, int depth,
                             float scaleX, float scaleY, float scaleZ)
    : m_glbase(static_cast<GLBase*>(scd->glbase)), m_shCol(&static_cast<GLBase*>(scd->glbase)->shaderCollector()),
      m_width(width), m_height(height), m_scaleX(scaleX), m_scaleY(scaleY), m_scaleZ(scaleZ) {
    depth   = depth;
    m_nrLoops = depth;

    fbo = make_unique<FBO>(FboInitParams{m_glbase, width, height, depth, GL_RGBA8, GL_TEXTURE_3D, false, 1, 1, 1, GL_REPEAT, false});
    auto xBlendFboH = make_unique<FBO>(FboInitParams{m_glbase, width, height, 1, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false});
    auto xBlendFboV = make_unique<FBO>(FboInitParams{m_glbase, width, height, 1, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false});

    unique_ptr<Quad> quad    = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f});
    unique_ptr<Quad> fboQuad = make_unique<Quad>(QuadInitParams{0.f, 0.f, 1.f, 1.f, vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f});

    initShdr();
    auto xBlendShaderH = initBlendShdrH();
    auto xBlendShaderV = initBlendShdrV();

    m_stdTexShdr = m_shCol->getStdTex();

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // bind the s_fbo and render one layer of perlin noise
    for (short i = 0; i < m_nrLoops; i++) {
        auto zPos = float(i) / float(m_nrLoops - 1);

        // generate perlin noise texture
        fbo->set3DLayer(0, i);
        fbo->bind();

        m_noiseShdr->begin();
        m_noiseShdr->setIdentMatrix4fv("m_pvm");
        m_noiseShdr->setUniform1i("useColor", 1);
        m_noiseShdr->setUniform1f("zPos", (zPos * m_scaleZ));
        m_noiseShdr->setUniform1f("m_scaleX", m_scaleX);
        m_noiseShdr->setUniform1f("m_scaleY", m_scaleY);
        m_noiseShdr->setUniform1i("nrOctaves", std::max<int>(nrOctaves, 1));

        quad->draw();
        fbo->unbind();

        // ---- blend texture perlin noise pattern to be used borderless
        // --------
        // ---- blend horizontal --------

        xBlendFboH->bind();
        xBlendFboH->clear();

        xBlendShaderH->begin();
        xBlendShaderH->setIdentMatrix4fv("m_pvm");
        xBlendShaderH->setUniform1i("tex", 0);
        xBlendShaderH->setUniform1i("width", width);
        xBlendShaderH->setUniform1f("zPos", zPos + 0.5f / float(m_nrLoops - 1));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, fbo->getColorImg());

        quad->draw();

        xBlendFboH->unbind();

        // ---- blend vertical --------

        xBlendFboV->bind();
        xBlendFboV->clear();

        xBlendShaderV->begin();
        xBlendShaderV->setIdentMatrix4fv("m_pvm");
        xBlendShaderV->setUniform1i("tex", 0);
        xBlendShaderV->setUniform1i("height", height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, xBlendFboH->getColorImg());

        quad->draw();

        xBlendFboV->unbind();

        // ---- copy the result ---------

        fbo->set3DLayer(0, i);
        fbo->bind();

        m_stdTexShdr->begin();
        m_stdTexShdr->setIdentMatrix4fv("m_pvm");
        m_stdTexShdr->setUniform1i("tex", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, xBlendFboV->getColorImg());
        quad->draw();

        fbo->unbind();
    }
}

void Noise3DTexGen::initShdr() {
    //------ Position Shader -----------------------------------------

    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    std::string vert        = STRINGIFY(
        layout(location = 0) in vec4 position;\n
        layout(location = 1) in vec3 normal;\n
        layout(location = 2) in vec2 texCoord;\n
        layout(location = 3) in vec4 color;\n
		\n uniform mat4 m_pvm;\n
        out vec4 pos;\n
        out vec2 texCo;\n
		void main() { \n
            pos         = position;\n
            texCo       = texCoord;\n
            gl_Position = m_pvm * position; \n
        });
    vert = "// Noise3DTexGen pos tex vertex shader\n" + shdr_Header + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;

        in vec4 pos;
        in vec2 texCo;
        uniform float scaleX;
        uniform float scaleY;
        uniform float zPos;
        uniform int nrOctaves;
        uniform int useColor;
        float pi = 3.1415926535897932384626433832795;
        int i;
        int col;

        vec2 modTexCoord;
        vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
        vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
        vec4 permute(vec4 x) { return mod289(((x * 34.0) + 1.0) * x); }
        vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

        vec3 fade(vec3 t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

        // Classic Perlin noise
        float cnoise(vec3 P) {
            vec3 Pi0 = floor(P);         // Integer part for indexing
            vec3 Pi1 = Pi0 + vec3(1.0);  // Integer part + 1
            Pi0      = mod289(Pi0);
            Pi1      = mod289(Pi1);
            vec3 Pf0 = fract(P);         // Fractional part for interpolation
            vec3 Pf1 = Pf0 - vec3(1.0);  // Fractional part - 1.0
            vec4 ix  = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
            vec4 iy  = vec4(Pi0.yy, Pi1.yy);
            vec4 iz0 = Pi0.zzzz;
            vec4 iz1 = Pi1.zzzz;

            vec4 ixy  = permute(permute(ix) + iy);
            vec4 ixy0 = permute(ixy + iz0);
            vec4 ixy1 = permute(ixy + iz1);

            vec4 gx0 = ixy0 * (1.0 / 7.0);
            vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
            gx0      = fract(gx0);
            vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
            vec4 sz0 = step(gz0, vec4(0.0));
            gx0 -= sz0 * (step(0.0, gx0) - 0.5);
            gy0 -= sz0 * (step(0.0, gy0) - 0.5);

            vec4 gx1 = ixy1 * (1.0 / 7.0);
            vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
            gx1      = fract(gx1);
            vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
            vec4 sz1 = step(gz1, vec4(0.0));
            gx1 -= sz1 * (step(0.0, gx1) - 0.5);
            gy1 -= sz1 * (step(0.0, gy1) - 0.5);

            vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
            vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
            vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
            vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
            vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
            vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
            vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
            vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

            vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
            g000 *= norm0.x;
            g010 *= norm0.y;
            g100 *= norm0.z;
            g110 *= norm0.w;
            vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
            g001 *= norm1.x;
            g011 *= norm1.y;
            g101 *= norm1.z;
            g111 *= norm1.w;

            float n000 = dot(g000, Pf0);
            float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
            float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
            float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
            float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
            float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
            float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
            float n111 = dot(g111, Pf1);

            vec3  fade_xyz = fade(Pf0);
            vec4  n_z      = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
            vec2  n_yz     = mix(n_z.xy, n_z.zw, fade_xyz.y);
            float n_xyz    = mix(n_yz.x, n_yz.y, fade_xyz.x);
            return 2.2 * n_xyz;
        }

        void main() {
            modTexCoord = vec2(m_scaleX * texCo.x, m_scaleY * texCo.y);
            if (useColor == 1) {
                vec3 noise = vec3(0.0);
                for (col = 0; col < 3; col++)
                    for (i = 0; i < nrOctaves; i++)
                        noise[col] += cnoise(vec3(modTexCoord * pow(2.0, float(i)), zPos + float(col) * 0.3));
                // noise /= max(nrOctaves / 2, 1);
                fragColor = vec4(noise, 1.0);
            } else {
                float noise = 0.0;
                for (i = 0; i < nrOctaves; i++) noise += cnoise(vec3(modTexCoord * pow(2.0, float(i)), zPos));
                // noise /= max(nrOctaves / 2, 1);
                fragColor = vec4(noise, noise, noise, 1.0);
            }
        });
    frag = "// Noise3DTexGen pos tex shader\n" + shdr_Header + frag;

    m_noiseShdr = m_shCol->add("noise3DTexGen", vert.c_str(), frag.c_str());
}

ara::Shaders* Noise3DTexGen::initBlendShdrH() {
    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    std::string vert        = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                                        layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;
                                        uniform mat4 m_pvm; uniform float zPos;

                                        out vec3 tex_coordLeft;   // shift, + stretch, left edge stays
                                        out vec3 tex_coordRight;  // shift, + stretch, right edge stays
                                        out vec4 col;

                                        void main() {
                                     col = color;
                                     // shift texCoord, edges will be in the center
                                     tex_coordLeft  = vec3(texCoord.x * 0.5 + 0.5, texCoord.y + 0.5, zPos);
                                     tex_coordRight = vec3(texCoord.x * 0.5 + 1.0, texCoord.y + 0.5, zPos);
                                     gl_Position    = m_pvm * position;
                                        });
    vert = "// Noise3DTexGen blendH vertex Shader\n" + shdr_Header + vert;

    std::string frag = STRINGIFY(uniform sampler3D tex; uniform int width;

                  in vec3 tex_coordLeft; in vec3 tex_coordRight; in vec4 col; layout(location = 0) out vec4 color;

                  void main() {
                      float blendPos = gl_FragCoord.x / float(width);
                      vec4  left     = texture(tex, tex_coordLeft);
                      vec4  right    = texture(tex, tex_coordRight);

                      color = mix(left, right, blendPos);
                      color.a = 1.0;
                  });
    frag = "// Noise3DTexGen blendH tex shader\n" + shdr_Header + frag;

    return new ara::Shaders(vert.c_str(), frag.c_str(), false);
}

ara::Shaders* Noise3DTexGen::initBlendShdrV() {
    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                  layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color; uniform mat4 m_pvm;

                  out vec2 tex_coordTop;     // shift, + stretch, Top edge stays
                  out vec2 tex_coordBottom;  // shift, + stretch, bottom edge stays
                  out vec4 col;

                  void main() {
                      col             = color;
                      tex_coordTop    = vec2(texCoord.x, texCoord.y * 0.5 + 1.0);
                      tex_coordBottom = vec2(texCoord.x, texCoord.y * 0.5 + 0.5);
                      gl_Position     = m_pvm * position;
                  });
    vert = "// Noise3DTexGen blendV vertex Shader\n" + shdr_Header + vert;

    std::string frag = STRINGIFY(uniform sampler2D tex; uniform int height;

                                 in vec2 tex_coordTop;     // shift, + stretch, left edge stays
                                 in vec2 tex_coordBottom;  // shift, + stretch, right edge stays
                                 in vec4 col; layout(location = 0) out vec4 color;

                                 void main() {
                                     float blendPos = gl_FragCoord.y / float(height);
                                     vec4  top      = texture(tex, tex_coordTop);
                                     vec4  bottom   = texture(tex, tex_coordBottom);
                                     color          = mix(top, bottom, blendPos);
                                     color.a        = 1.0;
                                 });
    frag             = "// Noise3DTexGen blendV tex shader\n" + shdr_Header + frag;

    return new ara::Shaders(vert.c_str(), frag.c_str(), false);
}

GLuint Noise3DTexGen::getTex() {
    return fbo->getColorImg();
}

}  // namespace ara
