#include "SceneNodes/SNGridFloorAxes.h"

#include "Shaders/ShaderPrototype/SPGridFloorAxes.h"

using namespace glm;
using namespace std;

namespace ara {

SNGridFloorAxes::SNGridFloorAxes(sceneData* sd) : SNGridFloor(sd) {
    setName(getTypeName<SNGridFloorAxes>());

    m_protoName[GLSG_SCENE_PASS] = getTypeName<SPGridFloorAxes>();  // force the use of the GridFloor
                                                                    // ShaderPrototype if it is present

    // x and z axis
    vector<GLfloat> positions{-1.f, 0.f, -0.0001f, 1.f, 0.f, -0.0001f, 0.f, -1.f, -0.0001f, 0.f, 1.f, -0.0001f};

    m_lineVao = make_unique<VAO>("position:3f", GL_STATIC_DRAW);
    m_lineVao->upload(CoordType::Position, &positions[0], 4);
}

void SNGridFloorAxes::update(double time, double dt, CameraSet* cs) { SNGridFloor::update(time, dt, cs); }

void SNGridFloorAxes::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO*) {
    if (pass != GLSG_SCENE_PASS) return;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(m_depthMask);
    glLineWidth(2.f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->setUniform2fv("floorGridSize", &m_gridSize[0]);
    shader->setUniform4fv("lineCol", &m_lineCol[0][0], 2);
    m_lineVao->draw(GL_LINES);

    if (!m_depthMask) glDepthMask(true);

    glLineWidth(1.f);
}

}  // namespace ara
