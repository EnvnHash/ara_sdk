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

GLSLHistogram::GLSLHistogram(GLBase* glbase, int _width, int _height, GLenum _type, unsigned int _downSample,
                             unsigned int _histWidth, bool _getBounds, bool _normalize, float _maxValPerChan)
    : m_glbase(glbase), shCol(&glbase->shaderCollector()), texType(_type), geoAmp(32), width(_width), height(_height),
      maxHistoWidth(static_cast<int>(_histWidth)), downSample(_downSample), normalize(_normalize), getBounds(_getBounds),
      valThres(0.f), indValThres(0.05f), maxValPerChan(_maxValPerChan) {
    quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f,
                    0.f);  // color will be replaced when rendering with blending on

    minMax    = new GLfloat[2 * 4];
    minMax[0] = 0;
    minMax[1] = 0;
    maxValSum = static_cast<float>(width) * static_cast<float>(height);

    // get nr channels
    if (_type == GL_R8
#ifndef ARA_USE_GLES31
        || _type == GL_R16
#endif
        || _type == GL_R16F || _type == GL_R32F) {
        nrChan    = 1;
        texFormat = GL_RED;
    } else if (_type == GL_RG8
#ifndef ARA_USE_GLES31
               || _type == GL_RG16
#endif
               || _type == GL_RG16F || _type == GL_RG32F) {
        nrChan    = 2;
        texFormat = GL_RG;
    } else if (_type == GL_RGB8
#ifndef ARA_USE_GLES31
               || _type == GL_RGB16
#endif
               || _type == GL_RGB16F || _type == GL_RGB32F) {
        nrChan    = 3;
        texFormat = GL_RGB;
    } else if (_type == GL_RGBA8
#ifndef ARA_USE_GLES31
               || _type == GL_RGBA16
#endif
               || _type == GL_RGBA16F || _type == GL_RGBA32F) {
        nrChan    = 4;
        texFormat = GL_RGBA;
    }

    // array to save the downloaded Texture of the SpectrFBO
    minMaxSpectr = new GLfloat[4 * nrChan];
    for (int i = 0; i < nrChan * 4; i++) {
        minMaxSpectr[i] = 0;
    }

    energyMed = new GLfloat[nrChan];
    for (int i = 0; i < nrChan; i++) {
        energyMed[i] = 0;
    }

    // get pixel m_format
    if (_type == GL_R8 || _type == GL_RG8 || _type == GL_RGB8 || _type == GL_RGBA8) {
        maxHistoWidth = std::min(maxHistoWidth, 256);
        maxValOfType  = 256.f;
        maximum       = 1.f;
        format        = GL_BYTE;
    }
#ifndef ARA_USE_GLES31
    else if (_type == GL_R16 || _type == GL_RG16 || _type == GL_RGB16 || _type == GL_RGBA16) {
        format       = GL_SHORT;
        maxValOfType = 65504.f;
    }
