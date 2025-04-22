//
//  GLSLHistogram.hpp
//  tav_core
//
//  Created by Sven Hahne on 08/06/16.
//  Copyright © 2016 Sven Hahne. All rights reserved..
//

#pragma once

#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/FBO.h>
#include <Utils/PingPongFbo.h>

namespace ara {
class GLSLHistogram {
public:
    GLSLHistogram(GLBase* glbase, int width, int height, GLenum type, unsigned int downSample,
                  unsigned int histWidth = 512, bool getBounds = true, bool normalize = true,
                  float maxValPerChan = 1.f);
    ~GLSLHistogram();

    static void procMinMax(GLint texId);
    void proc(GLint texId);
    void getMinMax(GLuint texId);
    void getSpectrum(GLuint texId) const;
    void getHistoBounds() const;
    void normalizeHisto();
    void downloadMinMax();
    void downloadEnergyCenter();

    float getMaximum(unsigned int chan = 0) {
        downloadMinMax();
        return minMaxSpectr[chan * 4];
    }
    float getMinimum(unsigned int chan = 0) {
        downloadMinMax();
        return maxValSum - minMaxSpectr[chan * 4 + 1];
    }
    float getMaxInd(unsigned int chan = 0) {
        downloadMinMax();
        return minMaxSpectr[chan * 4 + 2] / static_cast<float>(histoTexSize.x);
    }
    float getMinInd(unsigned int chan = 0) {
        downloadMinMax();
        return (static_cast<float>(histoTexSize.x - 1) - minMaxSpectr[chan * 4 + 3]) / static_cast<float>(histoTexSize.x);
    }
    float getHistoPeakInd(unsigned int chan = 0) {
        getEnergyCenter = true;
        downloadEnergyCenter();
        return energyMed[chan] / static_cast<float>(histoTexSize.x);
    }
    float getEnergySum(float lowThresh) {
        energySumLowThresh = lowThresh;
        b_getEnergySum     = true;
        return energySum;
    }
    [[nodiscard]] GLint getResult() const { return histoFbo->src->getColorImg(); }
    [[nodiscard]] GLint getMinMaxTex() const { return minMaxFbo->getColorImg(); }
    [[nodiscard]] float getSubtrMax() const { return maxValOfType; }
    void  setSmoothing(float val) { smoothing = val; }
    void  setValThres(float val) { valThres = val; }
    void  setIndValThres(float val) { indValThres = val; }

    void initShader();
    void initNormShader();
    void initMinMaxShader();
    void initMinMaxSpectrShader();
    void initEnergyMedShader();

private:
    GLBase*          m_glbase           = nullptr;
    ShaderCollector* shCol              = nullptr;
    Shaders*         histoShader        = nullptr;
    Shaders*         minMaxShader       = nullptr;
    Shaders*         minMaxSpectrShader = nullptr;
    Shaders*         energyMedShader    = nullptr;
    Shaders*         normShader         = nullptr;

    FBO*         minMaxFbo       = nullptr;
    FBO*         minMaxSpectrFbo = nullptr;
    FBO*         energyMedFbo    = nullptr;
    PingPongFbo* histoFbo        = nullptr;

    VAO*  trigVao       = nullptr;
    VAO*  trigSpectrVao = nullptr;
    Quad* quad          = nullptr;

    GLenum minMaxFormat;
    GLenum texType;
    GLenum format;
    GLenum texFormat;

    GLint    geoAmp;
    GLfloat* minMax        = nullptr;
    GLfloat* minMaxSpectr  = nullptr;
    GLfloat* energyMed     = nullptr;
    GLfloat* histoDownload = nullptr;

    int          width                 = 0;
    int          height                = 0;
    int          totNrCells            = 0;
    int          maxHistoWidth         = 0;
    int          nrChan                = 0;
    int          spectrNrEmitTrig      = 0;
    int          spectrGeoAmp          = 0;
    unsigned int downSample            = 0;
    int          procFrame             = 0;
    int          lastDownloadFrame     = 0;
    int          lastEnerDownloadFrame = 0;
    int          procEnerFrame         = 0;

    bool normalize       = false;
    bool getBounds       = false;
    bool getEnergyCenter = false;
    bool b_getEnergySum  = false;

    float valThres           = 0.f;
    float indValThres        = 0.f;
    float maxValOfType       = 0.f;
    float maxValPerChan      = 0.f;
    float maxValSum          = 0.f;
    float maximum            = 0.f;
    float minimum            = 0.f;
    float spectrMax          = 0.f;
    float spectrMin          = 0.f;
    float energySum          = 0.f;
    float energySumLowThresh = 0.1f;
    int   spectrMaxInd       = 0;
    int   spectrMinInd       = 0;

    float      smoothing = 0.f;
    glm::ivec2 cellSize{0};
    glm::ivec2 nrCells{0};
    glm::ivec2 histoTexSize{0};
};
}  // namespace ara
