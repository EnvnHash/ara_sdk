/**
 * \brief common headers for the mc3d namespace
 *  includes for glew and glm
 *  macros for the basically geometrical constants (M_PI, etc.)
 *
 */

#pragma once

#include <util_common.h>

#include <cstdint>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <unordered_map>
#include <utility>
#include <random>  // for random number generation

#include "glb_common/gl_headers.h"
#include "glb_common/matrix.h"

// -------------------- SCENE CONSTANTS ----------------------------

#define CHUNK_SIZE 1024
#define MAX_NUM_COL_SCENE 4
#define MAX_SEP_REC_BUFS 4
#define MAX_TEXTURE_MIPS 14
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

// -------------------- CONSTANTS ----------------------------

#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647693
#endif

#ifndef M_TWO_PI
#define M_TWO_PI 6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI 1.57079632679489661923
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI / 180.0)
#endif

#ifndef ONE_THIRD
#define ONE_THIRD (1.0f / 3.0f)
#endif

#ifndef FOUR_THIRDS
#define FOUR_THIRDS (4.0f / 3.0f)
#endif

#ifndef SEL_LAYER_COUNT
#define SEL_LAYER_COUNT 2
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// -------------------- KEYS ----------------------------

#define GLSG_KEY_UNKNOWN (-1)

#define GLSG_KEY_LEFT 18
#define GLSG_KEY_UP 19
#define GLSG_KEY_RIGHT 20
#define GLSG_KEY_DOWN 21

#define GLSG_KEY_SPACE 32
#define GLSG_KEY_APOSTROPHE 39 /* ' */
#define GLSG_KEY_COMMA 44      /* , */
#define GLSG_KEY_MINUS 45      /* - */
#define GLSG_KEY_PERIOD 46     /* . */
#define GLSG_KEY_SLASH 47      /* / */
#define GLSG_KEY_0 48
#define GLSG_KEY_1 49
#define GLSG_KEY_2 50
#define GLSG_KEY_3 51
#define GLSG_KEY_4 52
#define GLSG_KEY_5 53
#define GLSG_KEY_6 54
#define GLSG_KEY_7 55
#define GLSG_KEY_8 56
#define GLSG_KEY_9 57
#define GLSG_KEY_SEMICOLON 59 /* ; */
#define GLSG_KEY_EQUAL 61     /* = */
#define GLSG_KEY_A 65
#define GLSG_KEY_B 66
#define GLSG_KEY_C 67
#define GLSG_KEY_D 68
#define GLSG_KEY_E 69
#define GLSG_KEY_F 70
#define GLSG_KEY_G 71
#define GLSG_KEY_H 72
#define GLSG_KEY_I 73
#define GLSG_KEY_J 74
#define GLSG_KEY_K 75
#define GLSG_KEY_L 76
#define GLSG_KEY_M 77
#define GLSG_KEY_N 78
#define GLSG_KEY_O 79
#define GLSG_KEY_P 80
#define GLSG_KEY_Q 81
#define GLSG_KEY_R 82
#define GLSG_KEY_S 83
#define GLSG_KEY_T 84
#define GLSG_KEY_U 85
#define GLSG_KEY_V 86
#define GLSG_KEY_W 87
#define GLSG_KEY_X 88
#define GLSG_KEY_Y 89
#define GLSG_KEY_Z 90
#define GLSG_KEY_LEFT_BRACKET 91  /* [ */
#define GLSG_KEY_BACKSLASH 92     /* \ */
#define GLSG_KEY_RIGHT_BRACKET 93 /* ] */
#define GLSG_KEY_GRAVE_ACCENT 96  /* ` */
#define GLSG_KEY_WORLD_1 161      /* non-US #1 */
#define GLSG_KEY_WORLD_2 162      /* non-US #2 */

