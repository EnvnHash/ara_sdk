//
// Created by user on 01.03.2021.
//

#pragma once

#include "Button/Button.h"
#include <Label.h>
#include <UIWindow.h>

namespace ara {

class InfoDialog : public UIWindow {
public:
    InfoDialog(GLBase *glbase, int width, int height, int shiftX, int shiftY, bool osDecoration,
               bool transparentFB = false, bool floating = false, bool initToCurrentCtx = false,
               bool multisample = true, void *extWinHandle = nullptr, bool scaleToMonitor=true);

    virtual ~InfoDialog() {}

    void open(bool isModal);
    void close(std::function<bool()> cb = nullptr);
    void addCloseEvent(std::function<bool()> cb);
    void setType(infoDiagType tp);

    void setInfoMsg(std::string msg) {
        m_infoMsg = msg;
        if (m_msgLabel) m_msgLabel->setText(msg);
    }

    void setMinStayTime(int ms) { m_minStayTime = ms; }
    void setConfirmCb(std::function<bool()> f) { m_confirmCb = f; }
    void setCloseCb(std::function<void()> f) { m_closeCb = f; }
    void setCancelCb(std::function<bool()> f) { m_cancelCb = f; }
    void setRemoveCb(std::function<void()> f) { m_removeCb = f; }
    long getMinStayTime() { return m_minStayTime; }

private:
    Label                                             *m_msgLabel     = nullptr;
    Button                                            *m_okButton     = nullptr;
    Button                                            *m_cancelButton = nullptr;
    std::string                                        m_infoMsg;
    long                                               m_minStayTime = 500;
    infoDiagType                                       m_diagType    = infoDiagType::info;
    std::chrono::time_point<std::chrono::system_clock> m_creationTime;
    std::function<bool()>                              m_confirmCb;
    std::function<void()>                              m_closeCb;
    std::function<void()>                              m_removeCb;
    std::function<bool()>                              m_cancelCb;
};

}  // namespace ara
