#include "SceneNodes/SNGizmoTransAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoTransAxis::SNGizmoTransAxis(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = GLSG_GIZMO;

    vec2    cylRadius      = {0.0125f, 0.08f};
    vec2    capRadius      = {0.05f, 0.2f};
    float   capYPos           = 0.8f;
    int32_t cylNrPointsCircle = 15;

    // Modern graphic cards are optimized for triangles, so we define Triangles

    // define a ring in the x, z plain
    auto ringPos = get2DRing(cylNrPointsCircle * 2);

    // three rings and two points with each three coordinates (x, y, z)
    vector<vec3> positions(cylNrPointsCircle * 3 + 2);
    vector<vec3> normals(cylNrPointsCircle * 3 + 2);

    auto posIt = positions.begin();
    auto normIt = normals.begin();

    for (int vn = 0; vn < 2; vn++) {
        // define the three rings
        // 0: the cylinder bottom
        // 1: the cylinder top
        // 3: the cap of the tip
        for (uint ringNr = 0; ringNr < 3; ringNr++) {
            buildRing(cylNrPointsCircle,
                posIt,
                normIt,
                ringPos,
                (ringNr < 2 ? cylRadius[vn] : capRadius[vn]),
                ringNr == 0 ? 0.f : ringNr == 1 ? 0.9f : capYPos);
        }

        // center of cap and center of the tip
        for (uint i = 0; i < 2; i++) {
            posIt->x = 0.f;
            posIt->y = i == 0 ? capYPos : (vn ? 1.3f : 1.f);
            posIt->z = 0.f;

            normIt->x = 0.f;
            normIt->y = 1.f;
            normIt->z = 0.f;

            ++posIt;
            ++normIt;
        }

        // create Indices
        auto cyl_indices = buildCylinderIndices(cylNrPointsCircle);
        auto ind = cyl_indices.size();
        cyl_indices.reserve(cylNrPointsCircle * 12);

        // cap and tip
        uint capCenterInd = cylNrPointsCircle * 3;
        uint posIndOffs   = cylNrPointsCircle * 2;

        for (auto k = 0; k < 2; k++) {
            for (auto i = 0; i < cylNrPointsCircle; i++) {
                for (auto j = 0; j < 3; j++) switch (j) {
                    case 0: cyl_indices[ind++] = capCenterInd + k; break;  // always center of cap / tip
                    case 1: cyl_indices[ind++] = ((i + 1) % cylNrPointsCircle) + posIndOffs; break;
                    case 2: cyl_indices[ind++] = i + posIndOffs; break;
                    default: break;
                }
            }
        }

        m_gizVao[vn] = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
        m_gizVao[vn]->upload(CoordType::Position, &positions[0][0], cylNrPointsCircle * 3 + 2);
        m_gizVao[vn]->upload(CoordType::Normal, &normals[0][0], cylNrPointsCircle * 3 + 2);
        m_gizVao[vn]->setElemIndices(cylNrPointsCircle * 12, &cyl_indices[0]);
    }

    m_totNrPoints = cylNrPointsCircle * 12;
}

void SNGizmoTransAxis::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (pass == GLSG_GIZMO_PASS || pass == GLSG_SCENE_PASS || pass == GLSG_OBJECT_MAP_PASS) {
        // material
        shader->setUniform1i("hasTexture", 0);
        shader->setUniform1i("lightMode", 0);
        shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        shader->setUniform4f("emissive", m_gColor.r * m_emisBright, m_gColor.g * m_emisBright, m_gColor.b * m_emisBright,
                             m_gColor.a * m_emisBright);
        shader->setUniform4fv("diffuse", glm::value_ptr(m_gColor));
        shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        shader->setUniform1i("drawGridTexture", (int)m_drawGridTex);

        m_gizVao[pass == GLSG_OBJECT_MAP_PASS ? 1 : 0]->drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, m_totNrPoints);
    }
}

}  // namespace ara