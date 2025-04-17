#include "SceneNodes/SNGizmoScaleAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoScaleAxis::SNGizmoScaleAxis(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = GLSG_GIZMO;

    const uint cylNrPointsCircle = 15;

    float x, z, fInd;
    float cylRadius = 0.03f;
    float capRadius = 0.25f;
    float cubeScale = 0.1f;

    // Modern graphic cards are optimized for triangles, so we define Triangles

    // define a ring in the x, z plain
    GLfloat ringPos[cylNrPointsCircle * 2];

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
    // two rings and eight points with each three coordinates (x, y, z)
    GLfloat positions[(cylNrPointsCircle * 2 + 8) * 3];
    GLfloat normals[(cylNrPointsCircle * 2 + 8) * 3];

    // define the two rings
    // 0: the cylinder bottom
    // 1: the cylinder top
    uint ind = 0;
    for (uint ringNr = 0; ringNr < 2; ringNr++) {
        for (uint i = 0; i < cylNrPointsCircle; i++) {
            positions[ind * 3]     = ringPos[i * 2] * (ringNr < 2 ? cylRadius : capRadius);
            positions[ind * 3 + 1] = ringNr == 0 ? 0.f : 1.f;
            positions[ind * 3 + 2] = ringPos[i * 2 + 1] * (ringNr < 2 ? cylRadius : capRadius);

            normals[ind * 3]     = ringPos[i * 2];
            normals[ind * 3 + 1] = 0.f;
            normals[ind * 3 + 2] = ringPos[i * 2 + 1];

            ind++;
        }
    }

    // the eight points of the cube
    GLfloat cube_vertices[8][3] = {{1.f, 1.f, 1.f},  {1.f, -1.f, 1.f},  {-1.f, -1.f, 1.f},  {-1.f, 1.f, 1.f},
                                   {1.f, 1.f, -1.f}, {1.f, -1.f, -1.f}, {-1.f, -1.f, -1.f}, {-1.f, 1.f, -1.f}};

    for (uint i = 0; i < 8; i++) {
        positions[ind * 3]     = cube_vertices[i][0] * cubeScale;
        positions[ind * 3 + 1] = cube_vertices[i][1] * cubeScale + 1.f - cubeScale;
        positions[ind * 3 + 2] = cube_vertices[i][2] * cubeScale;

        vec3 cubePointNormal = glm::normalize(vec3(cube_vertices[i][0], cube_vertices[i][1], cube_vertices[i][2]));
        normals[ind * 3]     = cubePointNormal.x;
        normals[ind * 3 + 1] = cubePointNormal.y;
        normals[ind * 3 + 2] = cubePointNormal.z;

        ind++;
    }

    // create Indices
    // for the cylinder we need one quad of two triangles per ringPoint =
    // cylNrPointsCircle *6 Points for the cylinder = 6 sides * 6 points (=2
    // triangles per side)
    GLuint cyl_indices[cylNrPointsCircle * 9 + 36];

    GLuint cube_faces[6][6] = {{0, 1, 3, 3, 1, 2}, {1, 5, 2, 2, 5, 6}, {4, 0, 7, 7, 0, 3},
                               {3, 2, 7, 7, 2, 6}, {4, 5, 0, 0, 5, 1}, {7, 6, 4, 4, 6, 5}};

    //  clockwise (viewed from the camera)
    GLuint oneQuadTemp[6] = {0, 0, 1, 1, 0, 1};
    GLuint upDownTemp[6]  = {0, 1, 0, 0, 1, 1};  // 0 = bottom, 1 ==top

    ind = 0;
    for (uint i = 0; i < cylNrPointsCircle; i++)
        for (uint j = 0; j < 6; j++)
            cyl_indices[ind++] = ((oneQuadTemp[j] + i) % cylNrPointsCircle) + (cylNrPointsCircle * upDownTemp[j]);

    // cylinder
    for (uint i = 0; i < 6; i++)
        for (uint j = 0; j < 6; j++) cyl_indices[ind++] = cube_faces[i][j] + (cylNrPointsCircle * 2);

    gizVao = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
    gizVao->upload(CoordType::Position, &positions[0], cylNrPointsCircle * 2 + 8);
    gizVao->upload(CoordType::Normal, &normals[0], cylNrPointsCircle * 2 + 8);
    gizVao->setElemIndices(cylNrPointsCircle * 6 + 36, &cyl_indices[0]);

    totNrPoints = cylNrPointsCircle * 6 + 36;
}

void SNGizmoScaleAxis::draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (_pass == GLSG_GIZMO_PASS || _pass == GLSG_OBJECT_MAP_PASS) {
        // material
        _shader->setUniform1i("hasTexture", 0);
        _shader->setUniform1i("lightMode", 0);
        _shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        _shader->setUniform4f("emissive", gColor.r * emisBright, gColor.g * emisBright, gColor.b * emisBright,
                              gColor.a * emisBright);
        _shader->setUniform4fv("diffuse", glm::value_ptr(gColor));
        _shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        _shader->setUniform1i("drawGridTexture", 0);

        gizVao->drawElements(GL_TRIANGLES, NULL, GL_TRIANGLES, totNrPoints);
    }
}

}  // namespace ara