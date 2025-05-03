//
// Created by user on 09.10.2020.
//

#pragma once

#include <GLBase.h>
#include <OSFileDialog.h>
#include <Asset/AssetFont.h>
#include <Asset/AssetManager.h>
#include <StopWatch.h>
#include <WindowElements/MenuBar.h>
#include <ObjectMapInteraction.h>
#include <UINodeFact.h>
#include <DrawManagers/DrawManager.h>
#include <WindowElements/WindowResizeAreas.h>
#include <WindowManagement/GLFWWindow.h>
#include <WindowManagement/GLWindow.h>
#include <WindowManagement/WindowBase.h>

namespace ara {

class UIApplication;
class ShaderProto;

struct UIWindowParams {
    GLBase* glbase = nullptr;
    glm::ivec2 size{};
    glm::ivec2 shift{};
    bool osDecoration=  false;
    bool transparentFB = false;
    bool floating = false;
    bool initToCurrentCtx = false;
    bool multisample = true;
    void* extWinHandle = nullptr;
    bool scaleToMonitor = false;
};

struct InfoDiagParams {
    glm::ivec2 pos{};
    glm::ivec2 size{};
    infoDiagType tp{};
    std::string msg;
    long minStayTime = 500;
    bool isModal = true;
    std::function<bool()> onConfirm;
    std::function<void()> onClose;
    std::function<bool()> onCancel;
};

class UIWindow : public WindowBase {
public:
    explicit UIWindow(const UIWindowParams& par);
    ~UIWindow() override = default;

    // opengl drawing
    void         initGL() const;
    bool         draw(double time, double dt, int ctxNr) override;
    virtual void drawNodeTree();

    void copyToScreen() const;
    void update();
    void iterate() const;

    // window changes
    void onSetViewport(int x, int y, int width, int height) override;
    void setMonitorMaxArea(int x, int y, int w, int h);
    void getActualMonitorMaxArea(int win_xpos, int win_ypos);

#ifdef _WIN32
    std::string OpenFileDialog(std::vector<COMDLG_FILTERSPEC>& allowedSuffix) const;
    std::string SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes) const;
#elif defined(__linux__) && !defined(__ANDROID__)
    std::string OpenFileDialog(std::vector<const char*>& allowedSuffix) {
        return OpenFileDialog(allowedSuffix);
    }

    std::string SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes) const {
        return SaveFileDialog(fileTypes);
    }
#elif __APPLE__
    std::string OpenFileDialog(std::vector<const char*>& allowedSuffix) { return OpenFileDialog(allowedSuffix); }
    std::string SaveFileDialog(std::vector<std::pair<std::string, std::string>> fileTypes) const {
        return SaveFileDialog(fileTypes);
    }
#endif

    // non-synchronized HID callbacks, called from the glfw event loop
    virtual void key_callback(int key, int scancode, int action, int mods);
    virtual void char_callback(unsigned int codepoint);
    virtual void cursor_callback(double xpos, double ypos);
    virtual void mouseBut_callback(int button, int action, int mods);
    virtual void scroll_callback(double xoffset, double yoffset);
    virtual void window_pos_callback(int xpos, int ypos);
    virtual void window_focus_callback(int focused);
    virtual void window_maximize_callback(int maximized);
    virtual void window_minimize_callback(int minimized);
    virtual void window_size_callback(int width, int height);
    virtual void window_close_callback();
    virtual void window_refresh_callback();

    // gl synchronized keyboard and mouse interaction
    void onKeyDown(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) override;
    void onKeyUp(int keyNum, bool shiftReleased, bool ctrlReleased, bool altReleased) override;
    void onChar(unsigned int codepoint) override;
    void onMouseDownLeft(float _xPos, float _yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) override;
    void onMouseDownLeftNoDrag(float _xPos, float _yPos) override {}
    void onMouseUpLeft() override;
    void onMouseDownRight(float _xPos, float _yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) override;
    void onMouseUpRight() override;
    void onMouseMove(float _xPos, float _yPos, ushort _mode) override;
    void onWheel(float _deg) override;
    void onResizeDone() override {}

    virtual void procInputFocus();
    virtual void onNodeRemove(UINode* node);
    virtual int32_t getObjAtPos(float x, float y, hidEvent evt);

    void fillHidData(hidEvent evt, float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed);

    // ---------- global callbacks that are not related to a UINode
