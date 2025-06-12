#pragma once

#include "ScrollBar.h"

namespace ara {

class ScrollView : public Div {
public:
    ScrollView();
    ~ScrollView() override = default;

    void init() override;
    void updtMatrIt(scissorStack* ss) override;
    void clearContentChildren() const;
    [[nodiscard]] std::vector<std::unique_ptr<UINode>>* getContChildren() const;
    void mouseWheel(hidData& data) override;
#ifdef __ANDROID__
    void mouseDrag(hidData& data) override;
#endif
    void setViewport(float x, float y, float width, float height) override;

    /** in pixels, origin top-left, +y is towards screen bottom */
    virtual void setScrollOffset(float offsX, float offsY);
    void         setOnScrollCb(std::function<void()> func) { m_scrollCb = std::move(func); }
    glm::vec4&   getRawPadding() { return m_origPadding; }

    /** get the bounding box around the children in parent relative coordinates
     * (without the parent's transformation matrix) plus padding  */
    glm::vec2 getBBSize();
    void setPadding(float val);
    void setPadding(float left, float top, float right, float bot);
    void setPadding(glm::vec4& val);
    void blockVertScroll(bool val) { m_blockVerScroll = val; }
    void blockHorScroll(bool val) { m_blockHorScroll = val; }
    void setAdaptContentTrans(bool val) { m_adaptContentTrans = val; }

    template <typename T, typename... Args>
    requires (sizeof...(Args) != 1 || (!std::is_same_v<Args, UINodePars> && ...))
    T* addChild(Args&& ... args) {
        return m_content->addChild<T>(args...);
    }

    template<typename T>
    T* addChild(const UINodePars& arg) {
        return m_content->addChild<T>(arg);
    }

    int m_scrollBarSize = 16;

    ScrollBar* m_VSB    = nullptr;
    ScrollBar* m_HSB    = nullptr;
    Div*       m_corner  = nullptr;
    Div*       m_content = nullptr;

protected:
    glm::vec4   m_origPadding{0.f};
    glm::vec4   m_bb{0.f};
    glm::vec2   m_bbSize{0.f};
    glm::vec2   m_maxOffs{0.f};
    glm::vec2   m_offs{0.f};
    glm::vec2   m_newPadd{0.f};
    glm::vec2   m_newOffs{0.f};
    glm::vec2   m_dragInitOffs{0.f};

    std::function<void()> m_scrollCb;

    bool    m_blockVerScroll    = false;
    bool    m_blockHorScroll    = false;
    bool    m_adaptContentTrans = false;
    bool    m_needH             = false;
    bool    m_needV             = false;
};

}  // namespace ara
