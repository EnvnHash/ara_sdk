//
// Created by user on 01.03.2021.
//

#pragma once

#include <UIWindow.h>

namespace ara {

class Label;
class Button;

class InfoDialog : public UIWindow {
public:
    InfoDialog(const UIWindowParams& params);
    ~InfoDialog() override = default;

    void open(bool isModal);
    void close(const std::function<bool()>& cb = nullptr);
    void addCloseEvent(const std::function<bool()>& cb);
    void setType(infoDiagType tp);

    void setInfoMsg(const std::string& msg);
    void setMinStayTime(int ms) { m_minStayTime = ms; }
    void setConfirmCb(const std::function<bool()>& f) { m_confirmCb = f; }
    void setCloseCb(const std::function<void()>& f) { m_closeCb = f; }
    void setCancelCb(const std::function<bool()>& f) { m_cancelCb = f; }
    void setRemoveCb(const std::function<void()>& f) { m_removeCb = f; }

    [[maybe_unused]] long getMinStayTime() const { return m_minStayTime; }

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
