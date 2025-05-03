#include "SceneNodes/SNGizmoScaleAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoScaleAxis::SNGizmoScaleAxis(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = sceneNodeType::gizmo;

    constexpr uint cylNrPointsCircle = 15;

    float cylRadius = 0.03f;
    float capRadius = 0.25f;
    float cubeScale = 0.1f;

    // Modern graphic cards are optimized for triangles, so we define Triangles

    // define a ring in the x, z plain
    auto ringPos = get2DRing(cylNrPointsCircle * 2);

    // allocate memory for all positions and normals
    // two rings and eight points with each three coordinates (x, y, z)
    vector<vec3> positions(cylNrPointsCircle * 2 + 8);
    vector<vec3> normals(cylNrPointsCircle * 2 + 8);

    auto posIt = positions.begin();
    auto normIt = normals.begin();

    // define the two rings
    // 0: the cylinder bottom
    // 1: the cylinder top
    uint ind = 0;
    for (auto ringNr = 0; ringNr < 2; ringNr++) {
        buildRing(cylNrPointsCircle, posIt, normIt, ringPos, cylRadius, ringNr == 0 ? 0.f : 1.f);
    }

    // the eight points of the cube
    std::array cube_vertices = {
        vec3{1.f, 1.f, 1.f},
        vec3{1.f, -1.f, 1.f},
        vec3{-1.f, -1.f, 1.f},
        vec3{-1.f, 1.f, 1.f},
        vec3{1.f, 1.f, -1.f},
        vec3{1.f, -1.f, -1.f},
        vec3{-1.f, -1.f, -1.f},
        vec3{-1.f, 1.f, -1.f}};

    for (auto & cube_vertice : cube_vertices) {
        for (auto i = 0; i < 3; i++) {
            (*posIt)[i] = cube_vertice[i] * cubeScale + (i == 1 ? 1.f - cubeScale : 0.f);
        }

        vec3 cubePointNormal = glm::normalize(vec3(cube_vertice[0], cube_vertice[1], cube_vertice[2]));
        for (auto i = 0; i < 3; i++) {
            (*normIt)[i]  = cubePointNormal[i];
        }

        ++posIt;
        ++normIt;
    }

    // create Indices
    // for the cylinder we need one quad of two triangles per ringPoint =
    // cylNrPointsCircle *6 Points for the cylinder = 6 sides * 6 points (=2
    // triangles per side)
    std::vector<GLuint> cyl_indices(cylNrPointsCircle * 9 + 36);

    std::array cube_faces = {
        std::array<GLuint, 6>{0, 1, 3, 3, 1, 2},
        std::array<GLuint, 6>{1, 5, 2, 2, 5, 6},
        std::array<GLuint, 6>{4, 0, 7, 7, 0, 3},
        std::array<GLuint, 6>{3, 2, 7, 7, 2, 6},
        std::array<GLuint, 6>{4, 5, 0, 0, 5, 1},
        std::array<GLuint, 6>{7, 6, 4, 4, 6, 5}};

    //  clockwise (viewed from the camera)
    std::array<GLuint, 6> oneQuadTemp = {0, 0, 1, 1, 0, 1};
    std::array<GLuint, 6> upDownTemp  = {0, 1, 0, 0, 1, 1};  // 0 = bottom, 1 ==top

    ind = 0;
    for (auto i = 0; i < cylNrPointsCircle; i++) {
        for (auto j = 0; j < 6; j++) {
            cyl_indices[ind++] = ((oneQuadTemp[j] + i) % cylNrPointsCircle) + (cylNrPointsCircle * upDownTemp[j]);
        }
    }

    // cylinder
    for (auto & cube_face : cube_faces) {
        for (unsigned int j : cube_face) {
            cyl_indices[ind++] = j + (cylNrPointsCircle * 2);
        }
    }

    m_gizVao = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
    m_gizVao->upload(CoordType::Position, &positions[0][0], cylNrPointsCircle * 2 + 8);
    m_gizVao->upload(CoordType::Normal, &normals[0][0], cylNrPointsCircle * 2 + 8);
    m_gizVao->setElemIndices(cylNrPointsCircle * 6 + 36, &cyl_indices[0]);

    m_totNrPoints = cylNrPointsCircle * 6 + 36;
}

void SNGizmoScaleAxis::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (pass == renderPass::gizmo || pass == renderPass::objectMap) {
        // material
        shader->setUniform1i("hasTexture", 0);
        shader->setUniform1i("lightMode", 0);
        shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        shader->setUniform4f("emissive", m_gColor.r * m_emisBright, m_gColor.g * m_emisBright, m_gColor.b * m_emisBright,
                              m_gColor.a * m_emisBright);
        shader->setUniform4fv("diffuse", glm::value_ptr(m_gColor));
        shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        shader->setUniform1i("drawGridTexture", 0);

        m_gizVao->drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, m_totNrPoints);
    }
}

}  // namespace ara