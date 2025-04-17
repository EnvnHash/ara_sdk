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
FastBlurMem::FastBlurMem(GLBase* glbase, float _alpha, int _blurW, int _blurH, GLenum _target, GLenum _intFormat,
                         uint _nrLayers, bool _rot180, blurKernelSize _kSize, bool _singleFbo)
    : m_glbase(glbase), alpha(_alpha), blurW(_blurW), blurH(_blurH), bright(1.f), intFormat(_intFormat), kSize(_kSize),
      nrLayers(_nrLayers), rot180(_rot180), pp(nullptr), firstPassFbo(nullptr), singleFbo(_singleFbo), target(_target) {
    fboQuad = make_unique<Quad>(-1.f, -1.f, 2.f, 2.f);

    if (_nrLayers > 0) {
        initShader();
        initFbo();
    }
}

void FastBlurMem::initFbo() {
    fWidth  = static_cast<float>(blurW);
    fHeight = static_cast<float>(blurH);

    actKernelSize = kSize == KERNEL_3 ? 3 : 5;

    blurOffs.resize(actKernelSize);
    for (uint i = 0; i < actKernelSize; i++) blurOffs[i] = float(i) / fWidth;

    blurOffsScale.resize(actKernelSize);

    if (pp) delete pp;
    if (target == GL_TEXTURE_2D_ARRAY)
        pp = new PingPongFbo(m_glbase, blurW, blurH, nrLayers, intFormat, target, false, 1, 1, 1, GL_CLAMP_TO_BORDER,
                             true);
    else
        pp = new PingPongFbo(m_glbase, blurW, blurH, intFormat, target, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

    if (intFormat != GL_R32F && intFormat != GL_RG32F && intFormat != GL_RGB32F && intFormat != GL_RGBA32F &&
        intFormat != GL_R16F && intFormat != GL_RG16F && intFormat != GL_RGB16F && intFormat != GL_RGBA16F) {
        pp->setMagFilter(GL_LINEAR);
        pp->setMinFilter(GL_LINEAR);
    } else {
        pp->setMagFilter(GL_NEAREST);
        pp->setMinFilter(GL_NEAREST);
    }

    pp->clear();

    if (firstPassFbo) delete firstPassFbo;
    if (target == GL_TEXTURE_2D_ARRAY)
        firstPassFbo =
            new FBO(m_glbase, blurW, blurH, nrLayers, intFormat, target, false, 1, 1, 1, GL_CLAMP_TO_BORDER, true);
    else
        firstPassFbo = new FBO(m_glbase, blurW, blurH, intFormat, target, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

    firstPassFbo->bind();
    firstPassFbo->clear();
    firstPassFbo->unbind();

    if (intFormat != GL_R32F && intFormat != GL_RG32F && intFormat != GL_RGB32F && intFormat != GL_RGBA32F &&
        intFormat != GL_R16F && intFormat != GL_RG16F && intFormat != GL_RGB16F && intFormat != GL_RGBA16F) {
        firstPassFbo->setMagFilter(GL_LINEAR);
        firstPassFbo->setMinFilter(GL_LINEAR);
    } else {
        firstPassFbo->setMagFilter(GL_NEAREST);
        firstPassFbo->setMinFilter(GL_NEAREST);
    }
}

FastBlurMem::~FastBlurMem() {
    delete pp;
    delete firstPassFbo;
}

void FastBlurMem::proc(GLint texIn) {
    glEnable(GL_BLEND);

    for (uint i = 0; i < actKernelSize; i++) blurOffsScale[i] = blurOffs[i] * offsScale;

    firstPassFbo->bind();

    if (intFormat != GL_R32F && intFormat != GL_RG32F && intFormat != GL_RGB32F && intFormat != GL_RGBA32F &&
        intFormat != GL_R16F && intFormat != GL_RG16F && intFormat != GL_RGB16F && intFormat != GL_RGBA16F)
        firstPassFbo->clearAlpha(alpha, 0.f);
    else
        firstPassFbo->clear();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // linear vertical blur
    linearV->begin();
    linearV->setUniform1i("image", 0);
    linearV->setUniform1fv("offset", &blurOffsScale[0], actKernelSize);
    linearV->setUniform1f("rot180", rot180 ? -1.f : 1.f);
    linearV->setUniform1f("weightScale", weightScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texIn);

    fboQuad->draw();
    linearV->end();

    firstPassFbo->unbind();

    //-----------------------------------------------------------------

    // linear horizontal blur
    pp->dst->bind();
    pp->dst->clear();

    glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

    linearH->begin();
    linearH->setUniform1i("image", 0);
    linearH->setUniform1f("bright", bright);
    linearH->setUniform1f("weightScale", weightScale);
    linearH->setUniform1fv("offset", &blurOffsScale[0], actKernelSize);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, firstPassFbo->getColorImg());

    fboQuad->draw();
    linearH->end();

    pp->dst->unbind();

    if (!singleFbo) pp->swap();
}

void FastBlurMem::initShader() {
    std::string vert;

    if (target == GL_TEXTURE_2D_ARRAY)
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
    if (target == GL_TEXTURE_2D_ARRAY) {
        geom = m_glbase->shaderCollector().getShaderHeader() + "// FastBlurmem geom Shader\n";
        geom += "layout(triangles, invocations=" + std::to_string(nrLayers) + ") in;\n";

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
    if (target != GL_TEXTURE_2D_ARRAY)
        frag = "uniform sampler2D image;\n";
    else
        frag = "uniform sampler2DArray image;\n";

    frag += STRINGIFY(
        uniform float rot180;\n uniform float weightScale;\n vec2 flipTexCoord;\n vec4 col;\n in vec2 tex_coord;\n out vec4 FragmentColor;\n);

    if (kSize == KERNEL_3) {
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

    if (target == GL_TEXTURE_2D_ARRAY)
        frag += "\t col = texture(image, vec3(flipTexCoord, float(gl_Layer)));\n";
    else
        frag += "\t col = texture(image, flipTexCoord);\n";

    frag += "\t FragmentColor = col * weight[0] * weightScale;\n";

    if (kSize == KERNEL_3)
        frag += "\t for (int i=1; i<3; i++) {\n";
    else if (kSize == KERNEL_5)
        frag += "\t for (int i=1; i<5; i++) {\n";

    if (target == GL_TEXTURE_2D_ARRAY)
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

    if (target == GL_TEXTURE_2D_ARRAY)
        linearV = m_glbase->shaderCollector().add("FastBlurMemVShader_" + std::to_string(nrLayers), vert.c_str(),
                                                  geom.c_str(), frag.c_str());
    else
        linearV = m_glbase->shaderCollector().add("FastBlurMemVShader", vert.c_str(), frag.c_str());

    //===================================================================================

    if (target != GL_TEXTURE_2D_ARRAY)
        frag = "uniform sampler2D image;\n";
    else
        frag = "uniform sampler2DArray image;\n";

    frag +=
        STRINGIFY(uniform float bright;\n uniform float weightScale;\n in vec2 tex_coord;\n out vec4 FragmentColor;\n);

    if (kSize == KERNEL_3) {
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

    if (target == GL_TEXTURE_2D_ARRAY) {
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

    if (kSize == KERNEL_3)
        frag += "for (int i=1; i<3; i++) {\n";
    else if (kSize == KERNEL_5)
        frag += "for (int i=1; i<5; i++) {\n";

    if (target == GL_TEXTURE_2D_ARRAY) {
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

    if (target == GL_TEXTURE_2D_ARRAY)
        linearH = m_glbase->shaderCollector().add("FastBlurMemHShader_" + std::to_string(nrLayers), vert.c_str(),
                                                  geom.c_str(), frag.c_str());
    else
        linearH = m_glbase->shaderCollector().add("FastBlurMemHShader", vert.c_str(), frag.c_str());
}

}  // namespace ara
