//
// Created by sven on 20-07-22.
//

#include "GLUtils/GLSLChromaKey.h"

using namespace std;

namespace ara {

bool GLSLChromaKey::init(GLBase* glbase, Property<GLSLChromaKeyPar*>* par) {
    m_glbase = glbase;
    if (!par) return false;
    m_par = par;

    m_rgbFbo  = make_unique<FBO>(FboInitParams{m_glbase, (*par)()->width, (*par)()->height, 1, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 2,
                                 GL_CLAMP_TO_BORDER, false});
    m_procFbo = make_unique<FBO>(FboInitParams{m_glbase, (*par)()->width, (*par)()->height, 1, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 2,
                                 GL_CLAMP_TO_BORDER, false});
    m_maskFbo = make_unique<FBO>(FboInitParams{m_glbase, (*par)()->width, (*par)()->height, 1, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 2,
                                 GL_CLAMP_TO_BORDER, false});
    m_histo   = make_unique<GLSLHistogram>(m_glbase, (*par)()->width, (*par)()->height, 1, GL_RGBA8, 4,
                                           256);  // m_maxValPerChan, values not stable ... some bug
    m_fastBlurMem = make_unique<FastBlurMem>(m_glbase, 0.3f, (*par)()->width, (*par)()->height);

    m_downBuf = std::vector<uint8_t>((*par)()->width * (*par)()->height * 4);

    m_hFlipQuad = make_unique<Quad>(QuadInitData{-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 1.f, nullptr, 1,
                                    true});  // create a Quad, standard width and height (normalized into -1|1), static red

    m_quad = make_unique<Quad>(QuadInitData{-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 1.f, nullptr, 1,
                               false});  // create a Quad, standard width and height (normalized into -1|1), static red

    m_stdTex = m_glbase->shaderCollector().getStdTex();

    //----------------------------------------------------

    m_keyCol[0] = (*m_par)()->keyCol_r.ptr;
    m_keyCol[1] = (*m_par)()->keyCol_g.ptr;
    m_keyCol[2] = (*m_par)()->keyCol_b.ptr;

    m_keyColUpdtFunc = [this](std::any f) {
        for (int i = 0; i < 3; i++) m_keyColTmp[i] = (*m_keyCol[i])() / 255.f;
        m_keyCC.x = RGBAToCC(m_keyColTmp).x;
        m_keyCC.y = RGBAToCC(m_keyColTmp).y;
    };

    onChanged<float>((*m_par)()->keyCol_r.ptr, m_keyColUpdtFunc);
    onChanged<float>((*m_par)()->keyCol_g.ptr, m_keyColUpdtFunc);
    onChanged<float>((*m_par)()->keyCol_b.ptr, m_keyColUpdtFunc);
    m_keyColUpdtFunc(any_cast<float>(1.f));

    //----------------------------------------------------

    m_keyCol2[0] = (*m_par)()->keyCol2_r.ptr;
    m_keyCol2[1] = (*m_par)()->keyCol2_g.ptr;
    m_keyCol2[2] = (*m_par)()->keyCol2_b.ptr;

    m_keyCol2UpdtFunc = [this](std::any f) {
        for (int i = 0; i < 3; i++) m_keyColTmp2[i] = (*m_keyCol2[i])() / 255.f;
        m_keyCC.z = RGBAToCC(m_keyColTmp2).x;
        m_keyCC.w = RGBAToCC(m_keyColTmp2).y;
    };

    onChanged<float>((*m_par)()->keyCol2_r.ptr, m_keyCol2UpdtFunc);
    onChanged<float>((*m_par)()->keyCol2_g.ptr, m_keyCol2UpdtFunc);
    onChanged<float>((*m_par)()->keyCol2_b.ptr, m_keyCol2UpdtFunc);
    m_keyCol2UpdtFunc(any_cast<float>(1.f));

    m_inited = true;

    return true;
}

void GLSLChromaKey::initChromaMaskShdr() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });
    vert = m_glbase->shaderCollector().getShaderHeader() + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n
        in vec2 tex_coord; \n
        layout(location = 0) out vec4 color; \n
        \n
        uniform vec4 keyCC;  \n    // the CC part of YCC color model of key color
        uniform vec3 keyRGB;  \n    // the CC part of YCC color model of key color
        uniform vec3 keyRGB2;  \n    // the CC part of YCC color model of key color
        uniform vec2 range;  \n    // the smoothstep range
        uniform vec2 range2;  \n    // the smoothstep range
        uniform vec2 maskCut;  \n
        uniform float maskGamma;  \n
        uniform float maskGamma2;  \n
        uniform int actMask1;  \n
        uniform int actMask2;  \n
        uniform int rgbColorSpace;  \n
        \n
        vec3 RGBToCC(vec4 rgba) {\n
            float Y = 0.299 * rgba.r + 0.587 * rgba.g + 0.114 * rgba.b;\n
            return vec3(Y, (rgba.b - Y) * 0.565, (rgba.r - Y) * 0.713);\n
        }\n
        void main() {\n
            vec4 src1Color = texture(tex, tex_coord);\n
            vec3 ycc = RGBToCC(src1Color);\n
            \n
            float mask = bool(rgbColorSpace) ?  pow(smoothstep(range.x, range.y, distance(src1Color.rgb, keyRGB)), maskGamma) :
            pow(smoothstep(range.x, range.y, distance(ycc.gb, keyCC.xy)), maskGamma);\n

            float mask2 = bool(rgbColorSpace) ? pow(smoothstep(range2.x, range2.y, distance(src1Color.rgb, keyRGB2)), maskGamma2) :
            pow(smoothstep(range2.x, range2.y, distance(ycc.gb, keyCC.zw)), maskGamma2);\n

            color = vec4( min( max(
                (1.0 - smoothstep(maskCut.x, maskCut.y, mask)) * float(actMask1)
                + (1.0 - smoothstep(maskCut.x, maskCut.y, mask2)) * float(actMask2), 0.0 ), 1.0) ) ;\n
    });

    frag             = m_glbase->shaderCollector().getShaderHeader() + frag;
    m_chromaMaskShdr = m_glbase->shaderCollector().add("greenScreenMaskChroma", vert, frag);
}