protected:
    std::unordered_map<void*, std::function<void(hidData*)>>           m_globalMouseDownLeftCb;
    std::unordered_map<void*, std::function<void(hidData*)>>           m_globalMouseDownRightCb;
    std::unordered_map<void*, std::function<void(hidData*)>>           m_globalMouseUpLeftCb;
    std::unordered_map<void*, std::function<void(int, int, int, int)>> m_globalSetViewportCb;
    std::unordered_map<void*, std::function<void(int, int)>>           m_globalWinPosCb;
    std::unordered_map<void*, std::function<void(hidData*)>>           m_globalMouseMoveCb;
    std::unordered_map<void*, std::function<void(hidData*)>>           m_globalKeyDownCb;
    std::unordered_map<void*, std::function<void(hidData*)>>           m_globalKeyUpCb;

public:
    void addGlobalWinPosCb(void* ptr, std::function<void(int, int)> f);
    void removeGlobalWinPosCb(void* ptr);
    void addGlobalSetViewportCb(void* ptr, std::function<void(int, int, int, int)> f);
    void removeGlobalSetViewportCb(void* ptr);
    void addGlobalMouseDownLeftCb(void* ptr, std::function<void(hidData*)> f);
    void removeGlobalMouseDownLeftCb(void* ptr);
    void addGlobalMouseUpLeftCb(void* ptr, std::function<void(hidData*)> f);
    void removeGlobalMouseUpLeftCb(void* ptr);
    void addGlobalMouseDownRightCb(void* ptr, std::function<void(hidData*)> f);
    void removeGlobalMouseDownRightCb(void* ptr);
    void addGlobalMouseMoveCb(void* ptr, std::function<void(hidData*)> f);
    void removeGlobalMouseMoveCb(void* ptr);
    void addGlobalKeyDownCb(void* ptr, std::function<void(hidData*)> f);
    void removeGlobalKeyDownCb(void* ptr);
    void addGlobalKeyUpCb(void* ptr, std::function<void(hidData*)> f);
    void removeGlobalKeyUpCb(void* ptr);

    // ------------------------------------------------------------------------------------------

    void setAltPressed(bool val) { m_hidData.altPressed = val; }
    void setCtrlPressed(bool val) { m_hidData.ctrlPressed = val; }
    void setShiftPressed(bool val) { m_hidData.shiftPressed = val; }
    void setEnableMenuBar(bool val);
    void setEnableWindowResizeHandles(bool val);
    void setEnableMinMaxButtons(bool val) { m_menuBar->setEnableMinMaxButtons(val); }

    std::map<winProcStep, ProcStep>* getProcSteps() { return &m_procSteps; }
    ShaderCollector*                 getShaderCollector() { return &s_shCol; }
    static ShaderProto*              getShaderProto(uint ind, std::string& name) { return nullptr; };
    UINode*                          getRootNode() { return (m_menuBarEnabled && m_contentRoot) ? m_contentRoot : m_uiRoot.get(); };
    MenuBar*                         getMenuBar() { return m_menuBarEnabled ? m_menuBar : nullptr; };
    std::unordered_map<uiColors, glm::vec4>& getColors() { return m_colors; };
    glm::vec4&                       getColor(uiColors colName) { return m_colors[colName]; };
    UIApplication*                   getApplicationHandle() { return m_appHandle; };
    float                            getPixelRatio() { return s_devicePixelRatio; };
    bool                             isCursorVisible() const { return m_cursorVisible; };
    UISharedRes*                     getSharedRes() { return &m_sharedRes; }
    UINode*                          getLastHoverFound() { return m_lastHoverFound; }
    void                             setLastHoverFound(UINode* node) { m_lastHoverFound = node; }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    GLWindow*   getWinHandle() const { return m_winHandle; }
    void        makeCurrent() const { if (m_winHandle) m_winHandle->makeCurrent(); }
    bool        isOpen() const { return m_winHandle->isOpen(); }
    bool        isModal() const { return m_isModal; }
    void        pollEvents() { if (m_winHandle) ara::GLFWWindow::pollEvents(); }
#else
    void* getWinHandle() { return m_winHandle; }
#endif

#if defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
    unsigned int getWidthReal() { return s_viewPort.z; }
    unsigned int getHeightReal() { return s_viewPort.w; }
