#pragma once

#include <Res/Font.h>
#include <Utils/FBO.h>

namespace ara {

class Shaders;

enum fontalign { center = 0, left, right, justify, justify_ex };
enum fontvalign { vcenter = 0, top, bottom };

class Font;

class FontList {
public:
    Font *get(const std::string& font_path, int size, float pixRatio);
    [[nodiscard]] Font *find(const std::string &font_path, int size, float pixRatio) const;
    Font *add(const std::string& font_path, int size, float pixRatio);
    Font *addFromFilePath(const std::string& font_path, int size, float pixRatio);
    Font *add(std::vector<uint8_t> &vp, const std::string& font_path, int size, float pixRatio);
    void  update3DLayers();

    void  setGlbase(GLBase *glbase) { m_glbase = glbase; }
    int   getCount() const { return static_cast<int>(m_FontList.size()); }
    Font *get(int index) const { return (index < 0 || index >= getCount()) ? nullptr : m_FontList[index].get(); }
    void  clear() { m_FontList.clear(); }

private:
    std::vector<std::unique_ptr<Font>>                m_FontList;
    std::unordered_map<int, std::unique_ptr<Texture>> m_fontTexLayers;
    std::unordered_map<int, std::list<Font *>>        m_layerCount;
    FBO                                               m_fbo;
    GLBase                                           *m_glbase = nullptr;
};

}  // namespace ara