//
//  FastBlurMem.cpp
//
//  Created by Sven Hahne on 15/5/15.
//

#include "GLUtils/FastBlurMem.h"

#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {
FastBlurMem::FastBlurMem(GLBase* glbase, float alpha, int blurW, int blurH, GLenum target, GLenum intFormat,
                         uint nrLayers, bool rot180, blurKernelSize kSize, bool singleFbo)
    : m_glbase(glbase), m_alpha(alpha), m_blurW(blurW), m_blurH(blurH), m_bright(1.f), m_intFormat(intFormat), m_kSize(kSize),
      m_nrLayers(nrLayers), m_rot180(rot180), m_pp(nullptr), m_firstPassFbo(nullptr), m_singleFbo(singleFbo), m_target(target) {
    m_fboQuad = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f});

    if (m_nrLayers > 0) {
        initShader();
        initFbo();
    }
}

void FastBlurMem::initFbo() {
    m_fWidth  = static_cast<float>(m_blurW);
    m_fHeight = static_cast<float>(m_blurH);

    m_actKernelSize = m_kSize == KERNEL_3 ? 3 : 5;

    m_blurOffs.resize(m_actKernelSize);
    for (uint i = 0; i < m_actKernelSize; i++) {
        m_blurOffs[i] = float(i) / m_fWidth;
    }

    m_blurOffsScale.resize(m_actKernelSize);

    if (m_pp) {
        m_pp.reset();
    }

    if (m_target == GL_TEXTURE_2D_ARRAY) {
        m_pp = make_unique<PingPongFbo>(FboInitParams{m_glbase, m_blurW, m_blurH, m_nrLayers, m_intFormat, m_target, false, 1, 1, 1, GL_CLAMP_TO_BORDER,
                                                      true});
    } else {
        m_pp = make_unique<PingPongFbo>(FboInitParams{m_glbase, m_blurW, m_blurH, 1, m_intFormat, m_target, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false});
    }

    if (m_intFormat != GL_R32F && m_intFormat != GL_RG32F && m_intFormat != GL_RGB32F && m_intFormat != GL_RGBA32F &&
        m_intFormat != GL_R16F && m_intFormat != GL_RG16F && m_intFormat != GL_RGB16F && m_intFormat != GL_RGBA16F) {
        m_pp->setMagFilter(GL_LINEAR);
        m_pp->setMinFilter(GL_LINEAR);
    } else {
        m_pp->setMagFilter(GL_NEAREST);
        m_pp->setMinFilter(GL_NEAREST);
    }

    m_pp->clear();

    if (m_target == GL_TEXTURE_2D_ARRAY)
        m_firstPassFbo = make_unique<FBO>(FboInitParams{m_glbase, m_blurW, m_blurH, m_nrLayers, m_intFormat, m_target, false, 1, 1, 1, GL_CLAMP_TO_BORDER, true});
    else
        m_firstPassFbo = make_unique<FBO>(FboInitParams{m_glbase, m_blurW, m_blurH, 1, m_intFormat, m_target, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false});

    m_firstPassFbo->bind();
    m_firstPassFbo->clear();
    m_firstPassFbo->unbind();

    if (m_intFormat != GL_R32F && m_intFormat != GL_RG32F && m_intFormat != GL_RGB32F && m_intFormat != GL_RGBA32F &&
        m_intFormat != GL_R16F && m_intFormat != GL_RG16F && m_intFormat != GL_RGB16F && m_intFormat != GL_RGBA16F) {
        m_firstPassFbo->setMagFilter(GL_LINEAR);
        m_firstPassFbo->setMinFilter(GL_LINEAR);
    } else {
        m_firstPassFbo->setMagFilter(GL_NEAREST);
        m_firstPassFbo->setMinFilter(GL_NEAREST);
    }
}

