#include "GLRenderer.h"

#include <GLBase.h>
#include <GeoPrimitives/Quad.h>

using namespace glm;
using namespace std;

namespace ara {
/** init the gl context, create a new window, disable the context after
   creation. return true if the context was created and false if it already
   existed */
bool GLRenderer::init(std::string name, glm::ivec2 pos, glm::ivec2 dimension, bool hidden) {
#ifdef ARA_USE_GLFW
    bool restartGlBaseLoop = false;

    // check if GLBase renderloop is running, if this is the case, stop it and
    // start it later again. Otherwise context sharing will fail
    if (m_glbase && m_glbase->isRunning()) {
        restartGlBaseLoop = true;
        m_glbase->stopRenderLoop();
    }

    m_dim        = glm::vec2(dimension);
    m_pos        = glm::vec2(pos);
    m_name       = name;
    m_fontHeight = 45;

    // add a new window through the GWindowManager
    glWinPar wp;
    wp.width = static_cast<int>(m_dim.x);
    wp.height = static_cast<int>(m_dim.y);
    wp.shiftX = static_cast<int>(m_pos.x);
    wp.shiftY = static_cast<int>(m_pos.y);
    wp.createHidden = hidden;
    wp.scaleToMonitor = true;
    wp.shareCont = static_cast<void *>(m_glbase->getGlfwHnd());

    m_winHandle = m_glbase->getWinMan()->addWin(wp);
    if (!m_winHandle) {
        return false;
    }

    m_winHandle->makeCurrent();

    initGLEW();

    if (!s_inited) {
        WindowBase::init(0, 0, dimension.x, dimension.y);
        initGL();

        glfwMakeContextCurrent(nullptr);
        m_winHandle->startDrawThread(std::bind(&GLRenderer::draw, this, std::placeholders::_1, std::placeholders::_2,
                                               std::placeholders::_3));  // window context will be made current
                                                                         // within the thread, so release it before
        s_inited = true;
    }

    if (restartGlBaseLoop && m_glbase) {
        if (!s_inited) glfwMakeContextCurrent(nullptr);
        m_glbase->startRenderLoop();
        // no context bound at this point
    }
#endif
    return true;
}

#ifdef ARA_USE_GLFW

void GLRenderer::initFromWinMan(std::string name, GLFWWindow *win) {
    m_dim       = glm::vec2(win->getWidth(), win->getHeight());
    m_pos       = glm::vec2(win->getPosition());
    m_winHandle = win;
    m_name      = name;
    s_inited    = true;
}

#endif

void GLRenderer::initGL() {
    // m_watch.setStart();
    // LOG << "  GLRenderer::initGL " << this << " glCtx.ctx " << (void*)
    // wglGetCurrentContext();

    glLineWidth(1.f);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);  // bei der implementation von nvidia gibt es nur
                               // LineWidth 0 -1 ...
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_stdCol = s_shCol.getStdCol();
    m_stdTex = s_shCol.getStdTex();
    // s_testPicShdr = initTestPicShdr();

    m_stdQuad = make_unique<Quad>(-1.f, -1.f, 2.f, 2.f, vec3(0.f, 0.f, 1.f), 0.f, 0.f, 0.f, 1.f);
    m_flipQuad =
        make_unique<Quad>(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f, nullptr, 1, true);
    // s_dataPath = dataPath;
    glGenVertexArrays(1, &m_nullVao);

    m_vwfShader         = initVwfRenderShdr();
    m_flatWarpShader    = initFlatWarpShdr();
    m_exportShader      = initExportShdr();
    m_exportBlendShader = initExportBlendShdr();

    glFinish();  // glbase renderloop may not be running, so be sure the command
                 // queue is executed

    // m_watch.setEnd();
    // m_watch.print("GLRenderer init ", true);

    s_inited = true;
}

