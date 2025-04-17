#include "SceneNodes/SNGizmoTransAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoTransAxis::SNGizmoTransAxis(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = GLSG_GIZMO;

    float x, z, fInd;
    float cylRadius[2]      = {0.0125f, 0.08f};
    float capRadius[2]      = {0.05f, 0.2f};
    float capYPos           = 0.8f;
    uint  cylNrPointsCircle = 15;

    // Modern graphic cards are optimized for triangles, so we define Triangles

    // define a ring in the x, z plain
    vector<GLfloat> ringPos(cylNrPointsCircle * 2);

    for (uint i = 0; i < cylNrPointsCircle; i++) {
        // define a circle with n points
        fInd = float(i) / float(cylNrPointsCircle);
        x    = std::cos(fInd * float(M_PI) * 2.f);
        z    = std::sin(fInd * float(M_PI) * 2.f);

        // tip and cap
        ringPos[i * 2]     = x;
        ringPos[i * 2 + 1] = z;
    }

    // allocate memory for all positions and normals
    // three rings and two points with each three coordinates (x, y, z)
    vector<GLfloat> positions((cylNrPointsCircle * 3 + 2) * 3);
    vector<GLfloat> normals((cylNrPointsCircle * 3 + 2) * 3);

    for (int vn = 0; vn < 2; vn++) {
        // define the three rings
        // 0: the cylinder bottom
        // 1: the cylindera top
        // 3: the cap of the tip
        uint ind = 0;
        for (uint ringNr = 0; ringNr < 3; ringNr++) {
            for (uint i = 0; i < cylNrPointsCircle; i++) {
                positions[ind * 3]     = ringPos[i * 2] * (ringNr < 2 ? cylRadius[vn] : capRadius[vn]);
                positions[ind * 3 + 1] = ringNr == 0 ? 0.f : ringNr == 1 ? 0.9f : capYPos;
                positions[ind * 3 + 2] = ringPos[i * 2 + 1] * (ringNr < 2 ? cylRadius[vn] : capRadius[vn]);

                normals[ind * 3]     = ringPos[i * 2];
                normals[ind * 3 + 1] = 0.f;
                normals[ind * 3 + 2] = ringPos[i * 2 + 1];

                ind++;
            }
        }

        // center of cap and center of the tip
        for (uint i = 0; i < 2; i++) {
            positions[ind * 3]     = 0.f;
            positions[ind * 3 + 1] = i == 0 ? capYPos : (vn ? 1.3f : 1.f);
            positions[ind * 3 + 2] = 0.f;

            normals[ind * 3]     = 0.f;
            normals[ind * 3 + 1] = 1.f;
            normals[ind * 3 + 2] = 0.f;

            ind++;
        }

        // create Indices
        // for the cylinder we need one quad of two triangles per ringPoint =
        // cylNrPointsCircle *6 Points for the tip we need a triangle for each
        // ringPoint = cylNrPointsCircle * 3 Points for the cap the same =
        // cylNrPointsCircle * 3 Points
        vector<GLuint> cyl_indices(cylNrPointsCircle * 12);

        //  clockwise (viewed from the camera)
        GLuint oneQuadTemp[6] = {0, 0, 1, 1, 0, 1};
        GLuint upDownTemp[6]  = {0, 1, 0, 0, 1, 1};  // 0 = bottom, 1 ==top

        ind = 0;
        for (uint i = 0; i < cylNrPointsCircle; i++)
            for (uint j = 0; j < 6; j++)
                cyl_indices[ind++] = ((oneQuadTemp[j] + i) % cylNrPointsCircle) + (cylNrPointsCircle * upDownTemp[j]);

        // cap and tip
        uint capCenterInd = cylNrPointsCircle * 3;
        uint posIndOffs   = cylNrPointsCircle * 2;

        for (uint k = 0; k < 2; k++)
            for (uint i = 0; i < cylNrPointsCircle; i++)
                for (uint j = 0; j < 3; j++) switch (j) {
                        case 0: cyl_indices[ind++] = capCenterInd + k; break;  // always center of cap / tip
                        case 1: cyl_indices[ind++] = ((i + 1) % cylNrPointsCircle) + posIndOffs; break;
                        case 2: cyl_indices[ind++] = i + posIndOffs; break;
                        default: break;
                    }

        m_gizVao[vn] = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
        m_gizVao[vn]->upload(CoordType::Position, &positions[0], cylNrPointsCircle * 3 + 2);
        m_gizVao[vn]->upload(CoordType::Normal, &normals[0], cylNrPointsCircle * 3 + 2);
        m_gizVao[vn]->setElemIndices(cylNrPointsCircle * 12, &cyl_indices[0]);
    }

    totNrPoints = cylNrPointsCircle * 12;
}

void SNGizmoTransAxis::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (pass == GLSG_GIZMO_PASS || pass == GLSG_SCENE_PASS || pass == GLSG_OBJECT_MAP_PASS) {
        // material
        shader->setUniform1i("hasTexture", 0);
        shader->setUniform1i("lightMode", 0);
        shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        shader->setUniform4f("emissive", gColor.r * emisBright, gColor.g * emisBright, gColor.b * emisBright,
                             gColor.a * emisBright);
        shader->setUniform4fv("diffuse", glm::value_ptr(gColor));
        shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        shader->setUniform1i("drawGridTexture", (int)m_drawGridTex);

        m_gizVao[pass == GLSG_OBJECT_MAP_PASS ? 1 : 0]->drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, totNrPoints);
    }
}

}  // namespace ara