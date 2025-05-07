//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  GLSLHistogram.cpp
//
//	GPU Histogram Calculation with Geometry Shaders
//
//  IGNORES 0 - Values !!!! Performance optimization
//
//  Histogram, writes into a 1-dimensional FBO
//  ColorValues = x-Position in the FBO
//  For every color a 1.0 is written. Values are summed up and normalized if
//  needed
//
//	Fbo zum speicher der min/max Werte: minMaxSpectrFbo
//  Structure: [0] = min Value, [1] = max Value, [2] = min Index Histo, [3] =
//  max Index Histo,
//             [4] = sum histogram index * value (normalized) / histogramWidth
//             [5] leer
//			   [6] = chan2 min Wert, etc.
//
//  The width of the FBO corresponds in the best case to the maximum number of
//  possibles Values (e.g. RGB8 = 256) in case of Floats a number is being
//  defined and filtered linear
//
//  generally the values uploaded as textures will be normalized [0-1].
//  In case of consciously set different boundaries the maxValPerChan Parameter
//  has to be adjusted manually
//
//  After processing the histogram it's helpful to know the lowest and highest
//  Index E.g. in case of depth images. In this case it's helpful to have a
//  threshold to suppress noise, etc. therefore the parameter indValThres is
//  used
//
//  normally it's not necessary to sample all pixels of an image, therefore the
//  parameter downsample can be used to sample only every n-th pixel - will
//  drastically increase performance
//
//  in case of a realtime video input, values can jitter. This can be suppressed
//  via the smoothing value which is nothing more than a FBO feedback

#include "GLUtils/GLSLHistogram.h"

#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {

