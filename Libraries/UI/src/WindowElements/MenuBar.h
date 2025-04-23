//
// Created by user on 05.10.2020.
//

#pragma once

#include "WindowManagement/GLFWWindow.h"

#include <utility>

#include "Image.h"
#include "Button/ImageButton.h"

namespace ara {

class MenuBar : public Image {
public:
    enum class butType : int { close = 0, maximize, minimize };
    MenuBar();
    virtual ~MenuBar() = default;

    void init() override;

    void hideButtons(bool val) {
        m_showButtons = !val;
        for (auto& it : m_menButtons) {
            it.second->setVisibility(!val);
        }
    }
#ifdef ARA_USE_GLFW
    void setWindowHandle(GLFWWindow* win) { m_win = win; }
#else
    void setWindowHandle(void* win) { m_win = win; }
#endif
    void setCloseIcon(const std::string& file) {}
    void setCloseFunc(std::function<void()> func) { m_closeFunc = std::move(func); }
    void setMinimizeFunc(std::function<void()> func) { m_minimizeFunc = std::move(func); }
    void setMaximizeFunc(std::function<void()> func) { m_maximizeFunc = std::move(func); }
    void setRestoreFunc(std::function<void()> func) { m_restoreFunc = std::move(func); }

    void mouseUp(hidData* data) override;
    void mouseDown(hidData* data) override;
    void mouseDrag(hidData* data) override;
    void setEnableMinMaxButtons(bool val);

private:
    bool m_showButtons = true;
#ifdef ARA_USE_GLFW
    GLFWWindow* m_win = nullptr;
#else
    void* m_win = nullptr;
#endif
    glm::ivec2                      m_dragStartWinPos;
    glm::ivec2                      m_mouseDownPixelPos;
    glm::vec4                       m_brightAdjCol;
    std::vector<Div*>               m_buttContainers;
    std::map<butType, ImageButton*> m_menButtons;
    float                           m_brightAdj           = -1.f;
    bool                            m_enableMinMaxButtons = true;

    Div* m_iconsRightSide = nullptr;

    std::function<void()> m_closeFunc;
    std::function<void()> m_minimizeFunc;
    std::function<void()> m_maximizeFunc;
    std::function<void()> m_restoreFunc;
};
}  // namespace ara