void GLSLChromaKey::initChromaShdr() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 1) in vec4 normal; \n layout(location = 2) in vec2 texCoord; \n layout(location = 3) in vec4 color; \n uniform mat4 m_pvm; \n out vec2 tex_coord; \n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });
    vert = m_glbase->shaderCollector().getShaderHeader() + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n
        uniform sampler2D maskTex; \n
        in vec2 tex_coord; \n
        layout(location = 0) out vec4 color; \n
        \n
        uniform vec4 keyCC;  \n    // the CC part of YCC color model of key color
        uniform vec2 spillRange;  \n
        uniform float satu;  \n
        uniform int doKey;  \n
        uniform int showMask;  \n
        uniform int showHue;  \n
        uniform int showSat;  \n
        uniform int removeSpill;  \n
        uniform int rgbColorSpace;  \n
        const float e = 1.0e-10;
        const vec4 rgb2hsvK = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
        const vec4 hsv2rgbK = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        \n
        vec3 RGBToCC(vec4 rgba) {\n
            float Y = 0.299 * rgba.r + 0.587 * rgba.g + 0.114 * rgba.b;\n
            return vec3(Y, (rgba.b - Y) * 0.565, (rgba.r - Y) * 0.713);\n
        }\n
        vec3 YCCToRGB(vec3 ycc) {\n
            return vec3(
            ycc.b / 0.713 + ycc.r,
            ycc.r - 0.72177 * ycc.b - 0.3473 * ycc.g,
            ycc.g / 0.565 + ycc.r);\n
        }\n
        vec3 saturation(vec3 rgb, float adjustment) {
            const vec3 W = vec3(0.2125, 0.7154, 0.0721);
            vec3 intensity = vec3(dot(rgb, W));
            return mix(intensity, rgb, adjustment);
        }\n
        void main() {\n
            vec4 src1Color = texture(tex, tex_coord);\n
            vec3 ycc = RGBToCC(src1Color);\n
            float mask = texture(maskTex, tex_coord).r;\n

            float spill = smoothstep(spillRange.x, spillRange.y, distance(keyCC.xy, ycc.gb));\n
            vec3 newRgb = YCCToRGB(vec3(ycc.r, ycc.g * spill, ycc.b * spill));
            newRgb = bool(removeSpill) ? saturation(newRgb, satu) : src1Color.rgb;

            color = bool(showMask) && !bool(showHue) && !bool(showSat) ? vec4(mask) :
            bool(doKey) && !bool(showHue) && !bool(showSat) ? vec4(newRgb, 1.0 - mask) :
            bool(showHue) && !bool(showSat) ? vec4((ycc.g + 0.5) * 2.0) :
            bool(showSat) ? vec4((ycc.b + 0.5) * 2.0):
            src1Color;
            //color =  vec4(hsv.r) * 2.0;
    });

    frag         = m_glbase->shaderCollector().getShaderHeader() + frag;
    m_chromaShdr = m_glbase->shaderCollector().add("greenScreenChroma", vert, frag);
}

