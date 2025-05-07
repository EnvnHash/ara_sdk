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

#include "Shaders/Shaders.h"

namespace ara {
class ShaderCollector {
public:
    ShaderCollector();
    ~ShaderCollector() = default;
    Shaders *add(const std::string &name, const std::string &vert, const std::string &frag);
    Shaders *add(const std::string &name, const std::string &vert, const std::string &geom, const std::string &frag);
    Shaders *add(const std::string &name, const std::string &vert, const std::string &cont, const std::string &eval,
                 const std::string &geom, const std::string &frag);
    Shaders *add(const std::string &name, const std::string &comp);
    Shaders *addNoLink(const std::string &name, const std::string &vert, const std::string &frag);
    Shaders *addNoLink(const std::string &name, const std::string &vert, const std::string &geom,
                       const std::string &frag);

    void clear();
    void deleteShader(const std::string &name);
    void deleteShader(const Shaders *ptr);

    Shaders    *getStdClear(bool layered = false, int nrLayers = 1);
    Shaders    *getStdCol();
    Shaders    *getStdParCol();
    Shaders    *getStdColAlpha();
    Shaders    *getStdColBorder();
    Shaders    *getStdTexBorder();
    Shaders    *getStdDirLight();
    Shaders    *getStdClassicOpenGlLight();
    Shaders    *getStdRec();
    Shaders    *getStdTexColConv();
    Shaders    *getStdTex();
    Shaders    *getStdTexNullVao();
    Shaders    *getStdParColNullVao();
    Shaders    *getGridShdrNullVao();
    Shaders    *getStdGreyTex();
    Shaders    *getStdDepthDebug();
    Shaders    *getStdTexMulti();
    Shaders    *getStdTexAlpha(bool multiSampTex = false);
    Shaders    *getStdGlyphShdr();
    Shaders    *getUIParCol();
    Shaders    *getUIObjMapOnly();
    Shaders    *getUIColBorder();
    Shaders    *getUITex();
    Shaders    *getUITexInv();
    Shaders    *getUIGridTexSimple();
    Shaders    *getUIGridTexYuv();
    Shaders    *getUIGridTexFrame();
    Shaders    *getUIGreyTex();
    Shaders    *getEdgeDetect();
    Shaders    *getStdHeightMapSobel();
    Shaders    *getPerlin();
    static std::string getFisheyeVertSnippet(size_t nrCameras);

    void         setShaderHeader(std::string hdr) { shdr_Header = std::move(hdr); }
    std::string &getUiObjMapUniforms() { return m_uiObjMapUniforms; }
    std::string &getUiObjMapMain() { return m_uiObjMapMain; }

    static std::string &getShaderHeader() { return shdr_Header; }

    Shaders *get(const std::string &name);
    bool     hasShader(const std::string &name) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Shaders>> shaderCollection;
#ifdef __APPLE__
    static inline std::string                                 shdr_Header = "#version 410\n";
#else
    static inline std::string                                 shdr_Header = "#version 430\n";
#endif
    std::string                                               m_uiObjMapUniforms;
    std::string                                               m_uiObjMapMain;
};
}  // namespace ara