GLSLHistogram::GLSLHistogram(GLBase* glbase, ivec2 size, GLenum type, unsigned int downSample,
                             unsigned int histWidth, bool getBounds, bool normalize, float maxValPerChan)
    : m_glbase(glbase), m_shCol(&glbase->shaderCollector()), m_texType(type), m_geoAmp(32), m_width(size.x), m_height(size.y),
      m_maxHistoWidth(static_cast<int>(histWidth)), m_downSample(downSample), m_normalize(normalize), m_getBounds(getBounds),
      m_valThres(0.f), m_indValThres(0.05f), m_maxValPerChan(maxValPerChan) {
    m_quad = make_unique<Quad>(QuadInitParams{.color = {0.f, 0.f, 0.f, 0.f} });  // color will be replaced when rendering with blending on

    m_minMax[0] = 0;
    m_minMax[1] = 0;
    m_maxValSum = static_cast<float>(m_width) * static_cast<float>(m_height);

    // get nr channels
    if (type == GL_R8
#ifndef ARA_USE_GLES31
        || type == GL_R16
#endif
        || type == GL_R16F || type == GL_R32F) {
        m_nrChan    = 1;
        m_texFormat = GL_RED;
    } else if (type == GL_RG8
#ifndef ARA_USE_GLES31
               || type == GL_RG16
#endif
               || type == GL_RG16F || type == GL_RG32F) {
        m_nrChan    = 2;
        m_texFormat = GL_RG;
    } else if (type == GL_RGB8
#ifndef ARA_USE_GLES31
               || type == GL_RGB16
#endif
               || type == GL_RGB16F || type == GL_RGB32F) {
        m_nrChan    = 3;
        m_texFormat = GL_RGB;
    } else if (type == GL_RGBA8
#ifndef ARA_USE_GLES31
               || type == GL_RGBA16
#endif
               || type == GL_RGBA16F || type == GL_RGBA32F) {
        m_nrChan    = 4;
        m_texFormat = GL_RGBA;
    }

    // array to save the downloaded Texture of the SpectrFBO
    m_minMaxSpectr.resize(4 * m_nrChan);
    for (int i = 0; i < m_nrChan * 4; i++) {
        m_minMaxSpectr[i] = 0;
    }

    m_energyMed.resize(m_nrChan);
    for (int i = 0; i < m_nrChan; i++) {
        m_energyMed[i] = 0;
    }

    // get pixel m_format
    if (type == GL_R8 || type == GL_RG8 || type == GL_RGB8 || type == GL_RGBA8) {
        m_maxHistoWidth = std::min(m_maxHistoWidth, 256);
        m_maxValOfType  = 256.f;
        m_maximum       = 1.f;
        m_format        = GL_BYTE;
    }
#ifndef ARA_USE_GLES31
    else if (type == GL_R16 || type == GL_RG16 || type == GL_RGB16 || type == GL_RGBA16) {
        m_format       = GL_SHORT;
        m_maxValOfType = 65504.f;
    }
#endif
    else if (type == GL_R16F || type == GL_RG16F || type == GL_RGB16F || type == GL_RGBA16F) {
        m_format       = GL_FLOAT;
        m_maxValOfType = 65504.f;
        m_minMaxFormat = GL_R16F;
    } else if (type == GL_R32F || type == GL_RG32F || type == GL_RGB32F || type == GL_RGBA32F) {
        m_format       = GL_FLOAT;
        m_minMaxFormat = GL_R32F;
        m_maxValOfType = 10000000.f;
    }

    m_histoTexSize = glm::ivec2(m_maxHistoWidth, 1);

    m_histoDownload.resize(m_nrChan * m_histoTexSize.x);
    for (int i = 0; i < m_nrChan * m_histoTexSize.x; i++) {
        m_histoDownload[i] = 0;
    }

    // create a 1 dimensional FBO
    m_histoFbo = make_unique<PingPongFbo>(FboInitParams{m_glbase, m_histoTexSize.x, m_nrChan, 1, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1,
                                 GL_CLAMP_TO_BORDER, false});
    m_histoFbo->setMinFilter(GL_NEAREST);
    m_histoFbo->setMagFilter(GL_NEAREST);
    m_histoFbo->clear();

    m_minMaxFormat = GL_R32F;
    m_minMaxFbo    = make_unique<FBO>(FboInitParams{m_glbase, 2, m_nrChan, 1, m_minMaxFormat, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false});
    m_minMaxFbo->clear();

    // in case of non float images, the m_maximum Value is unique and the first
    // runthrough is not necessary upload the manual set min/max values
    if (m_format != GL_FLOAT) {
        glBindTexture(GL_TEXTURE_2D, m_minMaxFbo->getColorImg());
        glTexSubImage2D(GL_TEXTURE_2D,  // target
                        0,              // mipmap level
                        0, 0,           // xoffset, yoffset
                        2,              // width
                        m_nrChan,         // height
                        m_texFormat, m_format, &m_minMax[0]);
    }

    m_minMaxSpectrFbo = make_unique<FBO>(FboInitParams{m_glbase, 4, m_nrChan, 1, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false});
    m_minMaxSpectrFbo->setMinFilter(GL_NEAREST);
    m_minMaxSpectrFbo->setMagFilter(GL_NEAREST);
    m_minMaxSpectrFbo->clear();

    m_energyMedFbo = make_unique<FBO>(FboInitParams{m_glbase, 1, m_nrChan, 1, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false});
    m_energyMedFbo->setMinFilter(GL_NEAREST);
    m_energyMedFbo->setMagFilter(GL_NEAREST);
    m_energyMedFbo->clear();

    // read the m_maximum amplification the GeoShader can handle. But the m_maximum
    // value doesn't correspond to the m_maximum performance to be optimized
    // manually according to platform and conditions
    GLint glMaxGeoAmpPoints;
#ifndef ARA_USE_GLES31
    glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &glMaxGeoAmpPoints);
#else
    glMaxGeoAmpPoints = 1024;
