/*!
 *
 * The Gizmos for Rotation basically consist of three rings in each plane. The
 * Rings are constructed as an extruded circle around another circle as base
 * path The Z-component of the Gizmos is always drawn first, so during this run
 * through we clear the DepthBuffer to have to Gizmo being always visible.
 *
 */

#include "CameraSets/CameraSet.h"
#include "SceneNodes/SNGizmoRotAxis.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoRotAxisLetter::SNGizmoRotAxisLetter(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = GLSG_GIZMO;

    // allocate memory for all positions and normals
    // we just need a simple line
    m_colors.resize(16);
    std::fill(m_colors.begin(), m_colors.end(), 1.f);

    gizVao = make_unique<VAO>("position:3f,color:4f", GL_STATIC_DRAW);
    gizVao->upload(CoordType::Position, &m_positions[0], 4);
    gizVao->upload(CoordType::Color, &m_colors[0], 4);

    totNrPoints = 2;
}

void SNGizmoRotAxisLetter::draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (_pass == GLSG_GIZMO_PASS || _pass == GLSG_OBJECT_MAP_PASS) {
        // material
        _shader->setUniform1i("hasTexture", 0);
        _shader->setUniform1i("lightMode", 0);
        _shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        _shader->setUniform4f("emissive", gColor.r * emisBright, gColor.g * emisBright, gColor.b * emisBright,
                              gColor.a * emisBright);
        _shader->setUniform4fv("diffuse", value_ptr(gColor));
        _shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        _shader->setUniform1i("drawGridTexture", 0);

        gizVao->draw(GL_LINES);
    }
}

}  // namespace ara