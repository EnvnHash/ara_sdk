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

    void proc(GLint texId);
    void getMinMax(GLuint texId);
    void getSpectrum(GLuint texId) const;
    void getHistoBounds() const;
    void normalizeHisto();
    void downloadMinMax();
    void downloadEnergyCenter();

    float getMaximum(unsigned int chan = 0) {
        downloadMinMax();
        return m_minMaxSpectr[chan * 4];
    }
    float getMinimum(unsigned int chan = 0) {
        downloadMinMax();
        return m_maxValSum - m_minMaxSpectr[chan * 4 + 1];
    }
    float getMaxInd(unsigned int chan = 0) {
        downloadMinMax();
        return m_minMaxSpectr[chan * 4 + 2] / static_cast<float>(m_histoTexSize.x);
    }
    float getMinInd(unsigned int chan = 0) {
        downloadMinMax();
        return (static_cast<float>(m_histoTexSize.x - 1) - m_minMaxSpectr[chan * 4 + 3]) / static_cast<float>(m_histoTexSize.x);
    }
    float getHistoPeakInd(unsigned int chan = 0) {
        m_getEnergyCenter = true;
        downloadEnergyCenter();
        return m_energyMed[chan] / static_cast<float>(m_histoTexSize.x);
    }
    float getEnergySum(float lowThresh) {
        m_energySumLowThresh = lowThresh;
        m_getEnergySum     = true;
        return m_energySum;
    }
    [[nodiscard]] GLint getResult() const { return m_histoFbo->src->getColorImg(); }
    [[nodiscard]] GLint getMinMaxTex() const { return m_minMaxFbo->getColorImg(); }
    [[nodiscard]] float getSubtrMax() const { return m_maxValOfType; }
    void  setSmoothing(float val) { m_smoothing = val; }
    void  setValThres(float val) { m_valThres = val; }
    void  setIndValThres(float val) { m_indValThres = val; }

    void initShader();
    void initNormShader();
    void initMinMaxShader();
    void initMinMaxSpectrShader();
    void initEnergyMedShader();

private:
    GLBase*                 m_glbase           = nullptr;
    ShaderCollector*        m_shCol              = nullptr;
    Shaders*                m_histoShader        = nullptr;
    Shaders*                m_minMaxShader       = nullptr;
    Shaders*                m_minMaxSpectrShader = nullptr;
    Shaders*                m_energyMedShader    = nullptr;
    Shaders*                m_normShader         = nullptr;

    std::unique_ptr<Quad>           m_quad;
    std::unique_ptr<PingPongFbo>    m_histoFbo;
    std::unique_ptr<FBO>            m_energyMedFbo;
    std::unique_ptr<FBO>            m_minMaxFbo;
    std::unique_ptr<FBO>            m_minMaxSpectrFbo;
    std::unique_ptr<VAO>            m_trigVao;
    std::unique_ptr<VAO>            m_trigSpectrVao;

    GLenum      m_minMaxFormat{};
    GLenum      m_texType{};
    GLenum      m_format{};
    GLenum      m_texFormat{};
    GLint       m_geoAmp{};

    std::array<GLfloat, 8>  m_minMax{};
    std::vector<GLfloat>    m_minMaxSpectr;
    std::vector<GLfloat>    m_energyMed;
    std::vector<GLfloat>    m_histoDownload;

    int          m_width                 = 0;
    int          m_height                = 0;
    int          m_totNrCells            = 0;
    int          m_maxHistoWidth         = 0;
    int          m_nrChan                = 0;
    int          m_spectrNrEmitTrig      = 0;
    int          m_spectrGeoAmp          = 0;
    unsigned int m_downSample            = 0;
    int          m_procFrame             = 0;
    int          m_lastDownloadFrame     = 0;
    int          m_lastEnerDownloadFrame = 0;
    int          m_procEnerFrame         = 0;

    bool m_normalize       = false;
    bool m_getBounds       = false;
    bool m_getEnergyCenter = false;
    bool m_getEnergySum  = false;

    float m_valThres           = 0.f;
    float m_indValThres        = 0.f;
    float m_maxValOfType       = 0.f;
    float m_maxValPerChan      = 0.f;
    float m_maxValSum          = 0.f;
    float m_maximum            = 0.f;
    float m_minimum            = 0.f;
    float m_energySum          = 0.f;
    float m_energySumLowThresh = 0.1f;

    float      m_smoothing = 0.f;
    glm::ivec2 m_cellSize{0};
    glm::ivec2 m_nrCells{0};
    glm::ivec2 m_histoTexSize{0};
};
}  // namespace ara