#elif defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    unsigned int getWidthReal() const { return m_winHandle ? m_winHandle->getWidthReal() : 0; }
    unsigned int getHeightReal() const { return m_winHandle ? m_winHandle->getHeightReal() : 0; }
#endif

#if !defined(ARA_USE_GLFW) && !defined(ARA_USE_EGL) || (defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP))
    glm::ivec2   getSize() { return glm::ivec2(s_windowViewport.z, s_windowViewport.w); }
    glm::ivec2   getPosition() { return glm::ivec2{0}; }
    unsigned int getWidth() { return (int)s_windowViewport.z; }
    unsigned int getHeight() { return (int)s_windowViewport.w; }
#else
    // just forwarding for convenience
    glm::ivec2   getSize() const { return m_winHandle ? m_winHandle->getSize() : glm::ivec2{0}; }
    glm::ivec2   getPosition() const { return m_winHandle ? m_winHandle->getPosition() : glm::ivec2{0}; }
    unsigned int getWidth() const { return m_winHandle ? m_winHandle->getWidth() : 0; }
    unsigned int getHeight() const { return m_winHandle ? m_winHandle->getHeight() : 0; }
    void         setPosition(int x, int y) const { m_winHandle->setPosition(x, y); }
    void         setSize(int width, int height) const { m_winHandle->setSize(width, height); }
#endif
    virtual void setBlockHid(bool val);
    virtual void setModal(bool val);
    virtual void setAppIcon(std::string& path);
    virtual void setMouseCursorVisible(bool val);

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    void swap() { m_winHandle->swap(); }
    void addKeyCb(const std::function<void(int, int, int, int)>& f) const { m_winHandle->addKeyCb(f); }
    void setCharCb(const std::function<void(int)>& f) const { m_winHandle->setCharCb(f); }
    void setMouseButtonCb(const std::function<void(int, int, int)>& f) const { m_winHandle->setMouseButtonCb(f); }
    void setMouseCursorCb(const std::function<void(double, double)>& f) const { m_winHandle->setMouseCursorCb(f); }
    void setWindowSizeCb(const std::function<void(int, int)>& f) const { m_winHandle->setWindowSizeCb(f); }
    void setScrollCb(const std::function<void(double, double)>& f) const { m_winHandle->setScrollCb(f); }
    void setWindowPosCb(const std::function<void(int, int)>& f) const { m_winHandle->setWindowPosCb(f); }
    void setWindowMaximizeCb(const std::function<void(int)>& f) const { m_winHandle->setWindowMaximizeCb(f); }
    void setWindowIconfifyCb(const std::function<void(int)>& f) const { m_winHandle->setWindowIconfifyCb(f); }
    void setWindowFocusCb(const std::function<void(int)>& f) const { m_winHandle->setWindowFocusCb(f); }
    void setCloseCb(const std::function<void()>& f) const { m_winHandle->setCloseCb(f); }
    void setWindowRefreshCb(const std::function<void()>& f) const { m_winHandle->setWindowRefreshCb(f); }
#endif
    void setBlockDraw(bool val) { m_blockDraw = val; }
    bool isBlockDraw() const { return m_blockDraw; }
    void setApplicationHandle(UIApplication* ptr) { m_appHandle = ptr; }
    void setToMainWindow() { m_isMainWindow = true; }
    void setWindowMinSize(int32_t x, int32_t y) { m_minWinSize.x = x; m_minWinSize.y = y; }
    void setCloseFunc(std::function<void(UIWindow*)> f) {
        m_closeFunc = std::move(f);
    }
    std::function<void(UIWindow*)>& getCloseFunc() { return m_closeFunc; }

    void         addWinCb(const std::function<void()>& f) { m_winProcCb.emplace_back(f); }
    virtual void open();
    virtual void close(bool direct = false);
    virtual bool closeEvtLoopCb();
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    virtual void hide() { m_winHandle->hide(); }
    virtual void startRenderLoop();
    virtual void stopRenderLoop();
    Conditional* getGlInitedSema() { return m_winHandle->getGlInitedSema(); }
    bool         isInited() { return m_winHandle != nullptr && m_winHandle->isInited(); }
    void         focus() { if (m_winHandle) m_winHandle->focus();}
#else
    virtual void hide() {}
    void         setExtWinHnd(void* hnd) { m_winHandle = hnd; }