/* std::function keys */
#define GLSG_KEY_ESCAPE 256
#define GLSG_KEY_ENTER 257
#define GLSG_KEY_TAB 258
#define GLSG_KEY_BACKSPACE 259
#define GLSG_KEY_INSERT 260
#define GLSG_KEY_DELETE 261
#define GLSG_KEY_PAGE_UP 266
#define GLSG_KEY_PAGE_DOWN 267
#define GLSG_KEY_HOME 268
#define GLSG_KEY_END 269
#define GLSG_KEY_CAPS_LOCK 280
#define GLSG_KEY_SCROLL_LOCK 281
#define GLSG_KEY_NUM_LOCK 282
#define GLSG_KEY_PRINT_SCREEN 283
#define GLSG_KEY_PAUSE 284
#define GLSG_KEY_F1 290
#define GLSG_KEY_F2 291
#define GLSG_KEY_F3 292
#define GLSG_KEY_F4 293
#define GLSG_KEY_F5 294
#define GLSG_KEY_F6 295
#define GLSG_KEY_F7 296
#define GLSG_KEY_F8 297
#define GLSG_KEY_F9 298
#define GLSG_KEY_F10 299
#define GLSG_KEY_F11 300
#define GLSG_KEY_F12 301
#define GLSG_KEY_F13 302
#define GLSG_KEY_F14 303
#define GLSG_KEY_F15 304
#define GLSG_KEY_F16 305
#define GLSG_KEY_F17 306
#define GLSG_KEY_F18 307
#define GLSG_KEY_F19 308
#define GLSG_KEY_F20 309
#define GLSG_KEY_F21 310
#define GLSG_KEY_F22 311
#define GLSG_KEY_F23 312
#define GLSG_KEY_F24 313
#define GLSG_KEY_F25 314
#define GLSG_KEY_KP_0 320
#define GLSG_KEY_KP_1 321
#define GLSG_KEY_KP_2 322
#define GLSG_KEY_KP_3 323
#define GLSG_KEY_KP_4 324
#define GLSG_KEY_KP_5 325
#define GLSG_KEY_KP_6 326
#define GLSG_KEY_KP_7 327
#define GLSG_KEY_KP_8 328
#define GLSG_KEY_KP_9 329
#define GLSG_KEY_KP_DECIMAL 330
#define GLSG_KEY_KP_DIVIDE 331
#define GLSG_KEY_KP_MULTIPLY 332
#define GLSG_KEY_KP_SUBTRACT 333
#define GLSG_KEY_KP_ADD 334
#define GLSG_KEY_KP_ENTER 335
#define GLSG_KEY_KP_EQUAL 336
#define GLSG_KEY_LEFT_SHIFT 340
#define GLSG_KEY_LEFT_CONTROL 341
#define GLSG_KEY_LEFT_ALT 342
#define GLSG_KEY_LEFT_SUPER 343
#define GLSG_KEY_RIGHT_SHIFT 344
#define GLSG_KEY_RIGHT_CONTROL 345
#define GLSG_KEY_RIGHT_ALT 346
#define GLSG_KEY_RIGHT_SUPER 347
#define GLSG_KEY_MENU 348

#define GLSG_KEY_LAST GLSG_KEY_MENU

/*! @brief If this bit is set one or more Shift keys were held down.
 *
 *  If this bit is set one or more Shift keys were held down.
 */
#define GLSG_MOD_SHIFT 0x0001
/*! @brief If this bit is set one or more Control keys were held down.
 *
 *  If this bit is set one or more Control keys were held down.
 */
#define GLSG_MOD_CONTROL 0x0002
/*! @brief If this bit is set one or more Alt keys were held down.
 *
 *  If this bit is set one or more Alt keys were held down.
 */
#define GLSG_MOD_ALT 0x0004
/*! @brief If this bit is set one or more Super keys were held down.
 *
 *  If this bit is set one or more Super keys were held down.
 */
#define GLSG_MOD_SUPER 0x0008
/*! @brief If this bit is set the Caps Lock key is enabled.
 *
 *  If this bit is set the Caps Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define GLSG_MOD_CAPS_LOCK 0x0010
/*! @brief If this bit is set the Num Lock key is enabled.
 *
 *  If this bit is set the Num Lock key is enabled and the @ref
 *  GLFW_LOCK_KEY_MODS input mode is set.
 */