GLuint GLSLChromaKey::proc(GLuint tex) {
    if (!m_inited || !m_par) return 0;
    if (!m_chromaShdr) initChromaShdr();
    if (!m_chromaMaskShdr) initChromaMaskShdr();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_rgbFbo->bind();

    if (!(*m_par)()->freeze()) {
        m_rgbFbo->clear();

        // draw input image
        m_stdTex->begin();
        m_stdTex->setIdentMatrix4fv("m_pvm");
        m_stdTex->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        m_hFlipQuad->draw();
    }

    m_rgbFbo->unbind();

    if (m_pickMask2Color.x != 0.f && m_pickMask2Color.y != 0.f) {
        m_rgbFbo->download(&m_downBuf[0], GL_RGBA8, GL_RGBA);

        m_pickMask2Color.y = 1.f - m_pickMask2Color.y;

        m_pickMask2Color.x *= (float)(*m_par)()->width;
        m_pickMask2Color.y *= (float)(*m_par)()->height;

        uint8_t* bufPtr = &m_downBuf[(size_t)(m_pickMask2Color.y * (float)(*m_par)()->width + m_pickMask2Color.x) * 4];

        (*m_par)()->keyCol2_r = *bufPtr;
        (*m_par)()->keyCol2_g = *(++bufPtr);
        (*m_par)()->keyCol2_b = *(++bufPtr);

        if (m_keyCol2UpdtFunc) m_keyCol2UpdtFunc(1.f);

        m_pickMask2Color.x = 0.f;
        m_pickMask2Color.y = 0.f;
    }

    //------------------------------------------------------------------------------------------------------------------

    // start m_histogram for Activity calculation
    if ((*m_par)()->procHisto()) m_histo->proc(m_rgbFbo->getColorImg(0));

    //------------------------------------------------------------------------------------------------------------------

    // color mask
    m_maskFbo->bind();
    m_maskFbo->clear();

    m_chromaMaskShdr->begin();
    m_chromaMaskShdr->setIdentMatrix4fv("m_pvm");
    m_chromaMaskShdr->setUniform1i("tex", 0);
    m_chromaMaskShdr->setUniform2f("range", (*m_par)()->range_x(), (*m_par)()->range_y());
    m_chromaMaskShdr->setUniform2f("range2", (*m_par)()->range2_x(), (*m_par)()->range2_y());
    m_chromaMaskShdr->setUniform4fv("keyCC", &m_keyCC[0]);
    m_chromaMaskShdr->setUniform3fv("keyRGB", &m_keyColTmp[0]);
    m_chromaMaskShdr->setUniform3fv("keyRGB2", &m_keyColTmp2[0]);
    m_chromaMaskShdr->setUniform1i("doKey", (int)(*m_par)()->chromaKey());
    m_chromaMaskShdr->setUniform1i("actMask1", (int)(*m_par)()->actMask1());
    m_chromaMaskShdr->setUniform1i("actMask2", (int)(*m_par)()->actMask2());
    m_chromaMaskShdr->setUniform1i("rgbColorSpace", m_chromaInRGBSpace);
    m_chromaMaskShdr->setUniform1f("maskGamma", (*m_par)()->maskGamma());
    m_chromaMaskShdr->setUniform1f("maskGamma2", (*m_par)()->maskGamma2());
    m_chromaMaskShdr->setUniform2f("maskCut", (*m_par)()->maskCut_x(), (*m_par)()->maskCut_y());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_rgbFbo->getColorImg(0));
    m_quad->draw();

    m_maskFbo->unbind();

    //------------------------------------------------------------------------------------------------------------------

    // third pass blur
    m_fastBlurMem->setBright(1.f);
    m_fastBlurMem->setOffsScale((*m_par)()->maskBlurSize());
    m_fastBlurMem->proc(m_maskFbo->getColorImg());

    for (int i = 0; i < (int)(*m_par)()->blurIt(); i++) m_fastBlurMem->proc(m_fastBlurMem->getResult());

    //----------------------------------------------------------------------------------

    m_procFbo->bind();
    m_procFbo->clear();

    m_chromaShdr->begin();
    m_chromaShdr->setIdentMatrix4fv("m_pvm");
    m_chromaShdr->setUniform1i("tex", 0);
    m_chromaShdr->setUniform1i("maskTex", 1);
    m_chromaShdr->setUniform2f("spillRange", (*m_par)()->spillRange_x(), (*m_par)()->spillRange_y());
    m_chromaShdr->setUniform1i("showMask", (int)(*m_par)()->showMask());
    m_chromaShdr->setUniform1i("showHue", (int)(*m_par)()->showHue());
    m_chromaShdr->setUniform1i("showSat", (int)(*m_par)()->showSat());
    m_chromaShdr->setUniform1i("doKey", (int)(*m_par)()->chromaKey());
    m_chromaShdr->setUniform1i("removeSpill", (int)(*m_par)()->removeSpill());
    m_chromaShdr->setUniform1f("satu", (*m_par)()->satu());
    m_chromaShdr->setUniform4fv("keyCC", &m_keyCC[0]);
    m_chromaShdr->setUniform3fv("keyRGB", &m_keyColTmp[0]);
    m_chromaShdr->setUniform1i("rgbColorSpace", m_chromaInRGBSpace);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_rgbFbo->getColorImg(0));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_fastBlurMem->getResult());

    m_hFlipQuad->draw();

    m_procFbo->unbind();

    // readHistoVal();

    /*
    if (m_keyCol.r != 0.f && m_keyCol.g != 0.f && m_keyCol.y != 0.f)
    {
        if (!gotFirstm_histo){
            gotFirstm_histo = true;
            m_startTime = chrono::system_clock::now();
        } else {
            procm_histo = std::chrono::duration<double,
    std::milli>(chrono::system_clock::now() - m_startTime).count() < 10.0;
        }
    }*/

    return m_procFbo->getColorImg();

    // LOG << "maxi : " <<  glm::to_string(m_keyCol);
}

