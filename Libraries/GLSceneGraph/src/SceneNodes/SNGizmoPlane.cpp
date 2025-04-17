#include "SNGizmoPlane.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoPlane::SNGizmoPlane(sceneData* sd) : SceneNode(sd), nrBasePoints(20), emisBright(0.3f) {
    m_nodeType = GLSG_GIZMO;

    // draw a quarter ring with a specific width in the x,y plane define as
    // triangles

    // create two versions of each plane. A visible small one and a bigger one
    // for the object map
    for (uint v = 0; v < 2; v++) {
        // separate the plane in two half for different colors
        for (uint l = 0; l < 2; l++) {
            // create base points for a quarter circle
            vector<vec3> b_positions(nrBasePoints);
            vector<vec3> b_normals(nrBasePoints);

            for (uint i = 0; i < nrBasePoints; i++) {
                float fInd  = static_cast<float>(i) / static_cast<float>(nrBasePoints - 1);
                float phase = (fInd * float(M_PI) * 0.25f) + float(l) * float(M_PI) * 0.25f;

                b_positions[i] = vec3(std::cos(phase), std::sin(phase), 0.f);
                b_normals[i]   = vec3(0.f, 0.f, 1.f);
            }

            // for each base point of the ring two triangles (6 vertices) are
            // needed to define the ring counterclockwise
            uint totNrPoints = (nrBasePoints - 1) * 6;

            vector<GLfloat> positions(totNrPoints * 3);
            vector<GLfloat> normals(totNrPoints * 3);
            GLuint          templ[6]   = {0, 0, 1, 1, 0, 1};
            GLfloat         radOffs[6] = {ringOuterWidth[v], ringInnerWidth[v], ringOuterWidth[v],
                                          ringOuterWidth[v], ringInnerWidth[v], ringInnerWidth[v]};

            for (uint i = 0; i < (nrBasePoints - 1); i++) {
                for (uint j = 0; j < 6; j++) {
                    uint ind = (i * 6 + j) * 3;

                    for (uint k = 0; k < 2; k++) positions[ind + k] = b_positions[(templ[j] + i)][k] * radOffs[j];

                    positions[ind + 2] = 0.f;

                    for (uint k = 0; k < 3; k++) normals[ind + k] = b_normals[i][k];
                }
            }

            planeVao[v][l] = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
            planeVao[v][l]->upload(CoordType::Position, &positions[0], totNrPoints);
            planeVao[v][l]->upload(CoordType::Normal, &normals[0], totNrPoints);
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
        shader->setUniform4f("emissive", gColor[i].r * emisBright, gColor[i].g * emisBright, gColor[i].b * emisBright,
                             gColor[i].a * emisBright);
        shader->setUniform4fv("diffuse", value_ptr(gColor[i]));

        planeVao[pass == GLSG_OBJECT_MAP_PASS ? 1 : 0][i]->draw();
    }
}

void SNGizmoPlane::setGizmoUpperColor(float _r, float _g, float _b, float _a) { gColor[1] = vec4(_r, _g, _b, _a); }

void SNGizmoPlane::setGizmoLowerColor(float _r, float _g, float _b, float _a) { gColor[0] = vec4(_r, _g, _b, _a); }

SNGizmoPlane::~SNGizmoPlane(void) {}

}  // namespace ara