Shaders *GLRenderer::initVwfRenderShdr() {
    string vert = s_shCol.getShaderHeader();
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n out vec2 tex_coord; \n void
                                                                                                     main() {
                \n tex_coord = texCoord;
                \n tex_coord.y = 1.0 - tex_coord.y;
                \n gl_Position = position;
                \n
            });

    string frag = s_shCol.getShaderHeader();
    frag += STRINGIFY(layout(location = 0) out vec4 fragColor; \n uniform sampler2D samContent; \n uniform sampler2D samWarp; \n uniform sampler2D samBlend; \n uniform sampler2D mask; \n uniform uint bBorder; \n uniform uint bDoNotBlend; \n uniform uint bFlipWarpH; \n uniform uint useMask; \n uniform vec3 colorCorr; \n uniform vec3 gradient; \n uniform vec3 gamma; \n uniform vec3 gammaP; \n uniform vec3 plateau; \n in vec2 tex_coord; \n void
                          main()\n {
                              \n vec4 blend;
                              vec4    tex = texture(samWarp, tex_coord);
                              \n

                                  tex.y = bool(bFlipWarpH) ? 1.0 - tex.y : tex.y;
                              \n  tex.x *= bool(bBorder) ? 1.02 : 1.0;
                              \n  tex.x -= bool(bBorder) ? 0.01 : 0.0;
                              \n  tex.y *= bool(bBorder) ? 1.02 : 1.0;
                              \n  tex.y -= bool(bBorder) ? 0.01 : 0.0;
                              \n

                                  blend = bool(bDoNotBlend) ? vec4(1.0) : texture(samBlend, tex_coord);
                              \n  tex.xy /= bool(bDoNotBlend) ? 1.0 : blend.a;
                              \n

                                  vec4 outV =
                                      texture(samContent, tex.xy) * tex.b *
                                      (bool(useMask) ? 1.0 - texture(mask, vec2(tex_coord.x, 1.0 - tex_coord.y)).a
                                                     : 1.0);
                              \n outV.rgb *= colorCorr;

                              fragColor = outV;
                              \n

                                  // blending
                                      blend.rgb = pow(blend.rgb, gamma);
                              \n vec3 f1;
                              \n for (uint i = 0; i < 3; i++)                                                            \n {
                                  if (blend[i] < 0.5)
                                  \n f1[i] = plateau[i] * pow(2.0 * blend[i], gradient[i]);
                                  \n else \n f1[i] =
                                      1.0 - (1.0 - plateau[i]) * pow(2.0 * (1.0 - blend[i]), gradient[i]);
                                  \n
                              }
                              \n blend.rgb = pow(f1, gammaP);
                              \n

                                  fragColor.rgb *= bool(bDoNotBlend) ? vec3(1.0) : blend.rgb;
                              \n
                          });

    return s_shCol.add("VWBRenderShdr", vert.c_str(), frag.c_str());
}

Shaders *GLRenderer::initFlatWarpShdr() {
    string vert = s_shCol.getShaderHeader();
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n out vec2 tex_coord; \n void
                                                                                                     main() {
                \n tex_coord = texCoord;
                \n gl_Position = position;
                \n
            });

    string frag = s_shCol.getShaderHeader();
    frag += STRINGIFY(
                layout(location = 0) out vec4 fragColor; \n
                uniform sampler2D warpProjector; \n
                uniform sampler2D warpCalib; \n
                uniform sampler2D warpCanvas; \n
                uniform int useCalib;\n
                uniform int useProjWarp;\n
                //uniform uint bFlipWarpH; \n
                in vec2 tex_coord; \n
                void main() { \n
                vec4 proj = bool(useProjWarp) ? texture(warpProjector, vec2(tex_coord.x, 1.0 - tex_coord.y)): vec4(tex_coord, 1.0, 1.0);   \n
                vec4 calib = bool(useCalib) ? texture(warpCalib, proj.xy) : proj;                \n
                fragColor = texture(warpCanvas, vec2(calib.x, 1.0 - calib.y)) * calib.b;            \n
        });

    return s_shCol.add("FlatWarpRenderShdr", vert.c_str(), frag.c_str());
}