#define GLSG_MOD_NUM_LOCK 0x0020

/*! @name Key and button actions
 *  @{ */
/*! @brief The key or mouse button was released.
 *
 *  The key or mouse button was released.
 *
 *  @ingroup input
 */
#define GLSG_RELEASE 0
/*! @brief The key or mouse button was pressed.
 *
 *  The key or mouse button was pressed.
 *
 *  @ingroup input
 */
#define GLSG_PRESS 1
/*! @brief The key was held down until it repeated.
 *
 *  The key was held down until it repeated.
 *
 *  @ingroup input
 */
#define GLSG_REPEAT 2
/*! @} */

/*! @defgroup buttons Mouse buttons
 *  @brief Mouse button IDs.
 *
 *  See [mouse button input](@ref input_mouse_button) for how these are used.
 *
 *  @ingroup input
 *  @{ */
#define GLSG_MOUSE_BUTTON_1 0
#define GLSG_MOUSE_BUTTON_2 1
#define GLSG_MOUSE_BUTTON_3 2
#define GLSG_MOUSE_BUTTON_4 3
#define GLSG_MOUSE_BUTTON_5 4
#define GLSG_MOUSE_BUTTON_6 5
#define GLSG_MOUSE_BUTTON_7 6
#define GLSG_MOUSE_BUTTON_8 7
#define GLSG_MOUSE_BUTTON_LAST GLSG_MOUSE_BUTTON_8
#define GLSG_MOUSE_BUTTON_LEFT GLSG_MOUSE_BUTTON_1
#define GLSG_MOUSE_BUTTON_RIGHT GLSG_MOUSE_BUTTON_2
#define GLSG_MOUSE_BUTTON_MIDDLE GLSG_MOUSE_BUTTON_3
/*! @} */

#define GLSG_CURSOR 0x00033001
#define GLSG_STICKY_KEYS 0x00033002
#define GLSG_STICKY_MOUSE_BUTTONS 0x00033003
#define GLSG_LOCK_KEY_MODS 0x00033004
#define GLSG_RAW_MOUSE_MOTION 0x00033005

#define GLSG_CURSOR_NORMAL 0x00034001
#define GLSG_CURSOR_HIDDEN 0x00034002
#define GLSG_CURSOR_DISABLED 0x00034003

// Internal key state used for sticky keys
#define _GLSG_STICK 3

#ifdef _WIN32
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#endif

#define GLSG_DONT_CARE (-1)

// -------------------- ENUMS ----------------------------

namespace ara {

enum class CoordType : int {
    Position = 0,
    Normal,
    TexCoord,
    Color,
    TexCoordMod,
    Velocity,
    Aux0,
    Aux1,
    Aux2,
    Aux3,
    ModMatr,
    Count
};
enum class align : int { center = 0, left, right, justify, justify_ex };
enum class valign : int { center = 0, top, bottom };
enum class MouseIcon : int { arrow = 0, move, scale, rotate, ibeam, hresize, vresize, count };
enum class WinMouseIcon : int {
    arrow = 0,
    hresize,
    vresize,
    lbtrResize,
    ltbrResize,
    crossHair,
    move,
    rotate,
    scale,
    count
};
enum class mouseButt : int { left = 0, middle, right };
enum class mouseDragType : int { none = 0, translating, rotating, rolling, zooming, snapToAxis };

static std::map<std::string, MouseIcon> stringToMouseIcon = {
    {"arrow", MouseIcon::arrow},    {"move", MouseIcon::move},   {"scale", MouseIcon::scale},
    {"rotate", MouseIcon::rotate},  {"ibeam", MouseIcon::ibeam}, {"hresize", MouseIcon::hresize},
    {"vresize", MouseIcon::vresize}};
enum class ShaderType : int { Vertex = 0, Geometry, Fragment, Count };
enum class StdMatNameInd : int { ModelMat = 0, CamModelMat, ViewMat, ProjectionMat, NormalMat };
enum class TexCordGenType : int { PlaneProjection = 0 };
enum class unitType : int { Pixels = 0, Percent };
enum class extrapolM : int { Mirror = 0, Circle };
enum class interpolM : int { Bilinear = 0, CatmullRomCentri, Bezier };
enum class maskType : int { Vector = 0, Bitmap, Count };
enum class cfState : int { fine = 0, normal, coarse };
enum class hidEvent : int {
    KeyDown = 0,
    KeyUp,
    onChar,
    MouseDownLeft,
    MouseDownLeftNoDrag,
    MouseUpLeft,
    MouseDownRight,
    MouseUpRight,
    MouseMove,
    MouseDrag,
    MouseWheel,
    OnResize,
    SetViewport,
    Size
};
enum class glFun : int { Enable = 0, Disable, BlendEquation, BlendFunc, PolygonMode };
enum class changeWinEvent : int { OnResize, SetViewport, Maximize, Minimize, SetWinPos };
enum class StereoEye : int { left = 0, right = 1, count = 2 };

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

class ProcStep {
public:
    bool                  active = false;
    std::function<void()> func   = nullptr;
};

// forward declarations
class ShaderCollector;

typedef struct {
    uint count;
    uint primCount;
    uint first;
    uint baseInstance;
} DrawArraysIndirectCommand;

// -----------------------------------------------

glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);