#endif
    else if (_type == GL_R16F || _type == GL_RG16F || _type == GL_RGB16F || _type == GL_RGBA16F) {
        format       = GL_FLOAT;
        maxValOfType = 65504.f;
        minMaxFormat = GL_R16F;
    } else if (_type == GL_R32F || _type == GL_RG32F || _type == GL_RGB32F || _type == GL_RGBA32F) {
        format       = GL_FLOAT;
        minMaxFormat = GL_R32F;
        maxValOfType = 10000000.f;
    }

    histoTexSize = glm::ivec2(maxHistoWidth, 1);

    histoDownload = new GLfloat[nrChan * histoTexSize.x];
    for (int i = 0; i < nrChan * histoTexSize.x; i++) histoDownload[i] = 0;

    // create a 1 dimensional FBO
    histoFbo = new PingPongFbo(m_glbase, histoTexSize.x, nrChan, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1,
                               GL_CLAMP_TO_BORDER, false);
    histoFbo->setMinFilter(GL_NEAREST);
    histoFbo->setMagFilter(GL_NEAREST);
    histoFbo->clear();

    minMaxFormat = GL_R32F;
    minMaxFbo    = new FBO(m_glbase, 2, nrChan, minMaxFormat, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
    minMaxFbo->clear();

    // in case of non float images, the maximum Value is unique and the first
    // runthrough is not necessary upload the manual set min/max values
    if (format != GL_FLOAT) {
        glBindTexture(GL_TEXTURE_2D, minMaxFbo->getColorImg());
        glTexSubImage2D(GL_TEXTURE_2D,  // target
                        0,              // mipmap level
                        0, 0,           // xoffset, yoffset
                        2,              // width
                        nrChan,         // height
                        texFormat, format, &minMax[0]);
    }

    minMaxSpectrFbo = new FBO(m_glbase, 4, nrChan, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
    minMaxSpectrFbo->setMinFilter(GL_NEAREST);
    minMaxSpectrFbo->setMagFilter(GL_NEAREST);
    minMaxSpectrFbo->clear();

    energyMedFbo = new FBO(m_glbase, 1, nrChan, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
    energyMedFbo->setMinFilter(GL_NEAREST);
    energyMedFbo->setMagFilter(GL_NEAREST);
    energyMedFbo->clear();

    // read the maximum amplification the GeoShader can handle. But the maximum
    // value doesn't correspond to the maximum performance to be optimized
    // manually according to platform and conditions
    GLint glMaxGeoAmpPoints;
#ifndef ARA_USE_GLES31
    glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &glMaxGeoAmpPoints);
#else
    glMaxGeoAmpPoints = 1024;
#endif
    geoAmp = std::min(geoAmp * nrChan, glMaxGeoAmpPoints) / nrChan;

    // get cell size, later be handed over to the geoAmp * nrChan geoshader
    float divisor = 2.f;
    while (divisor < (static_cast<float>(geoAmp) / divisor)) divisor += 2.f;

    cellSize.x = static_cast<int>(divisor);
    cellSize.y = static_cast<int>(static_cast<float>(geoAmp) / divisor);
    nrCells.x  = width / cellSize.x;
    nrCells.y  = height / cellSize.y;
    totNrCells = nrCells.x * nrCells.y;

    // in pixels from 0|0 to width|height
    vector<GLfloat> initEmitPos(totNrCells * 4);
    for (int y = 0; y < nrCells.y; y++) {
        for (int x = 0; x < nrCells.x; x++) {
            initEmitPos[(y * nrCells.x + x) * 4]     = static_cast<float>(x * cellSize.x);
            initEmitPos[(y * nrCells.x + x) * 4 + 1] = static_cast<float>(y * cellSize.y);
            initEmitPos[(y * nrCells.x + x) * 4 + 2] = 0.f;
            initEmitPos[(y * nrCells.x + x) * 4 + 3] = 1.f;
        }
    }

    trigVao = new VAO("position:4f", GL_DYNAMIC_DRAW);
    trigVao->initData(totNrCells, &initEmitPos[0]);

    //------------------------------------------------------------

    spectrGeoAmp = std::min(geoAmp, 3);  // lesser geo amp = faster, will be later
                                         // multiplied with nrChan and 4 (for every Value)
    spectrNrEmitTrig = histoTexSize.x / spectrGeoAmp;

    vector<GLfloat> initSpectrEmitPos(spectrNrEmitTrig * 4);
    for (int x = 0; x < spectrNrEmitTrig; x++) {
        initSpectrEmitPos[x * 4]     = x * static_cast<float>(spectrGeoAmp);
        initSpectrEmitPos[x * 4 + 1] = 0.f;
        initSpectrEmitPos[x * 4 + 2] = 0.f;
        initSpectrEmitPos[x * 4 + 3] = 1.f;
    }

    trigSpectrVao = new VAO("position:4f", GL_DYNAMIC_DRAW);
    trigSpectrVao->initData(spectrNrEmitTrig, &initSpectrEmitPos[0]);

    // set the number of geometry amplification in respect to the number of
    // channels
    geoAmp *= nrChan;

    initShader();
    initNormShader();
    initMinMaxShader();
    initMinMaxSpectrShader();
    initEnergyMedShader();
}

//---------------------------------------------------------

void GLSLHistogram::procMinMax(GLint texId) {
    /*
     glEnable(GL_BLEND);
     glDisable(GL_DEPTH_TEST);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

     // get min and max

     // clear FBO with negative values in order to get the minimum values, which
     come
     // in the negative range from the shader
     minMaxFbo->bind();
     minMaxFbo->clearToColor(-maxVal, -maxVal, -maxVal, 1.f);

     glBlendEquation(GL_MAX);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE);

     minMaxShader->begin();
     minMaxShader->setUniform1i("tex", 0);
     minMaxShader->setUniform1i("cellInc", downSample);
     minMaxShader->setUniform2i("cellSize", cellSize.x, cellSize.y);

     glActiveTexture(GL_TEXTURE0);
     glBindTexture(GL_TEXTURE_2D, texId);

     trigVao->draw(GL_POINTS);

     glReadPixels(0, 0, 2, 1, GL_RED, GL_FLOAT, &minMax[0]);

     minMaxFbo->unbind();

     maximum = minMax[0];
     minimum = -minMax[1];

     //    printf("max: %f , min: %f \n", maximum, minimum);

     glBlendEquation(GL_FUNC_ADD);
     */
}

void GLSLHistogram::proc(GLint texId) {
    if (format == GL_FLOAT) {
        getMinMax(texId);
    }

    getSpectrum(texId);

    // debug values
    if (b_getEnergySum) {
        glBindTexture(GL_TEXTURE_2D, histoFbo->src->getColorImg());
#ifdef ARA_USE_GLES31
        glesGetTexImage(histoFbo->src->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, histoFbo->getWidth(),
                        histoFbo->getHeight(), (GLubyte*)&histoDownload[0]);
#else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &histoDownload[0]);
#endif

        energySum = 0.f;
        for (int i = static_cast<int>(energySumLowThresh * static_cast<float>(histoTexSize.x)); i < histoTexSize.x; i++) {
            energySum += histoDownload[i];
        }
    }

    if (getBounds) {
        getHistoBounds();
    }

    if (normalize) {
       normalizeHisto();
    }

    procFrame++;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
}

void GLSLHistogram::getMinMax(GLuint texId) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    minMaxFbo->bind();
    minMaxFbo->clearToColor(-maxValOfType, -maxValOfType, -maxValOfType, 1.f);

    glBlendEquation(GL_MAX);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    minMaxShader->begin();
    minMaxShader->setUniform1i("tex", 0);
    minMaxShader->setUniform1i("cellInc", static_cast<int>(downSample));
    minMaxShader->setUniform2i("cellSize", cellSize.x, cellSize.y);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    trigVao->draw(GL_POINTS);

    glReadPixels(0, 0, 2, 1, GL_RED, GL_FLOAT, &minMax[0]);

    minMaxFbo->unbind();

    maximum = minMax[0];
    minimum = -minMax[1];

    glBlendEquation(GL_FUNC_ADD);
}