Shaders *GLRenderer::initExportShdr() {
    string vert = s_shCol.getShaderHeader();
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n out vec2 tex_coord; \n void
                                                                                                     main() {
                \n tex_coord = texCoord;
                \n tex_coord.y = tex_coord.y;
                \n gl_Position = position;
                \n
            });

    string frag = s_shCol.getShaderHeader();
    frag += STRINGIFY(
        layout(location = 0) out vec4 fragColor; \n layout(location = 1) out vec4 blending; \n uniform sampler2D warpToolUV; \n uniform sampler2D samWarp; \n uniform uint bBorder; \n uniform uint useWarpToolUV; \n in vec2 tex_coord; \n void
            main()\n {
                \n vec4 tex = texture(samWarp, vec2(tex_coord.x, 1.0 - tex_coord.y));
                \n  // vwf warp texture
                    tex.y = 1.0 - tex.y;
                \n if (bool(bBorder))                                                        \n {
                    \n tex.x *= 1.02;
                    \n tex.x -= 0.01;
                    \n tex.y *= 1.02;
                    \n tex.y -= 0.01;
                    \n
                }
                \n vec4 resCol = useWarpToolUV == 1 ? texture(warpToolUV,
                                                              tex.xy)            \n  // read from the output of
                                                                                     // the warping tool or take
                                                                                     // the tex_coord as is
                                                    : vec4(tex.x, 1.0 - tex.y, tex.zw);
                \n      resCol *= tex.b;
                \n      fragColor = vec4(resCol.rg, float((resCol.r + resCol.g) > 0.0), 1.0);
                \n
            });

    return s_shCol.add("VWBExportShdr", vert.c_str(), frag.c_str());
}

Shaders *GLRenderer::initExportBlendShdr() {
    string vert = s_shCol.getShaderHeader();
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n out vec2 tex_coord; \n void
                                                                                                     main() {
                \n tex_coord = texCoord;
                \n tex_coord.y = tex_coord.y;
                \n gl_Position = position;
                \n
            });

    string frag = s_shCol.getShaderHeader();
    frag += STRINGIFY(
        layout(location = 0) out vec4 blending; \n uniform sampler2D samBlend; \n uniform vec3 gradient; \n uniform vec3 gamma; \n uniform vec3 gammaP; \n uniform vec3 plateau; \n in vec2 tex_coord; \n void
            main()\n {
                \n vec4 blend     = texture(samBlend, tex_coord);
                \n      blend.rgb = pow(blend.rgb, gamma);
                \n vec3 f1;
                \n for (uint i = 0; i < 3; i++) {
                    \n if (blend[i] < 0.5)                                                                    \n f1[i] = plateau[i] * pow(2.0 * blend[i], gradient[i]);
                    \n else \n f1[i] = 1.0 - (1.0 - plateau[i]) * pow(2.0 * (1.0 - blend[i]), gradient[i]);
                    \n
                }
                \n blend.rgb = pow(f1, gammaP);
                \n blending  = blend;
                \n
            });

    return s_shCol.add("VWBExportBlendShdr", vert.c_str(), frag.c_str());
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

void GLRenderer::loadTypo(filesystem::path typoPath) {
    if (!m_typo && s_inited) {
        m_typo = make_unique<TypoGlyphMap>((uint)m_dim.x, (uint)m_dim.y);
        // m_typo->setFontHeight(45);
        m_typo->loadFont((m_dataPath / typoPath).string().c_str(), &s_shCol);
    }
}

bool GLRenderer::draw(double time, double dt, int ctxNr) {
    // std::unique_lock<std::mutex> l( *m_glbase->glMtx() );
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

    // m_drawWatch.setStart();
    // m_drawWatch.setEnd();
    // m_drawWatch.print(" draw fps:");

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
    if (!s_inited) return;

    if (!direct)
        m_glbase->runOnMainThread([this] {
            closeEvtLoopCb();
            return true;
        });
    else
        closeEvtLoopCb();
}

bool GLRenderer::closeEvtLoopCb() {
#ifdef ARA_USE_GLFW

    // LOG << "GLRenderer::closeEvtLoopCb";

    // check if the window was already closed;
    if (!s_inited) return true;

    if (m_winHandle) m_winHandle->stopDrawThread();  // also calls close() on GLFWWindow

    if (m_glbase) {
        // remove all gl resources, since all gl contexts are shared, this can
        // be done on any context
        m_glbase->addGlCbSync([this]() {
            m_stdQuad.reset();
            m_flipQuad.reset();
            s_shCol.clear();
            glDeleteVertexArrays(1, &m_nullVao);
            return true;
        });

        clearGlCbQueue();

        // if this was called from key event callback, it will cause a crash,
        // because we are still iterating through GLFWWindow member variables
        m_glbase->getWinMan()->removeWin(m_winHandle,
                                         false);  // removes the window from the ui_windows array
    }

    s_inited = false;
#endif

    return true;
}

}  // namespace ara
