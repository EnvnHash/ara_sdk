#pragma once

#include "ScrollBar.h"

namespace ara {

class ScrollView : public Div {
public:
    ScrollView();
    explicit ScrollView(const std::string& styleClass);
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

    UINode* addChild(std::unique_ptr<UINode>&& child) override;

    template <typename T>
    T* addChild() {
        if (!m_content) {
            return nullptr;
        }
        return static_cast<T*>(m_content->addChild(std::make_unique<T>()));
    }

    template <typename T>
    T* addChild(const std::string& styleClass) {
        if (!m_content) {
            return nullptr;
        }
        auto nc = m_content->addChild<T>();
        nc->addStyleClass(styleClass);
        return nc;
    }


    template <typename T, CoordinateType C>
    T* addChild(C x, C y, C w, C h) {
        if (!m_content) {
            return nullptr;
        }
        auto nc = m_content->addChild<T>();
        nc->setPos(x, y);
        nc->setSize(w, h);
        return nc;
    }

    template <typename T>
    T* addChild(int x, int y, int w, int h, const float* fgcolor, const float* bkcolor) {
        auto n = addChild<T>(x, y, w, h);
        if (fgcolor != nullptr) {
            n->setColor(fgcolor[0], fgcolor[1], fgcolor[2], fgcolor[3]);
        }
        if (bkcolor != nullptr) {
            n->setBackgroundColor(bkcolor[0], bkcolor[1], bkcolor[2], bkcolor[3]);
        }
        return n;
    }

    template <typename T>
    T* addChild(int x, int y, int w, int h, glm::vec4 fgcolor, glm::vec4 bkcolor) {
        auto n = addChild<T>(x, y, w, h);
        n->setColor(fgcolor);
        n->setBackgroundColor(bkcolor);
        return n;
    }

    template <typename T>
    T* addChild(int x, int y, int w, int h, std::filesystem::path path) {
        auto n = addChild<T>(x, y, w, h);
        n->setPath(std::move(path));
        return n;
    }

    int m_scrollBarSize = 16;

    ScrollBar* ui_VSB    = nullptr;
    ScrollBar* ui_HSB    = nullptr;
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

    // temporary local values made member values for performance reasons;
    std::pair<bool, bool> itRes;
    std::pair<bool, bool> boolPair;
};

}  // namespace ara
