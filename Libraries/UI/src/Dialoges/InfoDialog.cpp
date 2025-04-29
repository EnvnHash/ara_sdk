//
// Created by user on 01.03.2021.
//

#include "Dialoges/InfoDialog.h"

#include "UIApplication.h"

using namespace std;
using namespace std::chrono;

/**
 * InfoDialog meant to be opened and close only via a UIApplication instance
 */

namespace ara {

InfoDialog::InfoDialog(const UIWindowParams& params)
    : UIWindow(params) {
    setEnableWindowResizeHandles(false);
    setEnableMenuBar(false);

    auto base = getRootNode()->addChild<Div>("infoDiag");
    base->setPadding(10);
    base->setBorderWidth(1);
    base->setBorderColor(110.f / 255.f, 110.f / 255.f, 110.f / 255.f, 1.f);
    base->setBackgroundColor(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.f);

    m_msgLabel = base->addChild<Label>("infoDiag.message");
    m_msgLabel->setName("InfoDialogMessage");
    if (!m_infoMsg.empty()) m_msgLabel->setText(m_infoMsg);

    m_okButton = base->addChild<Button>("infoDiag.okButt");
    m_okButton->setVisibility(false);
    m_okButton->addMouseClickCb([this](hidData *data) {
        m_glbase->runOnMainThread([this] {
            close(m_confirmCb);
            return true;
        });
    });

    m_cancelButton = base->addChild<Button>("infoDiag.cancelButt");
    m_cancelButton->setVisibility(false);
    m_cancelButton->addMouseClickCb([this](hidData *data) {
        m_glbase->runOnMainThread([this] {
            close(m_cancelCb);
            return true;
        });
    });

    // save creation Time
    m_creationTime = std::chrono::system_clock::now();
}

void InfoDialog::open(bool isModal) {
    setModal(isModal);
    m_creationTime = std::chrono::system_clock::now();

#ifdef ARA_USE_GLFW
    getWinHandle()->setFloating(isModal);
#endif

    UIWindow::open();
}

void InfoDialog::close(std::function<bool()> cb) {
    // check how long the window has been open
    auto openTime = duration_cast<milliseconds>(system_clock::now() - m_creationTime);
    if (openTime.count() > m_minStayTime) {
        addCloseEvent(cb);
    } else {
        std::thread([this, cb, openTime] {
            this_thread::sleep_for(milliseconds(m_minStayTime - openTime.count()));
            m_glbase->runOnMainThread([this, cb] {
                addCloseEvent(cb);
                return true;
            });
        }).detach();
    }
}

void InfoDialog::addCloseEvent(std::function<bool()> cb) {
    // this will be called only from the eventloop
    bool closeAndRemove = true;

    hide();  // immediately hide the window, so the user knows his interaction
             // was received

    if (m_diagType == info || m_diagType == warning || m_diagType == error) {
        if (m_closeCb) m_closeCb();
        if (cb) {
            closeAndRemove = cb();
        }
    } else if (m_diagType == confirm) {
        if (cb) {
            closeAndRemove = cb();
        }
    }

    if (closeAndRemove) {
        UIWindow::close(true);
        if (m_removeCb) {
            m_removeCb();
        }
    }
}

void InfoDialog::setType(infoDiagType tp) {
    m_diagType = tp;
    if (!m_msgLabel) return;
    m_msgLabel->clearStyles();
    m_msgLabel->addStyleClass("infoDiag.message");

    switch (tp) {
        case info:
            m_msgLabel->addStyleClass("infoDiag.message.info");
            m_okButton->setVisibility(false);
            m_okButton->setX(0.f);
            m_cancelButton->setVisibility(false);
            break;
        case confirm:
            m_msgLabel->addStyleClass("infoDiag.message.error");
            m_okButton->setVisibility(true);
            m_okButton->setX(65);
            m_cancelButton->setVisibility(true);
            m_cancelButton->setX(-65);
            break;
        case warning:
            m_msgLabel->addStyleClass("infoDiag.message.warning");
            m_okButton->setVisibility(false);
            m_cancelButton->setVisibility(false);
            break;
        case error:
            m_msgLabel->addStyleClass("infoDiag.message.error");
            m_okButton->setVisibility(true);
            m_okButton->setX(0.f);
            m_cancelButton->setVisibility(false);
            break;
        case cancel:
            m_msgLabel->addStyleClass("infoDiag.message.error");
            m_okButton->setVisibility(false);
            m_cancelButton->setVisibility(true);
            m_cancelButton->setX(0.f);
            break;
        default: break;
    }

    m_sharedRes.setDrawFlag();
}

void InfoDialog::setInfoMsg(std::string msg) {
    m_infoMsg = msg;
    if (m_msgLabel) {
        m_msgLabel->setText(msg);
    }
}

}  // namespace ara