float perlinOct1D(float x, int octaves, float persistence);

glm::vec4 linInterpolVec4(float inInd, std::vector<glm::vec4> *array);

std::pair<bool, glm::vec2> projPointToLine(glm::vec2 point, glm::vec2 l1Start, glm::vec2 l1End);

std::pair<bool, glm::vec2> lineIntersect(glm::vec2 l1Start, glm::vec2 l1End, glm::vec2 l2Start, glm::vec2 l2End);

void catmullRom(std::vector<glm::vec2> &inPoints, std::vector<glm::vec2> &outPoints, uint32_t dstNrPoints);

float distPointLine(glm::vec2 _point, glm::vec2 _lineP1, glm::vec2 _lineP2, bool *projIsOutside = nullptr,
                    float *projAngle = nullptr);

float sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3);
bool  pointInTriangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3);
float angleBetweenVectors(const glm::vec3& a, const glm::vec3& b);
glm::vec3 getRandomPointOnPlane(const glm::vec3&, const glm::vec3&, const glm::vec3&);

void makeMatr(glm::mat4 *_matr, bool *_inited, float xOffs, float yOffs, float zOffs, float rotX, float rotY,
              float rotZ, float scaleX, float scaleY, float scaleZ);

matrix *projection_matrix(const double *x, double *y, double *_x, double *_y);

glm::mat4 matrixToGlm(matrix *_mat);

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
static const inline glm::vec2          stdQuadVertices[4]{glm::vec2{0.f}, glm::vec2{1.f, 0.f}, glm::vec2{0.f, 1.f},
                                                              glm::vec2{1.f, 1.f}};
static const inline int                stdQuadInd[6]{0, 1, 3, 3, 2, 0};  // two triangles defining a quad

[[maybe_unused]] static const std::vector<std::string> &getStdAttribNames() { return m_stdAttribNames; }
[[maybe_unused]] static const std::vector<std::string> &getStdRecAttribNames() { return m_stdRecAttribNames; }
[[maybe_unused]] static const std::vector<int>         &getCoTypeStdSize() { return m_coTypeStdSize; }
[[maybe_unused]] static const std::vector<int>         &getCoTypeFragSize() { return m_coTypeFragSize; }
[[maybe_unused]] static const std::vector<int>         &getRecCoTypeFragSize() { return m_recCoTypeFragSize; }
[[maybe_unused]] static const std::vector<std::string> &getStdMatrixNames() { return m_stdMatrixNames; };
[[maybe_unused]] static const std::string              &getStdPvmMult() { return m_stdPvmMult; };

void glesGetTexImage(GLuint textureObj, GLenum target, GLenum format, GLenum pixelType, int width, int height,
                     GLubyte *pixels);

bool initGLEW();

}  // namespace ara