void GLSLHistogram::getSpectrum(GLuint texId) const {
    histoFbo->dst->bind();
    histoFbo->dst->clearAlpha(smoothing, 0.f);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    // values will be uploaded normalized if nothing else is defined....
    histoShader->begin();
    histoShader->setUniform1i("tex", 0);
    histoShader->setUniform1i("cellInc", static_cast<int>(downSample));
    histoShader->setUniform2i("cellSize", cellSize.x, cellSize.y);
    histoShader->setUniform2f("texSize", static_cast<float>(width), static_cast<float>(height));
    histoShader->setUniform1f("range", maxValPerChan);
    histoShader->setUniform1i("nrChan", nrChan);
    histoShader->setUniform1f("thres", valThres);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);

    trigVao->draw(GL_POINTS);

    histoFbo->dst->unbind();
    histoFbo->swap();
}

void GLSLHistogram::getHistoBounds() const {
    minMaxSpectrFbo->bind();
    minMaxSpectrFbo->clear();

    glBlendEquation(GL_MAX);
    glBlendFunc(GL_ONE, GL_ONE);

    minMaxSpectrShader->begin();
    minMaxSpectrShader->setUniform1i("tex", 0);
    minMaxSpectrShader->setUniform1i("cellSize", spectrGeoAmp);
    minMaxSpectrShader->setUniform1f("maxVal", maxValSum);
    minMaxSpectrShader->setUniform1f("maxIndex", static_cast<float>(histoTexSize.x) - 1);
    minMaxSpectrShader->setUniform1f("indValThres", indValThres);
    minMaxSpectrShader->setUniform1i("nrChan", nrChan);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, histoFbo->src->getColorImg());

    trigSpectrVao->draw(GL_POINTS);

    minMaxSpectrFbo->unbind();

    glBlendEquation(GL_FUNC_ADD);
}

