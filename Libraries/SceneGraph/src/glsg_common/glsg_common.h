#pragma once

#include <Log.h>
#include <TypeName.h>
#include <GlbCommon/GlbCommon.h>

#include <regex>

namespace ara {

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

enum class mouseEditMode : int32_t { emNormal = 0, emDist, emBright };
enum class infoDiagType : int32_t { info, confirm, warning, error, cancel };
enum class transMode : int32_t { translate = 0, scale, rotate, rotate_axis, passive, twWidget, none };
enum class transFlag : uint64_t { propoScale = 1ULL << 1, noScale = 1ULL << 2 };
enum class twPlane : int32_t { xy = 0, xz, yz, count };

///> order matters, shadow_map pass has to be executed before the scene_pass
/// otherwise the object can cast shadows onto itself
enum class renderPass : int32_t {
    objectId = 0,
    objectMap,
    shadowMap,
    scene,
    gizmo,
    size
};

enum class sceneNodeType : int32_t {
    standard = 0,
    light,
    lightSceneMesh,
    gizmo,
    camera,
    cameraSceneMesh,
    size
};

enum class undoState : int32_t { undo = 0, redo, createCopy, none };
enum class basePlane : int32_t { xz = 0, xy, yz };

class trackballPreset {
public:
    glm::vec3 pos{};
    glm::vec3 rotEuler{};
    double    duration = 3.0;
};

static void buildRing(int32_t nrPoints, std::vector<glm::vec3>::iterator& pos, std::vector<glm::vec3>::iterator& norm,
                      const std::vector<glm::vec2>& ringPos, float radius, float yPos, std::optional<glm::vec3> fixedNorm = std::nullopt) {
    for (int32_t i = 0; i < nrPoints; i++) {
        pos->x = ringPos[i].x * radius;
        pos->y = yPos;
        pos->z = ringPos[i].y * radius;

        glm::vec3 defNorm = { ringPos[i]. x, 0.f, ringPos[i].y };

        *norm = fixedNorm.value_or(defNorm);

        ++pos;
        ++norm;
    }
}

static std::vector<GLuint> buildCylinderIndices(uint32_t nrPointsCircle) {
    std::vector<GLuint> indices(nrPointsCircle * 6);
    auto it = indices.begin();

    //  clockwise (viewed from the camera)
    std::array<GLuint, 6> oneQuadTemp = {0, 0, 1, 1, 0, 1};
    std::array<GLuint, 6> upDownTemp  = {0, 1, 0, 0, 1, 1};  // 0 = bottom, 1 ==top

    for (uint32_t i = 0; i < nrPointsCircle; i++) {
        for (auto j = 0; j < 6; j++) {
            *it++ = ((oneQuadTemp[j] + i) % nrPointsCircle) + (nrPointsCircle * upDownTemp[j]);
        }
    }

    return indices;
}

[[maybe_unused]] static std::string getStyleDefName(const std::string& name) { return "__" + name + "_default"; }

}  // namespace ara