void GLSLChromaKey::pickMask2Color(glm::vec2& pos) { m_pickMask2Color = pos; }

void GLSLChromaKey::readHistoVal() {
    if (m_par && m_histo && (*m_par)()->procHisto()) {
        for (int i = 0; i < 3; i++) {
            if (m_keyCol[i])
                if (m_histo->getHistoPeakInd(i) != 0.f) {
                    if ((*m_keyCol[i])() == 0.f)
                        (*m_keyCol[i]) = m_histo->getHistoPeakInd(i) * 255.f;
                    else
                        (*m_keyCol[i]) = ((*m_keyCol[i])() * 30.f + (m_histo->getHistoPeakInd(i) * 255.f)) / 31.f;
                }
        }

        for (int i = 0; i < 3; i++)
            if (m_keyCol[i]) m_keyColTmp[i] = (*m_keyCol[i])() / 255.f;

        for (int i = 0; i < 3; i++)
            if (m_keyCol[i]) m_keyColTmp2[i] = (*m_keyCol2[i])() / 255.f;

        m_keyCC.x = RGBAToCC(m_keyColTmp).x;
        m_keyCC.y = RGBAToCC(m_keyColTmp).y;

        m_keyCC.z = RGBAToCC(m_keyColTmp2).x;
        m_keyCC.w = RGBAToCC(m_keyColTmp2).y;
    }
}

}  // namespace ara
