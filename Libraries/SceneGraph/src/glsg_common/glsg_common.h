#pragma once

#include <Log.h>
#include <TypeName.h>
#include <glb_common/glb_common.h>

#include <regex>

#ifdef _WIN32
#include "Windows/WindowsMousePos.h"
#elif __linux__
#include <X11/X11MousePos.h>
#elif __APPLE__
#include <OsX/OSXMousePos.h>
#endif

namespace ara {
enum class cpEditMode : int {
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

enum gizmoType : uint64_t {
    GLSG_TRANS_GIZMO      = 1ULL << 1,
    GLSG_TRANS_GIZMO_X    = 1ULL << 2,
    GLSG_TRANS_GIZMO_Y    = 1ULL << 3,
    GLSG_TRANS_GIZMO_Z    = 1ULL << 4,
    GLSG_SCALE_GIZMO      = 1ULL << 5,
    GLSG_SCALE_GIZMO_X    = 1ULL << 6,
    GLSG_SCALE_GIZMO_Y    = 1ULL << 7,
    GLSG_SCALE_GIZMO_Z    = 1ULL << 8,
    GLSG_ROT_GIZMO        = 1ULL << 9,
    GLSG_ROT_GIZMO_X      = 1ULL << 10,
    GLSG_ROT_GIZMO_Y      = 1ULL << 11,
    GLSG_ROT_GIZMO_Z      = 1ULL << 12,
    GLSG_ROT_AXIS_GIZMO   = 1ULL << 13,
    GLSG_ROT_AXIS_GIZMO_X = 1ULL << 14,
    GLSG_ROT_AXIS_GIZMO_Y = 1ULL << 15,
    GLSG_ROT_AXIS_GIZMO_Z = 1ULL << 16,
    GLSG_PASSIVE_GIZMO    = 1ULL << 17,
    GLSG_PASSIVE_GIZMO_X  = 1ULL << 18,
    GLSG_PASSIVE_GIZMO_Y  = 1ULL << 19,
    GLSG_PASSIVE_GIZMO_Z  = 1ULL << 20
};

enum mouseEditMode { GLSG_EM_NORMAL = 0, GLSG_EM_DIST, GLSG_EM_BRIGHT };
enum infoDiagType : int { info, confirm, warning, error, cancel };
enum class transMode : int { translate = 0, scale, rotate, rotate_axis, passive, twWidget, none };
enum transFlag : uint64_t { GLSG_PROPO_SCALE = 1ULL << 1, GLSG_NO_SCALE = 1ULL << 2 };
enum sliderScale { GLSG_SLID_LIN = 0, GLSG_SLID_SQRT, GLSG_SLID_SQ };
enum class twPlane : int { xy = 0, xz, yz, count };

///> order matters, shadow_map pass has to be executed before the scene_pass
/// otherwise the object can cast shadows onto itself
enum renderPass {
    GLSG_OBJECT_ID_PASS = 0,
    GLSG_OBJECT_MAP_PASS,
    GLSG_SHADOW_MAP_PASS,
    GLSG_SCENE_PASS,
    GLSG_GIZMO_PASS,
    GLSG_NUM_RENDER_PASSES
};

enum sceneNodeType {
    GLSG_SNT_STANDARD = 0,
    GLSG_SNT_LIGHT,
    GLSG_SNT_LIGHT_SCENE_MESH,
    GLSG_GIZMO,
    GLSG_SNT_CAMERA,
    GLSG_SNT_CAMERA_SCENE_MESH,
    GLSG_SNT_COUNT
};

enum class pivotX { left = 0, right, center };
enum class pivotY { bottom = 0, top, center };
enum class undoState { undo = 0, redo, createCopy, none };
enum class basePlane { xz = 0, xy, yz };

class trackballPreset {
public:
    glm::vec3 pos      = glm::vec3(0.f);
    glm::vec3 rotEuler = glm::vec3(0.f);
    double    duration = 3.0;
};

// ---------------- Mouse-UI interaction -------------------------

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

    glm::ivec2 screenMousePos{0};       ///> in virtual pixels, relative to screen (not window)
    glm::vec2  mousePos{0.f};           ///> window relative mouse position in virtual
                                        /// pixels (origin top,left)
    glm::vec2 mousePosFlipY{0.f};       ///> window relative mouse position in hardware pixels (origin
                                        /// bottom,left) for use with opengl
    glm::vec2 mousePosNorm{0.f};        ///> normalized window relative mouse position
                                        /// in virtual pixels (origin top,left)
    glm::vec2 mousePosNormFlipY{0.f};   ///> normalized window relative mouse position in virtual pixels
                                        ///(origin bottom,left) for use with opengl
    glm::vec2 mousePosNodeRel{0.f};     ///> in virtual pixels
    glm::vec2 movedPix{0.f};            ///> in virtual pixels, dpi dependent
    glm::vec2 mouseClickPosFlipY{0.f};  ///> onClick normalized window relative mouse position in virtual
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
    std::unordered_map<hidEvent, UINode*> hitNode;

    int          key       = 0;  // key callback
    unsigned int codepoint = 0;  // char callback
    float        degrees   = 0;  // scroll wheel

    float vcViewZoom = 1.f;

    hidData() {
        std::list<hidEvent> evts{hidEvent::MouseMove,   hidEvent::MouseWheel,     hidEvent::MouseDownLeft,
                                 hidEvent::MouseUpLeft, hidEvent::MouseDownRight, hidEvent::MouseUpRight,
                                 hidEvent::MouseDrag,   hidEvent::MouseWheel};

        for (auto& it : evts) hitNode[it] = nullptr;
    }

    void reset() {
        consumed  = false;
        breakCbIt = false;
    }
    void getScreenMousePos(float devicePixelRatio) {
        mouse::getAbsMousePos(screenMousePos.x, screenMousePos.y);
        screenMousePos = (glm::ivec2)((glm::vec2)screenMousePos / devicePixelRatio);
    }
};

[[maybe_unused]] static std::string getStyleDefName(const std::string& name) { return "__" + name + "_default"; }

// second of pair indicates whether the callback should only be called when the
// element was really clicked (true => data->hit == true) or if the callback
// should be called in any case
typedef std::pair<std::function<void(hidData*)>, bool> mouseCb;
}  // namespace ara