#endif
    m_geoAmp = std::min(m_geoAmp * m_nrChan, glMaxGeoAmpPoints) / m_nrChan;

    // get cell size, later be handed over to the m_geoAmp * m_nrChan geoshader
    float divisor = 2.f;
    while (divisor < (static_cast<float>(m_geoAmp) / divisor)) {
        divisor += 2.f;
    }

    m_cellSize.x = static_cast<int>(divisor);
    m_cellSize.y = static_cast<int>(static_cast<float>(m_geoAmp) / divisor);
    m_nrCells.x  = m_width / m_cellSize.x;
    m_nrCells.y  = m_height / m_cellSize.y;
    m_totNrCells = m_nrCells.x * m_nrCells.y;

    // in pixels from 0|0 to width|height
    vector<GLfloat> initEmitPos(m_totNrCells * 4);
    for (int y = 0; y < m_nrCells.y; y++) {
        for (int x = 0; x < m_nrCells.x; x++) {
            initEmitPos[(y * m_nrCells.x + x) * 4]     = static_cast<float>(x * m_cellSize.x);
            initEmitPos[(y * m_nrCells.x + x) * 4 + 1] = static_cast<float>(y * m_cellSize.y);
            initEmitPos[(y * m_nrCells.x + x) * 4 + 2] = 0.f;
            initEmitPos[(y * m_nrCells.x + x) * 4 + 3] = 1.f;
        }
    }

    m_trigVao = make_unique<VAO>("position:4f", GL_DYNAMIC_DRAW);
    m_trigVao->initData(m_totNrCells, &initEmitPos[0]);

    //------------------------------------------------------------

    m_spectrGeoAmp = std::min(m_geoAmp, 3);  // lesser geo amp = faster, will be later
                                         // multiplied with m_nrChan and 4 (for every Value)
    m_spectrNrEmitTrig = m_histoTexSize.x / m_spectrGeoAmp;

    vector<GLfloat> initSpectrEmitPos(m_spectrNrEmitTrig * 4);
    for (int x = 0; x < m_spectrNrEmitTrig; x++) {
        initSpectrEmitPos[x * 4]     = x * static_cast<float>(m_spectrGeoAmp);
        initSpectrEmitPos[x * 4 + 1] = 0.f;
        initSpectrEmitPos[x * 4 + 2] = 0.f;
        initSpectrEmitPos[x * 4 + 3] = 1.f;
    }

    m_trigSpectrVao = make_unique<VAO>("position:4f", GL_DYNAMIC_DRAW);
    m_trigSpectrVao->initData(m_spectrNrEmitTrig, &initSpectrEmitPos[0]);

    // set the amount of geometry amplification in respect to the number of channels
    m_geoAmp *= m_nrChan;

    initShader();
    initNormShader();
    initMinMaxShader();
    initMinMaxSpectrShader();
    initEnergyMedShader();
}

void GLSLHistogram::proc(GLint texId) {
    if (m_format == GL_FLOAT) {
        getMinMax(texId);
    }

    getSpectrum(texId);

    // debug values
    if (m_getEnergySum) {
        glBindTexture(GL_TEXTURE_2D, m_histoFbo->m_src->getColorImg());
#ifdef ARA_USE_GLES31
        glesGetTexImage(m_histoFbo->m_src->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, m_histoFbo->getWidth(),
                        m_histoFbo->getHeight(), (GLubyte*)&m_histoDownload[0]);
#else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &m_histoDownload[0]);
#endif

        m_energySum = 0.f;
        for (int i = static_cast<int>(m_energySumLowThresh * static_cast<float>(m_histoTexSize.x)); i < m_histoTexSize.x; i++) {
            m_energySum += m_histoDownload[i];
        }
    }

    if (m_getBounds) {
        getHistoBounds();
    }

    if (m_normalize) {
       normalizeHisto();
    }

    m_procFrame++;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
}

void GLSLHistogram::getMinMax(GLuint texId) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_minMaxFbo->bind();
    m_minMaxFbo->clearToColor(-m_maxValOfType, -m_maxValOfType, -m_maxValOfType, 1.f);

    glBlendEquation(GL_MAX);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    m_minMaxShader->begin();
    m_minMaxShader->setUniform1i("tex", 0);
    m_minMaxShader->setUniform1i("cellInc", static_cast<int>(m_downSample));
    m_minMaxShader->setUniform2i("m_cellSize", m_cellSize.x, m_cellSize.y);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    m_trigVao->draw(GL_POINTS);

    glReadPixels(0, 0, 2, 1, GL_RED, GL_FLOAT, &m_minMax[0]);

    m_minMaxFbo->unbind();

    m_maximum = m_minMax[0];
    m_minimum = -m_minMax[1];

    glBlendEquation(GL_FUNC_ADD);
}

void GLSLHistogram::getSpectrum(GLuint texId) const {
    m_histoFbo->m_dst->bind();
    m_histoFbo->m_dst->clearAlpha(m_smoothing, 0.f);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    // values will be uploaded normalized if nothing else is defined....
    m_histoShader->begin();
    m_histoShader->setUniform1i("tex", 0);
    m_histoShader->setUniform1i("cellInc", static_cast<int>(m_downSample));
    m_histoShader->setUniform2i("m_cellSize", m_cellSize.x, m_cellSize.y);
    m_histoShader->setUniform2f("texSize", static_cast<float>(m_width), static_cast<float>(m_height));
    m_histoShader->setUniform1f("range", m_maxValPerChan);
    m_histoShader->setUniform1i("m_nrChan", m_nrChan);
    m_histoShader->setUniform1f("thres", m_valThres);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    m_trigVao->draw(GL_POINTS);

    m_histoFbo->m_dst->unbind();
    m_histoFbo->swap();
}

