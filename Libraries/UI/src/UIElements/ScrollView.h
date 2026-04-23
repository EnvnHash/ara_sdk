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
    [[nodiscard]] std::list<std::shared_ptr<Node>>* getContChildren() const;
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

    template<typename T>
    requires std::is_same_v<T, glm::vec4> || std::is_same_v<T, float>
    void setPadding(const T& val, state st = state::m_state) {
        if constexpr (std::is_same_v<T, float>) {
            m_origPadding = glm::vec4(val, val, val, val);
        } else {
            m_origPadding = val;
        }
        UINode::setPadding(val);
    }

    void setPadding(const float left, const float top, const float right, const float bottom, state st = state::m_state) override {
        setPadding(glm::vec4{left, top, right, bottom});
    }

    void blockVertScroll(const bool val) { m_blockVerScroll = val; }
    void blockHorScroll(const bool val) { m_blockHorScroll = val; }
    void setAdaptContentTrans(const bool val) { m_adaptContentTrans = val; }

    template <typename T, typename... Args>
    requires (sizeof...(Args) != 1 || (!std::is_same_v<Args, UINodePars> && ...))
    T& push(Args&& ... args) {
        return m_content->push<T>(args...);
    }

    template<typename T>
    T& push(const UINodePars& arg) {
        return m_content->push<T>(arg);
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
    glm::vec2   m_newOffs{0.f};
    glm::vec2   m_dragInitOffs{0.f};

    std::function<void()> m_scrollCb;

    bool    m_blockVerScroll    = false;
    bool    m_blockHorScroll    = false;
    bool    m_adaptContentTrans = false;
};

}  // namespace ara