void GLSLHistogram::normalizeHisto() {
    downloadMinMax();

    // ------ rewrite spectrum normalized ----------

    histoFbo->dst->bind();
    histoFbo->dst->clear();

    glBlendFunc(GL_ONE, GL_ZERO);

    normShader->begin();
    normShader->setIdentMatrix4fv("m_pvm");
    normShader->setUniform1i("minMax", 1);
    normShader->setUniform1i("histo", 0);
    normShader->setUniform1f("nrChan", static_cast<float>(nrChan));
    normShader->setUniform2f("histoSize", static_cast<float>(histoTexSize.x), static_cast<float>(nrChan));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, minMaxSpectrFbo->getColorImg());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, histoFbo->src->getColorImg());

    quad->draw();

    histoFbo->dst->unbind();
    histoFbo->swap();

    glEnable(GL_BLEND);

    if (getEnergyCenter) {
        energyMedFbo->bind();
        energyMedFbo->clear();

        glBlendEquation(GL_MAX);
        glBlendFunc(GL_ONE, GL_ONE);

        energyMedShader->begin();
        energyMedShader->setUniform1i("tex", 0);
        energyMedShader->setUniform1i("cellSize", spectrGeoAmp);
        energyMedShader->setUniform1i("nrChan", nrChan);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, histoFbo->src->getColorImg());

        trigSpectrVao->draw(GL_POINTS);

        energyMedFbo->unbind();
        ++procEnerFrame;
    }
}

void GLSLHistogram::downloadMinMax() {
    if (lastDownloadFrame != procFrame) {
        glBindTexture(GL_TEXTURE_2D, minMaxSpectrFbo->getColorImg());
#ifdef ARA_USE_GLES31
        glesGetTexImage(minMaxSpectrFbo->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, minMaxSpectrFbo->getWidth(),
                        minMaxSpectrFbo->getHeight(), (GLubyte*)&minMaxSpectr[0]);
#else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &minMaxSpectr[0]);
#endif

        lastDownloadFrame = procFrame;
    }
}

void GLSLHistogram::downloadEnergyCenter() {
    if (energyMed && lastEnerDownloadFrame != procEnerFrame) {
        glBindTexture(GL_TEXTURE_2D, energyMedFbo->getColorImg());
#ifdef ARA_USE_GLES31
        glesGetTexImage(energyMedFbo->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, energyMedFbo->getWidth(),
                        energyMedFbo->getHeight(), (GLubyte*)&energyMed[0]);
#else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &energyMed[0]);
#endif
        lastEnerDownloadFrame = procEnerFrame;
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
                                     for (int y = 0; y < cellSize.y; y += cellInc) {
                                         for (int x = 0; x < cellSize.x; x += cellInc) {
                                             texCoord = ivec2(gl_in[0].gl_Position.x + x, gl_in[0].gl_Position.y + y);
                                             col      = texelFetch(tex, texCoord, 0);
                                             lum      = (col.r + col.g + col.b);
                                             if (lum > thres) {
                                                 // normalize color to be written to fit into the normal
                                                 // square
                                                 col = (col - (range * 0.5)) * 2.0;
                                                 for (int i = 0; i < nrChan; i++) {
                                                     gl_Position =
                                                         vec4(col[i], float(2 * i + 1) / float(nrChan) - 1.0, 0.0, 1.0);
                                                     EmitVertex();
                                                     EndPrimitive();
                                                 }
                                             }
                                         }
                                     }
                                 });
    geom = "// GLSLHistogram geom shader\n" + shdr_Header + "layout (points, max_vertices = " + std::to_string(geoAmp) +
           ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });
    frag             = "// GLSLHistogram Update Shader\n" + shdr_Header + frag;

    histoShader = shCol->add("glsl_histogram", stdVert, geom, frag);
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
                                     float max      = texelFetch(minMax, ivec2(0, texC.y), 0).r;
                                     color          = vec4(histoVal / max, 0.0, 0.0, 1.0);
                                 });
    frag             = "// GLSLHistogram Normalization Shader\n" + shdr_Header + frag;

    normShader = shCol->add("glslHisto_norm", stdVert, frag);
}

