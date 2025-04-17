//
//  ShaderCollector.h
//
//  Created by Sven Hahne on 4/6/15.
//

#pragma once

#include "Shaders/Shaders.h"

namespace ara {
class ShaderCollector {
public:
    ShaderCollector();
    ~ShaderCollector();
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
    void deleteShader(Shaders *ptr);

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
    std::string getFisheyeVertSnippet(size_t nrCameras);

    void         setShaderHeader(std::string hdr) { shdr_Header = std::move(hdr); }
    std::string &getShaderHeader() { return shdr_Header; }
    std::string &getUiObjMapUniforms() { return m_uiObjMapUniforms; }
    std::string &getUiObjMapMain() { return m_uiObjMapMain; }

    Shaders *get(const std::string &name);
    bool     hasShader(const std::string &name);

private:
    std::unordered_map<std::string, std::unique_ptr<Shaders>> shaderCollection;
    std::string                                               shdr_Header;
    std::string                                               m_uiObjMapUniforms;
    std::string                                               m_uiObjMapMain;
};
}  // namespace ara
