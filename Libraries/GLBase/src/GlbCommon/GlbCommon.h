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

/**
 * \brief common headers for the ara namespace
 *  includes for glew and glm
 *  macros for the basically geometrical constants (M_PI, etc.)
 *
 */

#pragma once

#ifdef _WIN32
#include "Windows/WindowsMousePos.h"
#elif __linux__
#include <X11/X11MousePos.h>
#elif __APPLE__
#include <OsX/OSXMousePos.h>
#endif

#include <util_common.h>
#include <GlbCommon/GlbKeyDefines.h>
#include <GlbCommon/GlbConstants.h>

#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <utility>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#ifdef __APPLE__
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#endif
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "GlbCommon/GlHeaders.h"

// -------------------- SCENE CONSTANTS ----------------------------

#define MAX_NUM_COL_SCENE 4
#define MAX_SEP_REC_BUFS 4
#define MAX_VERTEX_ATTRIBUTE 16  // Maximum number of attributes per vertex

// -------------------- MACROS ----------------------------

#ifndef __GNUC__
#define access _access
#define F_OK 0 /* Test for existence.  */
#endif

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

#define aisgl_min(x, y) (x < y ? x : y)
#define aisgl_max(x, y) (y > x ? y : x)

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef STRINGIFY
#define STRINGIFY(A) #A
#endif

#ifndef ushort
typedef unsigned short ushort;
#endif

#ifndef uint
typedef uint32_t uint;
#endif