void GLSLHistogram::initMinMaxShader() {
    std::string shdr_Header = "#version 410 core\n";

    std::string stdVert = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });
    stdVert             = "// GLSLHistogram minMax vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in; uniform sampler2D tex; uniform ivec2 cellSize; uniform int cellInc;
                                 ivec2 texCoord; vec4 col; float lum; out float pixLum;

                                 void main() {
                                     for (int y = 0; y < cellSize.y; y += cellInc) {
                                         for (int x = 0; x < cellSize.x; x += cellInc) {
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
    geom             = "// GLSLHistogram minMax geom shader\n" + shdr_Header +
           "layout (points, max_vertices = " + std::to_string(geoAmp) + ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; in float pixLum;
                                 void main() { color = vec4(pixLum, pixLum, pixLum, 1.0); });
    frag             = "// GLSLHistogram Update Shader\n" + shdr_Header + frag;

    minMaxShader = shCol->add("GLSLHistogram_MinMax", stdVert, geom, frag);
}

void GLSLHistogram::initMinMaxSpectrShader() {
    std::string shdr_Header = "#version 410 core\n";
    std::string stdVert     = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });
    stdVert                 = "// GLSLHistogram minMax Spectr  vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in; uniform sampler2D tex; uniform int cellSize; uniform int nrChan;
                                 uniform float maxVal; uniform float maxIndex; uniform float indValThres; int texCoord;
                                 float val; float lum; out float pixLum;

                                 void main() {
                                     for (int i = 0; i < nrChan; i++) {
                                         for (int x = 0; x < cellSize; x++) {
                                             texCoord   = int(gl_in[0].gl_Position.x) + x;
                                             val        = texelFetch(tex, ivec2(texCoord, i), 0).r;
                                             float yPos = float(2 * i + 1) / float(nrChan) - 1.0;

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

                                             if (val > indValThres) {
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
    geom             = "// GLSLHistogram minMax Spectr geom shader\n" + shdr_Header +
           "layout (points, max_vertices = " + std::to_string(spectrGeoAmp * nrChan * 4) + ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; in float pixLum;
                                 void main() { color = vec4(pixLum, pixLum, pixLum, 1.0); });
    frag             = "// GLSLHistogram minMax Spectr frag Shader\n" + shdr_Header + frag;

    minMaxSpectrShader = shCol->add("GLSLHistogram_MinMaxSpectr", stdVert, geom, frag);
}

void GLSLHistogram::initEnergyMedShader() {
    std::string shdr_Header = "#version 410 core\n";
    std::string stdVert     = STRINGIFY(layout(location = 0) in vec4 position; void main() { gl_Position = position; });

    stdVert = "// GLSLHistogram energy Med vertex shader\n" + shdr_Header + stdVert;

    std::string geom = STRINGIFY(layout(points) in; uniform sampler2D tex; uniform int cellSize; uniform int nrChan;
                                 int texCoord; float val; out float pixLum;

                                 void main() {
                                     for (int i = 0; i < nrChan; i++) {
                                         for (int x = 0; x < cellSize; x++) {
                                             texCoord   = int(gl_in[0].gl_Position.x) + x;
                                             val        = texelFetch(tex, ivec2(texCoord, i), 0).r;
                                             float yPos = float(2 * i + 1) / float(nrChan) - 1.0;

                                             // summe ind * val [4]
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
           "layout (points, max_vertices = " + std::to_string(spectrGeoAmp * nrChan) + ") out;\n" + geom;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 color; in float pixLum;
                                 void main() { color = vec4(pixLum, 0.0, 0.0, 1.0); });

    frag = "// GLSLHistogram energy med frag Shader\n" + shdr_Header + frag;

    energyMedShader = shCol->add("GLSLHistogram_EnergyMedr", stdVert, geom, frag);
}

GLSLHistogram::~GLSLHistogram() {
    delete quad;
    delete histoFbo;
}

}  // namespace ara