void FastBlurMem::proc(GLint texIn) {
    glEnable(GL_BLEND);

    for (uint i = 0; i < m_actKernelSize; i++) {
        m_blurOffsScale[i] = m_blurOffs[i] * m_offsScale;
    }

    m_firstPassFbo->bind();

    if (m_intFormat != GL_R32F && m_intFormat != GL_RG32F && m_intFormat != GL_RGB32F && m_intFormat != GL_RGBA32F &&
        m_intFormat != GL_R16F && m_intFormat != GL_RG16F && m_intFormat != GL_RGB16F && m_intFormat != GL_RGBA16F)
        m_firstPassFbo->clearAlpha(m_alpha, 0.f);
    else
        m_firstPassFbo->clear();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // linear vertical blur
    m_linearV->begin();
    m_linearV->setUniform1i("image", 0);
    m_linearV->setUniform1fv("offset", &m_blurOffsScale[0], m_actKernelSize);
    m_linearV->setUniform1f("rot180", m_rot180 ? -1.f : 1.f);
    m_linearV->setUniform1f("weightScale", m_weightScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(m_target, texIn);

    m_fboQuad->draw();
    m_linearV->end();

    m_firstPassFbo->unbind();

    //-----------------------------------------------------------------

    // linear horizontal blur
    m_pp->dst->bind();
    m_pp->dst->clear();

    glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

    m_linearH->begin();
    m_linearH->setUniform1i("image", 0);
    m_linearH->setUniform1f("bright", m_bright);
    m_linearH->setUniform1f("weightScale", m_weightScale);
    m_linearH->setUniform1fv("offset", &m_blurOffsScale[0], m_actKernelSize);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(m_target, m_firstPassFbo->getColorImg());

    m_fboQuad->draw();
    m_linearH->end();

    m_pp->dst->unbind();

    if (!m_singleFbo) {
        m_pp->swap();
    }
}

void FastBlurMem::initShader() {
    std::string vert;

    if (m_target == GL_TEXTURE_2D_ARRAY)
        vert = STRINGIFY(
            layout(location = 0) in vec4 position;\n layout(location = 2) in vec2 texCoord;\n out vec2 gs_tex_coord;\n void
                main() {
                    \n
		\t gs_tex_coord = texCoord;
                    \n
		\t gl_Position  = vec4(position.xy, 0.0, 1.0);
                    \n
                });
    else
        vert = STRINGIFY(
            layout(location = 0) in vec4 position;\n layout(location = 2) in vec2 texCoord;\n out vec2 tex_coord;\n void
                                                                                                       main() {
                    \n
		\t tex_coord = texCoord;
                    \n
		\t gl_Position = vec4(position.xy, 0.0, 1.0);
                    \n
                });

    vert = "// FastBlur vertex shader\n" + m_glbase->shaderCollector().getShaderHeader() + vert;

    std::string geom;
    if (m_target == GL_TEXTURE_2D_ARRAY) {
        geom = m_glbase->shaderCollector().getShaderHeader() + "// FastBlurmem geom Shader\n";
        geom += "layout(triangles, invocations=" + std::to_string(m_nrLayers) + ") in;\n";

        geom += STRINGIFY(
            layout(triangle_strip, max_vertices = 3) out;\n in vec2 gs_tex_coord[];\n out vec2 tex_coord;\n void
                                                                                               main()  \n {
                    \n for (int i = 0; i < gl_in.length(); i++) \n {
                        \n gl_Layer = gl_InvocationID;
                        \n gl_Position = gl_in[i].gl_Position;
                        \n tex_coord = gs_tex_coord[i];
                        \n EmitVertex();
                        \n
                    }
                    \n EndPrimitive();
                    \n
                } \n);
    }

    std::string frag;
    if (m_target != GL_TEXTURE_2D_ARRAY)
        frag = "uniform sampler2D image;\n";
    else
        frag = "uniform sampler2DArray image;\n";

    frag += STRINGIFY(
        uniform float rot180;\n uniform float weightScale;\n vec2 flipTexCoord;\n vec4 col;\n in vec2 tex_coord;\n out vec4 FragmentColor;\n);

    if (m_kSize == KERNEL_3) {
        frag +=
            "uniform float offset[3];\n"
            "uniform float weight[3] = float[] (0.2270270270, 0.3162162162, "
            "0.0702702703);\n";
    } else {
        frag +=
            "uniform float offset[5];\n"
            "uniform float weight[5] = float[] (0.227027, 0.1945946, "
            "0.1216216, 0.054054, 0.016216);\n";
    }

    frag +=
        "void main(){ \n"
        "\t flipTexCoord = vec2(tex_coord.x * rot180, tex_coord.y * rot180);\n";

    if (m_target == GL_TEXTURE_2D_ARRAY)
        frag += "\t col = texture(image, vec3(flipTexCoord, float(gl_Layer)));\n";
    else
        frag += "\t col = texture(image, flipTexCoord);\n";

    frag += "\t FragmentColor = col * weight[0] * weightScale;\n";

    if (m_kSize == KERNEL_3)
        frag += "\t for (int i=1; i<3; i++) {\n";
    else if (m_kSize == KERNEL_5)
        frag += "\t for (int i=1; i<5; i++) {\n";

    if (m_target == GL_TEXTURE_2D_ARRAY)
        frag +=
            "\t\t FragmentColor += texture(image, vec3(flipTexCoord + "
            "vec2(0.0, offset[i]), float(gl_Layer))) * weight[i] * "
            "weightScale;\n"
            "\t\t FragmentColor += texture(image, vec3(flipTexCoord - "
            "vec2(0.0, offset[i]), float(gl_Layer))) * weight[i] * "
            "weightScale;\n \t} \n}";
    else
        frag +=
            "\t\t FragmentColor += texture(image, flipTexCoord + vec2(0.0, "
            "offset[i])) * weight[i] * weightScale;\n"
            "\t\t FragmentColor += texture(image, flipTexCoord - vec2(0.0, "
            "offset[i])) * weight[i] * weightScale;\n \t} \n}";

    frag = "// FastBlurMem Vertical fragment shader\n" + m_glbase->shaderCollector().getShaderHeader() + frag;

    if (m_target == GL_TEXTURE_2D_ARRAY)
        m_linearV = m_glbase->shaderCollector().add("FastBlurMemVShader_" + std::to_string(m_nrLayers), vert.c_str(),
                                                    geom.c_str(), frag.c_str());
    else
        m_linearV = m_glbase->shaderCollector().add("FastBlurMemVShader", vert.c_str(), frag.c_str());

    //===================================================================================

    if (m_target != GL_TEXTURE_2D_ARRAY)
        frag = "uniform sampler2D image;\n";
    else
        frag = "uniform sampler2DArray image;\n";

    frag += STRINGIFY(uniform float bright;\n uniform float weightScale;\n in vec2 tex_coord;\n out vec4 FragmentColor;\n);

    if (m_kSize == KERNEL_3) {
        frag +=
            "uniform float offset[3];\n"
            "uniform float weight[3] = float[] (0.2270270270, 0.3162162162, "
            "0.0702702703 );\n";
    } else {
        frag +=
            "uniform float offset[5];\n"
            "uniform float weight[5] = float[] (0.227027, 0.1945946, "
            "0.1216216, 0.054054, 0.016216);\n";
    }

    if (m_target == GL_TEXTURE_2D_ARRAY) {
        frag +=
            "void main(void){\n"
            "FragmentColor = texture( image, vec3( tex_coord, float(gl_Layer) "
            ") ) * weight[0] * weightScale;\n";
    } else {
        frag +=
            "void main(void){\n"
            "FragmentColor = texture( image, tex_coord ) * weight[0] * "
            "weightScale;\n";
    }

    if (m_kSize == KERNEL_3)
        frag += "for (int i=1; i<3; i++) {\n";
    else if (m_kSize == KERNEL_5)
        frag += "for (int i=1; i<5; i++) {\n";

    if (m_target == GL_TEXTURE_2D_ARRAY) {
        frag +=
            "FragmentColor += texture( image, vec3( tex_coord + "
            "vec2(offset[i], 0.0), float(gl_Layer) ) ) * weight[i] * "
            "weightScale;\n"
            "FragmentColor += texture( image, vec3( tex_coord - "
            "vec2(offset[i], 0.0), float(gl_Layer) ) ) * weight[i] * "
            "weightScale;\n }\n";
    } else {
        frag +=
            "FragmentColor += texture( image, tex_coord + vec2(offset[i], 0.0) "
            ") * weight[i] * weightScale;\n"
            "FragmentColor += texture( image, tex_coord - vec2(offset[i], 0.0) "
            ") * weight[i] * weightScale;\n }\n";
    }

    frag += "FragmentColor = FragmentColor * bright; \n";
    frag += "}\n";

    frag = "// FastBlurMem horizontal fragment shader\n" + m_glbase->shaderCollector().getShaderHeader() + frag;

    if (m_target == GL_TEXTURE_2D_ARRAY) {
        m_linearH = m_glbase->shaderCollector().add("FastBlurMemHShader_" + std::to_string(m_nrLayers), vert.c_str(),
                                                    geom.c_str(), frag.c_str());
    } else {
        m_linearH = m_glbase->shaderCollector().add("FastBlurMemHShader", vert.c_str(), frag.c_str());
    }
}

}  // namespace ara