namespace ara {

enum class CoordType : int { Position = 0, Normal, TexCoord, Color, TexCoordMod, Velocity, Aux0, Aux1, Aux2, Aux3, ModMatr, Count };
enum class align : int { center = 0, left, right, justify, justify_ex };
enum class valign : int { center = 0, top, bottom };
enum class MouseIcon : int { arrow = 0, move, scale, rotate, ibeam, hresize, vresize, count };
enum class WinMouseIcon : int { arrow = 0, hresize, vresize, lbtrResize, ltbrResize, crossHair, move, rotate, scale, count };
enum class mouseButt : int { left = 0, middle, right };
enum class mouseDragType : int { none = 0, translating, rotating, rolling, zooming, snapToAxis };

static std::map<std::string, MouseIcon> stringToMouseIcon = {
    {"arrow", MouseIcon::arrow},    {"move", MouseIcon::move},   {"scale", MouseIcon::scale},
    {"rotate", MouseIcon::rotate},  {"ibeam", MouseIcon::ibeam}, {"hresize", MouseIcon::hresize},
    {"vresize", MouseIcon::vresize}};

enum class stdMatNameInd : int { ModelMat = 0, CamModelMat, ViewMat, ProjectionMat, NormalMat };
enum class texCordGenType : int { PlaneProjection = 0 };
enum class unitType : int { Pixels = 0, Percent };
enum class coordComp : int { x = 0, y };
enum class extrapolM : int { Mirror = 0, Circle };
enum class interpolM : int { Bilinear = 0, CatmullRomCentri, Bezier };
enum class maskType : int { Vector = 0, Bitmap, Count };
enum class cfState : int { fine = 0, normal, coarse };
enum class hidEvent : int { KeyDown = 0, KeyUp, onChar, MouseDownLeft, MouseDownLeftNoDrag, MouseUpLeft, MouseDownRight, MouseUpRight, MouseMove, MouseDrag, MouseWheel, OnResize, SetViewport, Size };

enum class cpEditMode : int32_t {
    L2Move = 0,
    L2CpMove,
    L2CpScale,
    L2CpKeybMove,
    CpKeybMove,
    L2LineMove,
    L2LineKeybMove,
    L1Move,
    L1CpMove,
    L1CpScale,
    L1RotX,
    L1RotY,
    L1RotZ,
    L1TranZ,
    Move,
    AddCtrlPoint,
    DelCtrlPoint,
    ChangeInterp,  // order matters
    DrawShape,
    Count
};

enum class glFun : int { Enable = 0, Disable, BlendEquation, BlendFunc, PolygonMode };
enum class changeWinEvent : int { OnResize, SetViewport, Maximize, Minimize, SetWinPos };
enum class stereoEye : int { left = 0, right = 1, count = 2 };
enum class winCb : int { Key = 0, Char, MouseButton, CursorPos, WindowSize, WindowClose, WindowMaximize, WindowIconify, WindowFocus, WindowPos, Scroll, WindowRefresh, CharMods, CursorEnter, Drop, Fbsize, Scale, Size };

static inline std::unordered_map<winCb, std::string> winCbNames {
    { winCb::Key, "Key" },
    { winCb::Char, "Char" },
    { winCb::MouseButton, "MouseButton" },
    { winCb::CursorPos, "CursorPos" },
    { winCb::WindowSize, "WindowSize" },
    { winCb::WindowClose, "WindowClose" },
    { winCb::WindowMaximize, "WindowMaximize" },
    { winCb::WindowIconify, "WindowIconify" },
    { winCb::WindowFocus, "WindowFocus" },
    { winCb::WindowPos, "WindowPos" },
    { winCb::Scroll, "Scroll" },
    { winCb::WindowRefresh, "WindowRefresh" },
    { winCb::CharMods, "CharMods" },
    { winCb::CursorEnter, "CursorEnter" },
    { winCb::Drop, "Drop" },
    { winCb::Fbsize, "Fbsize" },
    { winCb::Scale, "Scale" },
};

// process steps, order matters!!!, these are for fixed functionality that is used frequently
enum winProcStep : int { Callbacks = 0, Tesselate, L1Lines, Select, ClearSel, Draw, Update, Count };

class custVec3 {
public:
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

// shortcut for strongly typed enums to be cast to their underlying type
template <typename E>
constexpr typename std::underlying_type<E>::type toType(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

class scissorStack {
public:
    std::vector<glm::vec4> stack;
    glm::vec4              active = glm::vec4{};
};

class ProcStep {
public:
    bool                  active = false;
    std::function<void()> func   = nullptr;
};

class UINode;

/// <summary>
/// data for mouse hovering callback
/// </summary>
class hidData {
public:
    int         objId         = -1;
    int         clickedObjId  = -1;
    int         releasedObjId = 0;
    MouseIcon   actIcon       = MouseIcon::arrow;
    cpEditMode* cp_editM      = nullptr;

    glm::ivec2 screenMousePos{};     ///> in virtual pixels, relative to screen (not window)
    glm::vec2  mousePos{};           ///> window relative mouse position in virtual
    /// pixels (origin top,left)
    glm::vec2 mousePosFlipY{};       ///> window relative mouse position in hardware pixels (origin
    /// bottom,left) for use with opengl
    glm::vec2 mousePosNorm{};        ///> normalized window relative mouse position
    /// in virtual pixels (origin top,left)
    glm::vec2 mousePosNormFlipY{};   ///> normalized window relative mouse position in virtual pixels
    ///(origin bottom,left) for use with opengl
    glm::vec2 mousePosNodeRel{};     ///> in virtual pixels
    glm::vec2 movedPix{};            ///> in virtual pixels, dpi dependent
    glm::vec2 mouseClickPosFlipY{};  ///> onClick normalized window relative mouse position in virtual
    /// pixels (origin bottom,left) for use with opengl

    bool dragStart         = false;
    bool dragging          = false;
    bool altPressed        = false;
    bool ctrlPressed       = false;
    bool shiftPressed      = false;
    bool mousePressed      = false;
    bool mouseRightPressed = false;
    bool isDoubleClick     = false;
    bool hit               = false;
    bool consumed          = false;
    bool breakCbIt         = false;

    std::map<winProcStep, ProcStep>* procSteps = nullptr;
    void*                            newNode   = nullptr;  // used in WindowResizeAreas for avoiding
    // unnecessary mouse icon changes
    std::unordered_map<hidEvent, UINode*> hitNode = {
        { hidEvent::MouseMove, nullptr },
        { hidEvent::MouseWheel, nullptr },
        { hidEvent::MouseDownLeft, nullptr },
        { hidEvent::MouseUpLeft, nullptr },
        { hidEvent::MouseDownRight, nullptr },
        { hidEvent::MouseUpRight, nullptr },
        { hidEvent::MouseDrag, nullptr },
        { hidEvent::MouseWheel, nullptr }
    };

    int          key       = 0;  // key callback
    unsigned int codepoint = 0;  // char callback
    float        degrees   = 0;  // scroll wheel

    void reset() {
        consumed  = false;
        breakCbIt = false;
    }
    void getScreenMousePos(float devicePixelRatio) {
        mouse::getAbsMousePos(screenMousePos.x, screenMousePos.y);
        screenMousePos = static_cast<glm::ivec2>(static_cast<glm::vec2>(screenMousePos) / devicePixelRatio);
    }
};

// second of pair indicates whether the callback should only be called when the element was really clicked
// (true => data->hit == true) or if the callback should be called in any case
typedef std::pair<std::function<void(hidData&)>, bool> mouseCb;

class MouseButtState {
public:
    bool dragStart = false;
    bool dragging  = false;
    bool pressed   = false;
};

// -------------------- STRUCTS ----------------------------

class auxTexPar {
public:
    int         unitNr = 0;
    int         texNr  = 0;
    GLenum      target = 0;
    std::string name;
};

class DisplayBasicInfo {
public:
    uint32_t offsX  = 0;
    uint32_t offsY  = 0;
    uint32_t width  = 0;
    uint32_t height = 0;
};

struct DrawArraysIndirectCommand {
    uint count = 0;
    uint primCount = 0;
    uint first = 0;
    uint baseInstance = 0;
};

class GLBase;

struct FboInitParams {
    GLBase *glbase = nullptr;
    int width = 0;
    int height = 0;
    int depth = 1;
    GLenum type = GL_RGBA8;
    GLenum target = GL_TEXTURE_2D;
    bool depthBuf{};
    int nrAttachments = 1;
    int mipMapLevels = 1;
    int nrSamples = 1;
    GLenum wrapMode = GL_REPEAT;
    bool layered = false;
};

// -----------------------------------------------

glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);

float perlinOct1D(float x, int octaves, float persistence);

glm::vec4 linInterpolVec4(float inInd, std::vector<glm::vec4> *array);

std::pair<bool, glm::vec2> projPointToLine(glm::vec2 point, glm::vec2 l1Start, glm::vec2 l1End);

std::pair<bool, glm::vec2> lineIntersect(glm::vec2 l1Start, glm::vec2 l1End, glm::vec2 l2Start, glm::vec2 l2End);
bool lineIntersectCheckOutlier(std::array<std::array<glm::vec2, 2>, 2>& linePoints, glm::vec2& intersection);

void catmullRom(std::vector<glm::vec2> &inPoints, std::vector<glm::vec2> &outPoints, uint32_t dstNrPoints);

float distPointLine(glm::vec2 _point, glm::vec2 _lineP1, glm::vec2 _lineP2, bool *projIsOutside = nullptr,
                    float *projAngle = nullptr);

float sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3);
bool  pointInTriangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3);
float angleBetweenVectors(const glm::vec3& a, const glm::vec3& b);
glm::vec3 getRandomPointOnPlane(const glm::vec3&, const glm::vec3&, const glm::vec3&);

