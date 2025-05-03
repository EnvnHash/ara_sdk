#include "SNGizmoPlane.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoPlane::SNGizmoPlane(sceneData* sd) : SceneNode(sd) {
    m_nodeType = sceneNodeType::gizmo;

    // draw a quarter ring with a specific width in the x,y plane define as
    // triangles

    // create two versions of each plane. A visible small one and a bigger one
    // for the object map
    for (uint v = 0; v < 2; v++) {
        // separate the plane in two half for different colors
        for (uint l = 0; l < 2; l++) {
            // create base points for a quarter circle
            vector<vec3> b_positions(m_nrBasePoints);
            vector<vec3> b_normals(m_nrBasePoints);

            for (uint i = 0; i < m_nrBasePoints; i++) {
                float fInd  = static_cast<float>(i) / static_cast<float>(m_nrBasePoints - 1);
                float phase = (fInd * static_cast<float>(M_PI) * 0.25f) + static_cast<float>(l) * static_cast<float>(M_PI) * 0.25f;

                b_positions[i] = vec3(std::cos(phase), std::sin(phase), 0.f);
                b_normals[i]   = vec3(0.f, 0.f, 1.f);
            }

            // for each base point of the ring two triangles (6 vertices) are
            // needed to define the ring counterclockwise
            uint totNrPoints = (m_nrBasePoints - 1) * 6;

            vector<GLfloat> positions(totNrPoints * 3);
            vector<GLfloat> normals(totNrPoints * 3);
            std::array<GLuint, 6> templ = {0, 0, 1, 1, 0, 1};
            std::array radOffs = {m_ringOuterWidth[v], m_ringInnerWidth[v], m_ringOuterWidth[v],
                                  m_ringOuterWidth[v], m_ringInnerWidth[v], m_ringInnerWidth[v]};

            for (uint i = 0; i < (m_nrBasePoints - 1); i++) {
                for (uint j = 0; j < 6; j++) {
                    uint ind = (i * 6 + j) * 3;

                    for (auto k = 0; k < 2; k++) {
                        positions[ind + k] = b_positions[(templ[j] + i)][k] * radOffs[j];
                    }

                    positions[ind + 2] = 0.f;

                    for (auto k = 0; k < 3; k++) {
                        normals[ind + k] = b_normals[i][k];
                    }
                }
            }

            m_planeVao[v][l] = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
            m_planeVao[v][l]->upload(CoordType::Position, &positions[0], totNrPoints);
            m_planeVao[v][l]->upload(CoordType::Normal, &normals[0], totNrPoints);
        }
    }
}

void SNGizmoPlane::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // material
    shader->setUniform1i("hasTexture", 0);
    shader->setUniform1i("lightMode", 0);
    shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
    shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
    shader->setUniform1i("drawGridTexture", 0);

    for (uint i = 0; i < 2; i++) {
        shader->setUniform4f("emissive", m_gColor[i].r * m_emisBright, m_gColor[i].g * m_emisBright, m_gColor[i].b * m_emisBright,
                             m_gColor[i].a * m_emisBright);
        shader->setUniform4fv("diffuse", value_ptr(m_gColor[i]));

        m_planeVao[pass == renderPass::objectMap ? 1 : 0][i]->draw();
    }
}

void SNGizmoPlane::setGizmoUpperColor(float r, float g, float b, float a) {
    m_gColor[1] = vec4{r, g, b, a};
}

void SNGizmoPlane::setGizmoLowerColor(float r, float g, float b, float a) {
    m_gColor[0] = vec4{r, g, b, a};
}

}  // namespace ara