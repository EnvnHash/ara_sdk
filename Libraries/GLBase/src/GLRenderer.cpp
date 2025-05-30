//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#include "GLRenderer.h"
#include <GLBase.h>
#include <GeoPrimitives/Quad.h>

using namespace glm;
using namespace std;

namespace ara {
/** init the gl context, create a new window, disable the context after
   creation. return true if the context was created and false if it already
   existed */
bool GLRenderer::init(const std::string& name, glm::ivec2 pos, glm::ivec2 dimension, bool hidden) {
#ifdef ARA_USE_GLFW
    bool restartGlBaseLoop = false;

    // check if GLBase renderloop is running, if this is the case, stop it and
    // start it later again. Otherwise context sharing will fail
    if (m_glbase && m_glbase->isRunning()) {
        restartGlBaseLoop = true;
        m_glbase->stopProcCallbackLoop();
    }

    m_dim        = glm::vec2(dimension);
    m_pos        = glm::vec2(pos);
    m_name       = name;
    m_fontHeight = 45;

    // add a new window through the GWindowManager
    m_winHandle = m_glbase->getWinMan()->addWin(glWinPar{
        .createHidden = hidden,
        .shift = static_cast<ivec2>(m_pos),
        .size = static_cast<ivec2>(m_dim),
        .scaleToMonitor = true,
        .shareCont = static_cast<void *>(m_glbase->getGlfwHnd()),
    });

    if (!m_winHandle) {
        return false;
    }

    m_winHandle->makeCurrent();

    initGLEW();

    if (!s_inited) {
        WindowBase::init({}, dimension);
        initGL();

        glfwMakeContextCurrent(nullptr);
        m_winHandle->startDrawThread(std::bind(&GLRenderer::draw, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));  // window context will be made current
                                                                         // within the thread, so release it before
        s_inited = true;
    }

    if (restartGlBaseLoop && m_glbase) {
        if (!s_inited) {
            glfwMakeContextCurrent(nullptr);
        }
        m_glbase->startGlCallbackProcLoop();
        // no context bound at this point
    }
#endif
    return true;
}

#ifdef ARA_USE_GLFW

void GLRenderer::initFromWinMan(const std::string& name, GLFWWindow *win) {
    m_dim       = glm::vec2(win->getWidth(), win->getHeight());
    m_pos       = glm::vec2(win->getPosition());
    m_winHandle = win;
    m_name      = name;
    s_inited    = true;
}

#endif

void GLRenderer::initGL() {
    glLineWidth(1.f);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);  // bei der implementation von nvidia gibt es nur
                               // LineWidth 0 -1 ...
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_stdCol = s_shCol.getStdCol();
    m_stdTex = s_shCol.getStdTex();

    m_stdQuad = make_unique<Quad>(QuadInitParams{ .color = { 0.f, 0.f, 0.f, 1.f} });
    m_flipQuad = make_unique<Quad>(QuadInitParams{ .flipHori = true });
    glGenVertexArrays(1, &m_nullVao);

    glFinish();  // glbase renderloop may not be running, so be sure the command queue is executed

    s_inited = true;
}

void GLRenderer::freeGLResources() {
    if (s_inited) {
        if (m_typo) m_typo.reset();
        m_stdQuad.reset();
        m_flipQuad.reset();
        s_shCol.clear();
        s_inited = false;
    }
}

void GLRenderer::loadTypo(const filesystem::path& typoPath) {
    if (!m_typo && s_inited) {
        m_typo = make_unique<TypoGlyphMap>(static_cast<uint>(m_dim.x), static_cast<uint>(m_dim.y));
        m_typo->loadFont((m_dataPath / typoPath).string().c_str(), &s_shCol);
    }
}

bool GLRenderer::draw(double time, double dt, int ctxNr) {
    m_doSwap = m_procSteps[Draw].active;

    if (s_inited) {
        m_procSteps[Callbacks].active = true;  // always process callbacks

        s_drawMtx.lock();

        // process all steps which have the first and second bool set to true
        for (auto &it : m_procSteps) {
            if (it.second.active) {
                if (it.second.func) it.second.func();
                it.second.active = false;
            }
        }
        s_drawMtx.unlock();
    }

    return m_doSwap;  // GLFWWindow will call glfwSwapBuffers when swap is true
}

void GLRenderer::update() {
#ifdef ARA_USE_GLFW
    if (m_winHandle->isRunning() && m_winHandle->isInited()) {
        m_procSteps[Draw].active = true;
        m_winHandle->iterate();
    }
#endif
}

void GLRenderer::iterate() {
#ifdef ARA_USE_GLFW
    if (m_winHandle->isRunning() && m_winHandle->isInited()) m_winHandle->iterate();
#endif
}

void GLRenderer::close(bool direct) {
    if (!s_inited) {
        return;
    }

    if (!direct) {
        m_glbase->runOnMainThread([this] {
            closeEvtLoopCb();
            return true;
        });
    } else {
        closeEvtLoopCb();
    }
}

bool GLRenderer::closeEvtLoopCb() {
#ifdef ARA_USE_GLFW
    // check if the window was already closed;
    if (!s_inited) {
        return true;
    }

    if (m_winHandle) {
        m_winHandle->stopDrawThread();  // also calls close() on GLFWWindow
    }

    if (m_glbase) {
        // remove all gl resources, since all gl contexts are shared, this can be done on any context
        m_glbase->addGlCbSync([this]() {
            m_stdQuad.reset();
            m_flipQuad.reset();
            s_shCol.clear();
            glDeleteVertexArrays(1, &m_nullVao);
            return true;
        });

        clearGlCbQueue();

        // if this was called from key event callback, it will cause a crash, because we are still iterating through
        // GLFWWindow member variables
        m_glbase->getWinMan()->removeWin(m_winHandle, false);  // removes the window from the ui_windows array
    }
    s_inited = false;
#endif
    return true;
}

}  // namespace ara
