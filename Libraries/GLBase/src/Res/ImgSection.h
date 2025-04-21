//
// Created by Sven Hahne on 04.02.2021.
//
#pragma once

#include <Utils/Texture.h>

#include "Res/ResNode.h"

namespace ara {

class Texture;

class ImageBase : public ResNode {
public:
    enum class Type { none, simple, frame };
    enum class Dist { none, horz, vert };
    ImageBase(const std::string& name, GLBase *glbase) : ResNode(name, glbase) {}

    virtual ~ImageBase() = default;

    virtual Texture             *getTexture() { return nullptr; }
    virtual GLuint               getTexID() { return 0; }
    virtual glm::ivec2          &getTexSize() { return m_texSize; }
    virtual int                 *getSrcPixSize() { return m_nullIntPtr; }
    virtual int                 *getSectionSize() { return m_nullIntPtr; }
    virtual int                 *getSectionPos() { return m_nullIntPtr; }
    virtual Type                getType() { return m_Type; }
    virtual Dist                getDist() { return m_Dist; }
    int                         getVer(int ver) { return (m_Ver.find(ver) != m_Ver.end()) ? m_Ver[ver] : m_VerDefault; }

    std::array<int, 2> getVerPos(int ver);  // returns pos
    int  getDistPixOffset() const { return m_DistPixOffset; }
    int *getSectionSep() { return m_PixSep; }

protected:
    static inline int   m_nullIntPtr[2] = {0, 0};
    Type                m_Type          = Type::none;
    Dist                m_Dist          = Dist::none;
    int                 m_PixSep[2]     = {0, 0};  // separation between each component in pixels
    int                 m_DistPixOffset = 0;
    int                 m_VerDefault    = 0;    // if a given version isn't found then this value will be used, by default is zero
    std::map<int, int>  m_Ver;                  // version, this value multiplies in m_Dist direction by m_DistPixOffset
    glm::ivec2          m_texSize{0};
};

class ImgSrc : public ImageBase {
public:
    std::string              m_ImgPath;
    int                      m_GridSize[2] = {1, 1};
    int                      m_MipMaps     = 1;
    std::unique_ptr<Texture> m_texMan;

    explicit ImgSrc(const std::string& name, GLBase *glbase) : ImageBase(name, glbase) {
        m_texMan = std::make_unique<Texture>(glbase);
    }

    virtual ~ImgSrc() = default;

    bool OnProcess() override;

    bool OnLoad() override {
        return LoadImg(m_MipMaps);
    }

    bool OnResourceChange(bool deleted, const std::string &res_fpath) override {
        if (deleted) {
            return false;
        }
        return LoadImg(m_MipMaps);
    }

    bool isOK() override { return m_texMan->getWidth() > 0 && m_texMan->getHeight() > 0 && m_texMan->getBpp() > 0; }
    static bool isClass(ResNode *snode) { return snode->getFlag("imgsrc") != nullptr; }

    bool LoadImg(int mimMapLevel = 1);

    Texture*           getTexture() override { return m_texMan.get(); }
    GLuint             getTexID() override { return m_texMan->getId(); }
    int               *getSrcPixSize() override { return m_texMan->getSize(); }
    int               *getSectionSize() override { return m_texMan->getSize(); }
    int               *getSectionPos() override { return m_nullIntPtr; }
};

class ImgSection : public ImageBase {
public:
    ImgSrc *m_ImgSrc = nullptr;

    int m_Pos[2]     = {0, 0};  // position of the element in GRID units from imgsrc
    int m_PixPos[2]  = {0, 0};  // m_Pos * m_ImgSrc->m_GridSize
    int m_PixSize[2] = {0, 0};  // size of each component, in pixels
    explicit ImgSection(const std::string& name, GLBase *glbase) : ImageBase(name, glbase) {}

    virtual bool OnProcess() override;

    static bool         isClass(ResNode *snode) { return snode->getFlag("img") != nullptr; }
    bool                isOK() override { return m_ImgSrc != nullptr; }
    bool                OnSourceResUpdate(bool deleted, ResNode *unode) override { return true; };
    Texture*            getTexture() override { return m_ImgSrc != nullptr ? m_ImgSrc->getTexture() : nullptr; };
    GLuint              getTexID() override { return m_ImgSrc != nullptr ? m_ImgSrc->getTexID() : 0; };
    int                 *getSrcPixSize() override { return m_ImgSrc ? m_ImgSrc->getSrcPixSize() : nullptr; };
    int                 *getSectionSize() override { return m_PixSize; };
    int                 *getSectionPos() override { return m_PixPos; }
};

}  // namespace ara
