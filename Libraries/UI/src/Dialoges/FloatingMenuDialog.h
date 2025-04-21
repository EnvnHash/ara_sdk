//
// Created by user on 09.04.2021.
//

#pragma once

#include "UIWindow.h"

namespace ara {

class FloatingMenuDialog : public UIWindow {
public:
    FloatingMenuDialog(GLBase* glbase, int width, int height, int shiftX, int shiftY, bool osDecoration,
                       bool transparentFB = false, bool floating = false, bool initToCurrentCtx = false,
                       bool multisample = true, void* extWinHandle = nullptr, bool scaleToMonitor = false);
    virtual ~FloatingMenuDialog() = default;

    virtual void close(bool direct = false);

    void setOnClose(const std::function<void(std::string)>& f) { m_onCloseFunc = f; }
    void setRemoveCb(const std::function<void()>& f) { m_removeCb = f; }

    template <class T>
    T* addItem(const std::string& returnValue, std::string style = "") {
        if (!m_base) {
            return nullptr;
        }

        T* newItem;
        if (!style.empty()) {
            newItem = m_base->addChild<T>(std::move(style));
        } else {
            newItem = m_base->addChild<T>();
        }

        ((UINode*)newItem)->addMouseUpCb([this, returnValue](hidData* data) {
            m_closing = true;
            // window creation and destruction must be done on the main thread
            m_glbase->runOnMainThread([this, returnValue]() {
                if (m_onCloseFunc) {
                    m_onCloseFunc(returnValue);
                }
                close();
                return true;
            }, true);
        });

        return newItem;
    }

protected:
    bool                             m_closing = false;
    Div*                             m_base    = nullptr;
    std::function<void(std::string)> m_onCloseFunc;
    std::function<void()>            m_removeCb;
};

}  // namespace ara