GLenum postGLError(bool silence = false);
GLenum getExtType(GLenum inType);
GLenum getPixelType(GLenum inType);
short  getNrColChans(GLenum internalType);
uint   getBitCount(GLenum inType);
void   decomposeMtx(const glm::mat4 &m, glm::vec3 &pos, glm::quat &rot, glm::vec3 &scale);
void   decomposeRot(const glm::mat4 &m, glm::quat &rot);

// static inline GLuint restartIndex=0xFFFF;

static inline std::vector<std::string> m_stdAttribNames{ "position", "normal", "texCoord", "color", "texCorMod", "velocity", "aux0", "aux1", "aux2", "aux3", "modMatr"};
static inline std::vector<std::string> m_stdRecAttribNames{"rec_position",  "normal",       "rec_texCoord", "rec_color",
                                                           "rec_texCorMod", "rec_velocity", "rec_aux0",     "rec_aux1",
                                                           "rec_aux2",      "rec_aux3",     "rec_modMatr"};
static inline std::vector<int>         m_coTypeStdSize{4, 3, 2, 4, 4, 4, 4, 4, 4, 4, 16};
static inline std::vector<int>         m_coTypeFragSize{4, 3, 2, 4, 4, 4, 4, 4, 4, 4, 16};
static inline std::vector<int>         m_recCoTypeFragSize{4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 16};
static inline std::vector<std::string> m_stdMatrixNames{"m_model", "m_cam_model", "m_view", "m_proj", "m_normal"};
static inline std::string m_stdPvmMult{"gl_Position = m_proj * m_view * m_cam_model * m_model * position; \n"};
static const inline std::array          stdQuadVertices{glm::vec2{0.f}, glm::vec2{1.f, 0.f}, glm::vec2{0.f, 1.f},
                                                              glm::vec2{1.f, 1.f}};
static inline constexpr std::array     stdQuadInd{0, 1, 3, 3, 2, 0};  // two triangles defining a m_quad

[[maybe_unused]] static const std::vector<std::string> &getStdAttribNames() { return m_stdAttribNames; }
[[maybe_unused]] static const std::vector<std::string> &getStdRecAttribNames() { return m_stdRecAttribNames; }
[[maybe_unused]] static const std::vector<int>         &getCoTypeStdSize() { return m_coTypeStdSize; }
[[maybe_unused]] static const std::vector<int>         &getCoTypeFragSize() { return m_coTypeFragSize; }
[[maybe_unused]] static const std::vector<int>         &getRecCoTypeFragSize() { return m_recCoTypeFragSize; }
[[maybe_unused]] static const std::vector<std::string> &getStdMatrixNames() { return m_stdMatrixNames; };
[[maybe_unused]] static const std::string              &getStdPvmMult() { return m_stdPvmMult; };

void glesGetTexImage(GLuint textureObj, GLenum target, GLenum format, GLenum pixelType, int width, int height,
                     GLubyte *pixels);

std::vector<glm::vec2> get2DRing(int nrPoints);

bool initGLEW();

static void printGLVersion() {
    LOG << "Vendor:   " << glGetString(GL_VENDOR) << "\n"
    << "Renderer: " << glGetString(GL_RENDERER) << "\n"
    << "Version:  " << glGetString(GL_VERSION) << "\n"
    << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION);
}

}  // namespace ara