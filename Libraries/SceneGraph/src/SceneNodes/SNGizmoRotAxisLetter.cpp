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
#include "SceneNodes/SNGizmoRotAxisLetter.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoRotAxisLetter::SNGizmoRotAxisLetter(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = GLSG_GIZMO;

    // allocate memory for all positions and normals we just need a simple line
    m_colors.resize(16);
    ranges::fill(m_colors, 1.f);

    m_gizVao = make_unique<VAO>("position:3f,color:4f", GL_STATIC_DRAW);
    m_gizVao->upload(CoordType::Position, &m_positions[0], 4);
    m_gizVao->upload(CoordType::Color, &m_colors[0], 4);

    m_totNrPoints = 2;
}

void SNGizmoRotAxisLetter::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (pass == GLSG_GIZMO_PASS || pass == GLSG_OBJECT_MAP_PASS) {
        shader->setUniform1i("hasTexture", 0);
        shader->setUniform1i("lightMode", 0);
        shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        shader->setUniform4f("emissive", m_gColor.r * m_emisBright, m_gColor.g * m_emisBright, m_gColor.b * m_emisBright,
                              m_gColor.a * m_emisBright);
        shader->setUniform4fv("diffuse", value_ptr(m_gColor));
        shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        shader->setUniform1i("drawGridTexture", 0);

        m_gizVao->draw(GL_LINES);
    }
}

}  // namespace ara