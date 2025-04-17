//
// Created by sven on 04-03-25.
//

#pragma once

#include <DataModel/Node.h>
#include <glm/glm.hpp>

namespace ara {

class GLBase;
class Texture;

namespace node {

class ImageBase : public Node {
public:
    enum class Type { none=0, simple, frame };
    enum class Dist { none=0, horz, vert };

    ImageBase();

    void setGLBase(GLBase *glbase) { m_glBase = glbase; }

    virtual Texture    *getTexture() { return nullptr; }
    virtual uint32_t    getTexID() { return 0; }
    virtual glm::ivec2  &getTexSize() { return m_texSize; }
    virtual int         *getSrcPixSize() { return m_nullIntPtr; }
    virtual int         *getSectionSize() { return m_nullIntPtr; }
    virtual int         *getSectionPos() { return m_nullIntPtr; }
    virtual Type        getType() { return m_Type; }
    virtual Dist        getDist() { return m_Dist; }

    int getVer(int ver) { return (m_Ver.find(ver) != m_Ver.end()) ? m_Ver[ver] : m_VerDefault; }
    int getDistPixOffset() { return m_DistPixOffset; }
    int *getSectionSep() { return m_PixSep.data(); }

    int *getVerPos(int *pos, int ver);  // returns pos

    //void serializeValues(nlohmann::json &j) override;
    //void deserializeValues_(const nlohmann::json &j) override;

protected:
    static inline int   m_nullIntPtr[2] = {0, 0};
    Type                m_Type = Type::none;
    Dist                m_Dist = Dist::none;
    std::array<int, 2>  m_PixSep{};             // separation between each component in pixels
    int                 m_DistPixOffset = 0;
    int                 m_VerDefault = 0;       // if a given version isn't found then this value will be used, by default is zero
    std::map<int, int>  m_Ver;                  // version, this value multiplies in m_Dist direction by m_DistPixOffset
    glm::ivec2          m_texSize{0};
    GLBase*             m_glBase = nullptr;
};

}
}
