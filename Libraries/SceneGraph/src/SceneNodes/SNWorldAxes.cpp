#include "SceneNodes/SNWorldAxes.h"

#include "Shaders/ShaderPrototype/SPWorldAxes.h"

using namespace glm;
using namespace std;

namespace ara {

SNWorldAxes::SNWorldAxes(sceneData* sd) : SNGridFloor(sd) {
    setName(getTypeName<SNWorldAxes>());

    m_protoName[GLSG_SCENE_PASS] = getTypeName<SPWorldAxes>();  // force the use of the GridFloor
                                                                // ShaderPrototype if it is present

    // x and z axis
    vector positions{vec3{-1.f, 0.f, -0.0001f}, vec3{1.f, 0.f, -0.0001f}, vec3{0.f, -1.f, -0.0001f},
                     vec3{ 0.f,  1.f, -0.0001f}, vec3{0.f, 0.f, -40.f}, vec3{0.f, 0.f,  40.f}};

    m_lineVao = make_unique<VAO>("position:3f", GL_STATIC_DRAW);
    m_lineVao->upload(CoordType::Position, &positions[0][0], 6);
}

void SNWorldAxes::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO*) {
    if (pass != GLSG_SCENE_PASS) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(m_depthMask);
    glLineWidth(2.f);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    shader->setUniform4fv("lineCol", &m_lineCol[0][0], 3);
    m_lineVao->draw(GL_LINES);

    if (!m_depthMask) {
        glDepthMask(true);
    }

    glLineWidth(1.f);
}

}  // namespace ara
