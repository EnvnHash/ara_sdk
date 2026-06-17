//
// Created by sven on 04-05-26.
//

#include "DemoView.h"
#include "UIElements/Resizable.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_MouseCursor::DemoView_MouseCursor() : DemoView("MouseCursor", vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_MouseCursor>());
}

void DemoView_MouseCursor::init() {
}

void DemoView_MouseCursor::mouseIn(hidData &data) {
    const auto win = getWindow();
    const auto glfwWin = win->getWinHandle();
    m_cursor = glfwWin->createCircularMouseCursor(20, {0.5f, 0.5f, 0.5f, 0.5f});
    glfwSetCursor(static_cast<GLFWwindow*>(glfwWin->getWin()), m_cursor);
}

void DemoView_MouseCursor::mouseOut(hidData &data) {
    if (m_cursor) {
        glfwDestroyCursor(m_cursor);
        m_cursor = nullptr;
    }
}