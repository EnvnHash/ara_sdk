//
// Created by user on 09.04.2021.
//

#include "FloatingMenuDialog.h"

#include <UIApplication.h>

#ifdef ARA_USE_GLFW
using namespace ara::mouse;
#endif

namespace ara {

FloatingMenuDialog::FloatingMenuDialog(const UIWindowParams& params)
    : UIWindow(params) {
#ifdef ARA_USE_GLFW
    setEnableWindowResizeHandles(false);
    setEnableMenuBar(false);

    m_base = getRootNode()->addChild<Div>();
    m_base->setBackgroundColor(0.2f, 0.2f, 0.2f, 1.f);

    // add a callback to listen for clicks on all windows
    m_glbase->getWinMan()->addGlobalMouseButtonCb(this, [this](GLFWwindow* win, int, int, int) {
        // sync with this window's gl thread
        addGlCb(this, "close", [this]() {
            if (!m_closing && getRootNode()->isInited()) {
                int x, y;
                getAbsMousePos(x, y);

                if (x < getPosition().x || x > (getPosition().x + getSize().x) || y < getPosition().y ||
                    y > (getPosition().y + getSize().y)) {
                    m_closing = true;

                    m_glbase->runOnMainThread([this] {
                        close();
                        return true;
                    });
                }
            }
            return true;
        });
        iterate();  // force the gl thread to iterate
    });

    startRenderLoop();
#endif
}

void FloatingMenuDialog::close(bool direct) {
#ifdef ARA_USE_GLFW
    m_glbase->getWinMan()->removeGlobalMouseButtonCb(this);
    UIWindow::close(true);
    if (getApplicationHandle()) {
        getApplicationHandle()->removeWindow(this);
    }
#endif
}

}  // namespace ara