void GLSLHistogram::getHistoBounds() const {
    m_minMaxSpectrFbo->bind();
    m_minMaxSpectrFbo->clear();

    glBlendEquation(GL_MAX);
    glBlendFunc(GL_ONE, GL_ONE);

    m_minMaxSpectrShader->begin();
    m_minMaxSpectrShader->setUniform1i("tex", 0);
    m_minMaxSpectrShader->setUniform1i("m_cellSize", m_spectrGeoAmp);
    m_minMaxSpectrShader->setUniform1f("maxVal", m_maxValSum);
    m_minMaxSpectrShader->setUniform1f("maxIndex", static_cast<float>(m_histoTexSize.x) - 1);
    m_minMaxSpectrShader->setUniform1f("m_indValThres", m_indValThres);
    m_minMaxSpectrShader->setUniform1i("m_nrChan", m_nrChan);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_histoFbo->m_src->getColorImg());

    m_trigSpectrVao->draw(GL_POINTS);

    m_minMaxSpectrFbo->unbind();

    glBlendEquation(GL_FUNC_ADD);
}

void GLSLHistogram::normalizeHisto() {
    downloadMinMax();

    // ------ rewrite spectrum normalized ----------

    m_histoFbo->m_dst->bind();
    m_histoFbo->m_dst->clear();

    glBlendFunc(GL_ONE, GL_ZERO);

    m_normShader->begin();
    m_normShader->setIdentMatrix4fv("m_pvm");
    m_normShader->setUniform1i("m_minMax", 1);
    m_normShader->setUniform1i("histo", 0);
    m_normShader->setUniform1f("m_nrChan", static_cast<float>(m_nrChan));
    m_normShader->setUniform2f("histoSize", static_cast<float>(m_histoTexSize.x), static_cast<float>(m_nrChan));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_minMaxSpectrFbo->getColorImg());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_histoFbo->m_src->getColorImg());

    m_quad->draw();

    m_histoFbo->m_dst->unbind();
    m_histoFbo->swap();

    glEnable(GL_BLEND);

    if (m_getEnergyCenter) {
        m_energyMedFbo->bind();
        m_energyMedFbo->clear();

        glBlendEquation(GL_MAX);
        glBlendFunc(GL_ONE, GL_ONE);

        m_energyMedShader->begin();
        m_energyMedShader->setUniform1i("tex", 0);
        m_energyMedShader->setUniform1i("m_cellSize", m_spectrGeoAmp);
        m_energyMedShader->setUniform1i("m_nrChan", m_nrChan);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_histoFbo->m_src->getColorImg());

        m_trigSpectrVao->draw(GL_POINTS);

        m_energyMedFbo->unbind();
        ++m_procEnerFrame;
    }
}

void GLSLHistogram::downloadMinMax() {
    if (m_lastDownloadFrame != m_procFrame) {
        glBindTexture(GL_TEXTURE_2D, m_minMaxSpectrFbo->getColorImg());
#ifdef ARA_USE_GLES31
        glesGetTexImage(m_minMaxSpectrFbo->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, m_minMaxSpectrFbo->getWidth(),
                        m_minMaxSpectrFbo->getHeight(), (GLubyte*)&m_minMaxSpectr[0]);
#else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &m_minMaxSpectr[0]);
#endif

        m_lastDownloadFrame = m_procFrame;
    }
}

void GLSLHistogram::downloadEnergyCenter() {
    if (m_lastEnerDownloadFrame != m_procEnerFrame) {
        glBindTexture(GL_TEXTURE_2D, m_energyMedFbo->getColorImg());
#ifdef ARA_USE_GLES31
        glesGetTexImage(m_energyMedFbo->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, m_energyMedFbo->getWidth(),
                        m_energyMedFbo->getHeight(), (GLubyte*)&m_energyMed[0]);
#else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, m_energyMed.data());
#endif
        m_lastEnerDownloadFrame = m_procEnerFrame;
    }
}