#endif
    bool         isMousePressed() const { return m_hidData.mousePressed; }
    void         setResChanged(bool val);

    bool                       resChanged() const { return m_resChanged; }
    void                       showObjMap(bool val) { m_showObjMap = val; }
    bool                       objMapVisible() const { return m_showObjMap; }
    UINode*                    getInputFocusNode() { return m_inputFocusNode; }
    UINode*                    getDraggingNode() { return m_draggingNode; }
    void                       resetDraggingNode() { m_draggingNode = nullptr; }
    void                       blockWindowResizeCb(bool val) { m_blockWindowResizeCb = val; }
    std::function<glm::vec2()> extGetWinOffs() { return m_extGetWinOffs; }
    void                       setExtGetWinOffs(std::function<glm::vec2()> f) { m_extGetWinOffs = std::move(f); }
    void                       forceDpiScale(float val) { m_forceDpiScale = val; }
    glm::vec2&                 getActMousePos() { return m_hidData.mousePos; }
    FBO*                       getSceneFbo() { return m_sceneFbo.get(); }
    void                       setInputFocusNode(UINode* node, bool procLostFocus = true);
    void                       addGlCb(void* cbName, const std::string& fName, const std::function<bool()>& func);
    void                       removeGlCbs(void* cbName) { WindowBase::eraseGlCb(cbName); }

    // UI Elements
    std::unique_ptr<UINode>               m_uiRoot;
    std::unique_ptr<ObjectMapInteraction> m_objSel;
    cpEditMode                            m_cp_editM = cpEditMode::Move;

protected:
    GLBase*            m_glbase            = nullptr;
    UINode*            m_lastHoverFound    = nullptr;
    UINode*            m_draggingNode      = nullptr;
    UINode*            m_contentRoot       = nullptr;
    UINode*            m_inputFocusNode    = nullptr;
    MenuBar*           m_menuBar           = nullptr;
    WindowResizeAreas* m_windowResizeAreas = nullptr;
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    GLWindow* m_winHandle = nullptr;
#else
    void* m_winHandle = nullptr;
#endif
    std::unique_ptr<FBO> m_sceneFbo;

    UISharedRes                  m_sharedRes;
    std::unique_ptr<DrawManager> m_drawMan;
    UIApplication*               m_appHandle = nullptr;
    UINode::ObjPosIt             m_opi;

    StopWatch m_watch;

    std::unique_ptr<Quad>                                     m_quad;
    std::unique_ptr<Quad>                                     m_normQuad;
    Shaders*                                                  m_fboDrawShdr = nullptr;
    Shaders*                                                  m_stdTex      = nullptr;
    Shaders*                                                  m_debugShdr   = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_texCol;
    std::unordered_map<uiColors, glm::vec4>                   m_colors;
    Shaders*                                                  m_stdTexMulti = nullptr;

    // Pseudo Event Based Structure
    std::map<winProcStep, ProcStep> m_procSteps;

    glm::ivec4 monitorMaxArea{0};
    glm::ivec2 m_minWinSize = {300, 300};
    glm::vec2  m_mouseClickPos{0.f};
    glm::vec2  m_lastClickedPos = glm::vec2{0};

    float m_stdPadding    = 5.f;
    float m_forceDpiScale = -1.f;

    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    bool m_blockDraw           = false;
    bool m_blockHID            = false;
    bool m_blockWindowResizeCb = false;
    bool m_cursorVisible       = true;
    bool m_doSwap              = false;
    bool m_debugGLFWwin        = true;
    bool m_isMainWindow        = false;
    bool m_isModal             = false;
    bool m_multisample;
    bool m_menuBarEnabled             = true;
    bool m_resChanged                 = false;
    bool m_removeFromApp              = false;
    bool m_showObjMap                 = false;
    bool m_selfManagedCtx             = false;
    bool m_windowResizeHandlesEnabled = true;

    GLuint m_nullVao = 0;

    MouseIcon m_actMouseCursor = MouseIcon::arrow;
    hidData   m_hidData;

    std::filesystem::path              m_dataFolder;
    std::vector<std::function<void()>> m_winProcCb;
    std::function<void(UIWindow*)>     m_closeFunc;
    std::function<glm::vec2()>         m_extGetWinOffs;
    std::function<void()>              m_sceneFinishCb;

    std::chrono::time_point<std::chrono::system_clock> m_lastLeftMouseDown;

    std::list<UINode*> m_draggingNodeTree;
};

}  // namespace ara
