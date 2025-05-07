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

#pragma once

#include <GLBase.h>
#include <StopWatch.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include <WindowManagement/WindowBase.h>

namespace ara {
class GLRenderer : public WindowBase {
public:
    GLRenderer()           = default;
    ~GLRenderer() override = default;

    virtual bool init(const std::string& name, glm::ivec2 pos, glm::ivec2 dimension, bool hidden = false);

#ifdef ARA_USE_GLFW
    virtual void initFromWinMan(const std::string& name, GLFWWindow *win);
#endif

    virtual void     initGL();
    virtual void     loadTypo(std::filesystem::path typoPath);
    virtual void     update();
    virtual void     iterate();
    virtual Shaders *initVwfRenderShdr();
    virtual Shaders *initFlatWarpShdr();
    virtual Shaders *initExportShdr();
    virtual Shaders *initExportBlendShdr();

    bool draw(double time, double dt, int ctxNr) override;

    virtual void freeGLResources();

#ifdef ARA_USE_GLFW
    void swap() const {
        if (s_inited && m_winHandle) m_winHandle->swap();
    }

    void makeCurrent(bool value = true) const {
        if (s_inited && m_winHandle) value ? m_winHandle->makeCurrent() : glfwMakeContextCurrent(nullptr);
    }

    void show() {
        if (s_inited && m_winHandle && m_glbase)
            m_glbase->runOnMainThread([this] {
                m_winHandle->open();
                update();
                return true;
            });
    }  // must be called from he main thread

    void hide() const {
        if (s_inited && m_winHandle && m_glbase)
            m_glbase->runOnMainThread([this] {
                m_winHandle->hide();
                return true;
            });
    }  // must be called from he main thread
#endif

    virtual void close(bool direct = false);

    virtual bool closeEvtLoopCb();

    // void close()                     { if (s_inited && m_winHandle)
    // m_winHandle->close(); } virtual void destroy();
    virtual void setGlBase(GLBase *glbase) { m_glbase = glbase; }
    bool         isInited() const { return s_inited; }

#ifdef ARA_USE_GLFW
    bool        isOpen() const { return (s_inited && m_winHandle) ? m_winHandle->isOpen() : false; }
    GLFWWindow *getWin() const { return m_winHandle; }
    glm::ivec2  getPosition() const { return m_winHandle ? m_winHandle->getPosition() : glm::ivec2(0.f, 0.f); }
#endif

    Shaders      *getVwfShader() const { return m_vwfShader; }
    Shaders      *getFlatWarpShader() const { return m_flatWarpShader; }
    Shaders      *getExportShader() const { return m_exportShader; }
    Shaders      *getExportBlendShader() const { return m_exportBlendShader; }
    Quad         *getStdQuad() const { return m_stdQuad.get(); }
    Quad         *getStdFlipQuad() const { return m_flipQuad.get(); }
    TypoGlyphMap *getTypo() const { return m_typo.get(); }
    void          onKeyDown(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) override {}
    void          onKeyUp(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) override {}
    void          onChar(unsigned int codepoint) override {}
    void onMouseDownLeft(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) override {}
    void onMouseDownLeftNoDrag(float xPos, float yPos) override {}
    void onMouseUpLeft() override {}
    void onMouseDownRight(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) override {}
    void onMouseUpRight() override {}
    void onMouseMove(float xPos, float yPos, ushort mode) override {}
    virtual void onWheel(int _deg) {}
    void         onResizeDone() override {}
    void         onSetViewport(int x, int y, int width, int height) override {}

protected:
    GLBase               *m_glbase = nullptr;
    std::unique_ptr<Quad> m_stdQuad;
    std::unique_ptr<Quad> m_flipQuad;
    Shaders              *m_stdCol            = nullptr;
    Shaders              *m_testPicShdr       = nullptr;
    Shaders              *m_stdTex            = nullptr;
    Shaders              *m_vwfShader         = nullptr;
    Shaders              *m_flatWarpShader    = nullptr;
    Shaders              *m_exportShader      = nullptr;
    Shaders              *m_exportBlendShader = nullptr;

    glm::vec2 m_dim{};
    glm::vec2 m_pos{};

    std::filesystem::path m_dataPath;

    bool m_doSwap = false;
    int  m_fontHeight{};

#ifdef ARA_USE_GLFW
    GLFWWindow *m_winHandle = nullptr;
#else
    void *m_winHandle = nullptr;
#endif
    std::string                   m_name;
    std::unique_ptr<TypoGlyphMap> m_typo;

    StopWatch m_drawWatch;

    GLuint m_nullVao = 0;
};
}  // namespace ara