void GLSLHistogram::initShader() {
    std::string shdr_Header = "#version 410 core\n";

    std::string stdVert = STRINGIFY(layout(location = 0) in vec4 position; void main() {
        gl_PointSize = 0.1;
        gl_Position  = position;
    });
    stdVert             = "// GLSLHistogram vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in;

                                 uniform sampler2D tex; uniform ivec2 cellSize; uniform int cellInc; uniform int nrChan;
                                 uniform float range; uniform float thres;

                                 ivec2 texCoord; vec4 col; float lum;

                                 void main() {
                                     for (int y = 0; y < m_cellSize.y; y += cellInc) {
                                         for (int x = 0; x < m_cellSize.x; x += cellInc) {
                                             texCoord = ivec2(gl_in[0].gl_Position.x + x, gl_in[0].gl_Position.y + y);
                                             col      = texelFetch(tex, texCoord, 0);
                                             lum      = (col.r + col.g + col.b);
                                             if (lum > thres) {
                                                 // normalize color to be written to fit into the normal square
                                                 col = (col - (range * 0.5)) * 2.0;
                                                 for (int i = 0; i < m_nrChan; i++) {
                                                     gl_Position =
                                                         vec4(col[i], float(2 * i + 1) / float(m_nrChan) - 1.0, 0.0, 1.0);
                                                     EmitVertex();
                                                     EndPrimitive();
                                                 }
                                             }
                                         }
                                     }
                                 });
    geom = "// GLSLHistogram geom shader\n" + shdr_Header + "layout (points, max_vertices = " + std::to_string(m_geoAmp) +
           ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });
    frag             = "// GLSLHistogram Update Shader\n" + shdr_Header + frag;

    m_histoShader = m_shCol->add("glsl_histogram", stdVert, geom, frag);
}

void GLSLHistogram::initNormShader() {
    std::string shdr_Header = "#version 410 core\n";

    std::string stdVert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 2) in vec2 texCoord;
                                    out vec2 tex_coord; void main() {
                                        tex_coord   = texCoord;
                                        gl_Position = position;
                                    });
    stdVert             = "// GLSLHistogram Normalization shader\n" + shdr_Header + stdVert;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; uniform sampler2D histo; uniform sampler2D minMax;
                                 uniform float nrChan; uniform vec2 histoSize; in vec2 tex_coord; void main() {
                                     ivec2 texC     = ivec2(tex_coord * histoSize);
                                     float histoVal = texelFetch(histo, texC, 0).r;
                                     float max      = texelFetch(m_minMax, ivec2(0, texC.y), 0).r;
                                     color          = vec4(histoVal / max, 0.0, 0.0, 1.0);
                                 });
    frag             = "// GLSLHistogram Normalization Shader\n" + shdr_Header + frag;

    m_normShader = m_shCol->add("glslHisto_norm", stdVert, frag);
}

void GLSLHistogram::initMinMaxShader() {
    std::string shdr_Header = "#version 410 core\n";

    std::string stdVert = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });
    stdVert             = "// GLSLHistogram m_minMax vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in;
        uniform sampler2D tex;
        uniform ivec2 cellSize;
        uniform int cellInc;
        ivec2 texCoord;
        vec4 col;
        float lum;
        out float pixLum;

         void main() {
             for (int y = 0; y < m_cellSize.y; y += cellInc) {
                 for (int x = 0; x < m_cellSize.x; x += cellInc) {
                     texCoord = ivec2(gl_in[0].gl_Position.x + x, gl_in[0].gl_Position.y + y);
                     col      = texelFetch(tex, texCoord, 0);
                     lum      = (col.r + col.g + col.b);

                     if (lum > 0) {
                         // max
                         pixLum      = lum;
                         gl_Position = vec4(-0.75, 0.0, 0.0, 1.0);
                         EmitVertex();
                         EndPrimitive();

                         // min
                         pixLum      = -lum;
                         gl_Position = vec4(0.75, 0.0, 0.0, 1.0);
                         EmitVertex();
                         EndPrimitive();
                     }
                 }
             }
         });
    geom = "// GLSLHistogram m_minMax geom shader\n" + shdr_Header +
                       "layout (points, max_vertices = " + std::to_string(m_geoAmp) + ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; in float pixLum;
                                 void main() { color = vec4(pixLum, pixLum, pixLum, 1.0); });
    frag             = "// GLSLHistogram Update Shader\n" + shdr_Header + frag;

    m_minMaxShader = m_shCol->add("GLSLHistogram_MinMax", stdVert, geom, frag);
}

