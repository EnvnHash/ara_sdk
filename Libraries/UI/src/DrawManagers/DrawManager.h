//
// Created by user on 30.12.2021.
//

#pragma once

#include "Utils/VAO.h"
#include "ui_common.h"

namespace ara {

class UINode;
class Shaders;
class ShaderCollector;
class GLBase;

class DrawSet {
public:
    DrawSet()          = default;
    virtual ~DrawSet() = default;

    Shaders*                                                  shdr = nullptr;
    VAO                                                       vao;
    std::deque<std::vector<DivVaoData>*>                      divData;
    std::deque<std::vector<GLuint>*>                          divIndices;
    std::unordered_map<GLuint, GLuint>                        fontTex;  // texId, texUnit
    std::vector<float>                                        layerSizes;
    std::vector<GLint>                                        layerUnits;
    std::unordered_map<GLuint, GLuint>                        textures;  // texId, texUnit
    std::deque<std::pair<std::vector<DivVaoData>*, uint32_t>> updtNodes;
    GLuint                                                    vaoSize    = 0;
    GLuint                                                    indOffs    = 0;
    GLuint                                                    imgUnitCnt = 0;
    std::function<void()>                                     func       = nullptr;
};

class DrawManager {
public:
    DrawManager(GLBase* glbase) : m_glbase(glbase) {};
    virtual ~DrawManager() = default;

    virtual Shaders*                getShader(DrawSet& ds);
    bool                            rebuildVaos();
    void                            update();
    virtual void                    draw();
    std::list<DrawSet>::reference   push(IndDrawBlock& block, UINode* node);
    float                           pushFont(GLuint texId, float nrLayers);
    float                           pushTexture(GLuint texId);
    float                           pushTexture(DrawSet& ds, GLuint texId);
    static void                     popTexture(DrawSet& ds, GLuint texId);
    void                            pushFunc(const std::function<void()>& f);
    void                            clear();
    void                            clearFonts();

    void     addSet() { m_drawSets.emplace_back(); }
    DrawSet& getWriteSet() { return m_drawSets.back(); }
    void     setShaderCollector(ShaderCollector* sc) { m_shCol = sc; }
    void     setMaxObjId(uint32_t id) { m_maxObjId = id; }
    void     removeUINode(UINode* node) { m_nodeList.remove(node); }

protected:
    GLBase*          m_glbase = nullptr;
    ShaderCollector* m_shCol  = nullptr;
    Shaders*         m_shdr   = nullptr;

    std::list<DrawSet>           m_drawSets;
    std::list<DrawSet>::iterator m_ws = m_drawSets.end();
    std::list<UINode*>           m_nodeList;

    uint32_t m_maxObjId  = 0;
    uint32_t m_vaoOffset = 0;

    std::vector<GLint> m_texUnitMap;
};

}  // namespace ara