void GLSLHistogram::initMinMaxSpectrShader() {
    std::string shdr_Header = "#version 410 core\n";
    std::string stdVert     = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });
    stdVert                 = "// GLSLHistogram m_minMax Spectr  vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in; uniform sampler2D tex; uniform int cellSize; uniform int nrChan;
                                 uniform float maxVal; uniform float maxIndex; uniform float indValThres; int texCoord;
                                 float val; float lum; out float pixLum;

                                 void main() {
                                     for (int i = 0; i < m_nrChan; i++) {
                                         for (int x = 0; x < m_cellSize; x++) {
                                             texCoord   = int(gl_in[0].gl_Position.x) + x;
                                             val        = texelFetch(tex, ivec2(texCoord, i), 0).r;
                                             float yPos = float(2 * i + 1) / float(m_nrChan) - 1.0;

                                             // max [0]
                                             pixLum      = val;
                                             gl_Position = vec4(-0.75, yPos, 0.0, 1.0);
                                             EmitVertex();
                                             EndPrimitive();

                                             // min [1]
                                             pixLum      = maxVal - pixLum;
                                             gl_Position = vec4(-0.25, yPos, 0.0, 1.0);
                                             EmitVertex();
                                             EndPrimitive();

                                             if (val > m_indValThres) {
                                                 // indexMax [2]
                                                 pixLum      = float(texCoord);
                                                 gl_Position = vec4(0.25, yPos, 0.0, 1.0);
                                                 EmitVertex();
                                                 EndPrimitive();

                                                 // indexMin [3]
                                                 pixLum      = maxIndex - float(texCoord);
                                                 gl_Position = vec4(0.75, yPos, 0.0, 1.0);
                                                 EmitVertex();
                                                 EndPrimitive();
                                             }
                                         }
                                     }
                                 });
    geom             = "// GLSLHistogram m_minMax Spectr geom shader\n" + shdr_Header +
                       "layout (points, max_vertices = " + std::to_string(m_spectrGeoAmp * m_nrChan * 4) + ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; in float pixLum;
                                 void main() { color = vec4(pixLum, pixLum, pixLum, 1.0); });
    frag             = "// GLSLHistogram m_minMax Spectr frag Shader\n" + shdr_Header + frag;

    m_minMaxSpectrShader = m_shCol->add("GLSLHistogram_MinMaxSpectr", stdVert, geom, frag);
}

void GLSLHistogram::initEnergyMedShader() {
    std::string shdr_Header = "#version 410 core\n";
    std::string stdVert     = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });

    stdVert = "// GLSLHistogram energy Med vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in; uniform sampler2D tex; uniform int cellSize; uniform int nrChan;
                                 int texCoord; float val; out float pixLum;

                                 void main() {
                                     for (int i = 0; i < m_nrChan; i++) {
                                         for (int x = 0; x < m_cellSize; x++) {
                                             texCoord   = int(gl_in[0].gl_Position.x) + x;
                                             val        = texelFetch(tex, ivec2(texCoord, i), 0).r;
                                             float yPos = float(2 * i + 1) / float(m_nrChan) - 1.0;

                                             // sum ind * val [4]
                                             if (val == 1.0) {
                                                 pixLum      = texCoord;
                                                 gl_Position = vec4(0.0, yPos, 0.0, 1.0);
                                                 EmitVertex();
                                                 EndPrimitive();
                                             }
                                         }
                                     }
                                 });

    geom = "// GLSLHistogram energy med geom shader\n" + shdr_Header +
           "layout (points, max_vertices = " + std::to_string(m_spectrGeoAmp * m_nrChan) + ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; in float pixLum;
                                 void main() { color = vec4(pixLum, 0.0, 0.0, 1.0); });

    frag = "// GLSLHistogram energy med frag Shader\n" + shdr_Header + frag;

    m_energyMedShader = m_shCol->add("GLSLHistogram_EnergyMedr", stdVert, geom, frag);
}

}  // namespace ara
