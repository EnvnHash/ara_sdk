/*!
 *
 * The Gizmos for Rotation basically consist of three rings in each plane. The
 * Rings are constructed as an extruded circle around another circle as base
 * path The Z-component of the Gizmos is always drawn first, so during this run
 * through we clear the DepthBuffer to have to Gizmo being always visible.
 *
 */

#include "SceneNodes/SNGizmoRotAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoRotAxis::SNGizmoRotAxis(sceneData* sd) : SNGizmoAxis(sd) {
    m_nodeType = sceneNodeType::gizmo;

#ifndef GIZMO_ROT_SHADER_ONLY
    constexpr uint  nrBasePathPoints   = 30;
    constexpr uint  nrExtrCirclePoints = 20;
    vec2            ringThickness   = {0.05f, 0.2f};

    // build the basePathRing in the x,y plane
    vec3 basePathRing[nrBasePathPoints];
    for (uint i = 0; i < nrBasePathPoints; i++) {
        float radius             = 0.75f;
        auto phase     = static_cast<float>(i) / static_cast<float>(nrBasePathPoints) * static_cast<float>(M_PI) * 2.f;
        basePathRing[i] = vec3(std::cos(phase) * radius, std::sin(phase) * radius, 0.f);
    }

    // build the circle to extrude x, z plane, these will also be used to
    // calculate the normals
    vec3 extrCircle[nrExtrCirclePoints];
    for (uint i = 0; i < nrExtrCirclePoints; i++) {
        float phase   = static_cast<float>(i) / static_cast<float>(nrExtrCirclePoints) * static_cast<float>(M_PI) * 2.f;
        extrCircle[i] = vec3(std::cos(phase), 0.f, std::sin(phase));
    }

    constexpr uint nrVert = nrBasePathPoints * nrExtrCirclePoints;
    GLfloat    positions[nrVert * 3];
    GLfloat    normals[nrVert * 3];

    // for each base-path point create a transformation matrix, to move and
    // rotate the extrude-circle according to the angle at that position

    for (int vn = 0; vn < 2; vn++) {
        for (auto i = 0; i < nrBasePathPoints; i++) {
            float phase = static_cast<float>(i) / static_cast<float>(nrBasePathPoints) * static_cast<float>(M_PI) * 2.f;

            mat4 extrMat = glm::translate(basePathRing[i]) * glm::rotate(phase, vec3(0.f, 0.f, 1.f)) *
                           glm::scale(vec3(ringThickness[vn], 1.f, ringThickness[vn]));

            mat3 normMat = inverseTranspose(mat3(extrMat));

            for (auto j = 0; j < nrExtrCirclePoints; j++) {
                vec3 tPoint = vec3(extrMat * vec4(extrCircle[j], 1.f));
                vec3 tNorm = normMat * extrCircle[j];

                for (auto k = 0; k < 3; k++) {
                    uint indInd       = (i * nrExtrCirclePoints + j) * 3 + k;
                    positions[indInd] = tPoint[k];
                    normals[indInd]   = tNorm[k];
                }
            }
        }

        // to build the full object we need for each extrCirclPoint a quad of
        // two triangles (6 points)
        m_totNrIndices = nrBasePathPoints * nrExtrCirclePoints * 6;

        // create the indices
        std::vector<GLuint>     indices          = std::vector<GLuint>(m_totNrIndices);
        std::array<GLuint, 6>   quadTemplExtCirc = {0, 0, 1, 1, 0, 1};
        std::array<GLuint, 6>   quadTemplBaseP   = {0, 1, 0, 0, 1, 1};

        for (auto i = 0; i < nrBasePathPoints; i++)
            for (auto j = 0; j < nrExtrCirclePoints; j++)
                for (auto k = 0; k < 6; k++) {
                    auto indInd     = (i * nrExtrCirclePoints + j) * 6 + k;
                    indices[indInd] = (((quadTemplExtCirc[k] + j) % nrExtrCirclePoints) +
                                       quadTemplBaseP[k] * nrExtrCirclePoints + i * nrExtrCirclePoints) %
                                      (nrBasePathPoints * nrExtrCirclePoints);
                }

        m_gizVao[vn] = make_unique<VAO>("position:3f,normal:3f", GL_STATIC_DRAW);
        m_gizVao[vn]->upload(CoordType::Position, &positions[0], nrVert);
        m_gizVao[vn]->upload(CoordType::Normal, &normals[0], nrVert);
        m_gizVao[vn]->setElemIndices(m_totNrIndices, &indices[0]);
    }
#endif
}

#ifdef GIZMO_ROT_SHADER_ONLY
void SNGizmoRotAxis::initShader() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n out vec2 tex_coord;
        out vec2 rawPos; void main() {
            \n tex_coord = texCoord;
            \n rawPos    = position.xy;
            gl_Position  = position;
            \n
        });

    vert = m_shCol->getShaderHeader() + "// SNGizmoRotAxis vert\n" + vert;

    std::string frag = STRINGIFY(
        in vec2 tex_coord; in vec2 rawPos; uniform ivec2 iResolution; \n uniform vec4 color; \n

            uniform mat4 projview;
        uniform mat4 invprojview; uniform float near; uniform float far;

        layout(location = 0) out vec4 fragColor; \n

                                                 const int AA = 2;

        float sdTorus(vec3 p, float rad, float thick) { return length(vec2(length(p.xz) - rad, p.y)) - thick; }

        vec4 raycast(vec3 ro, vec3 rd) {
            float res = -1.0;
            float t   = 0.0;
            float h   = 0.0;
            vec3  pos;

            for (int i = 0; i < 70; i++) {
                pos = ro + rd * t;
                h   = sdTorus(pos, 0.5, 0.05);
                if (abs(h) < (0.0001 * t)) {
                    res = t;
                    break;
                }
                t += h;
            }

            return vec4(res, pos);
        }

        void main() {
            vec3 rayOrigin = (invprojview * vec4(rawPos, -1.0, 1.0) * near).xyz;
            vec3 rayDirection;
            vec2 o;
            vec2 p;
            vec4 res;

            float tot = 0.0;
            for (int m = 0; m < AA; m++)
                for (int n = 0; n < AA; n++) {
                    // pixel coordinates
                    o = vec2(float(m), float(n)) / float(AA) - 0.5;
                    p = (2.0 * (gl_FragCoord.xy + o) - iResolution.xy) / iResolution.y;

                    rayDirection = normalize((invprojview * vec4(p * (far - near), far + near, far - near)).xyz);

                    res = raycast(rayOrigin, rayDirection);
                    tot += clamp(res.x, 0.0, 1.0);
                }

            tot /= float(AA * AA);
            fragColor = vec4(tot) * color;

            // manually write depth frag
            vec4 worldP = projview * vec4(res.yzw, 1.0);
            worldP /= worldP.w;
            gl_FragDepth = tot > 0.5 ? worldP.z : 1.0;
        });

    frag = m_shCol->getShaderHeader() + "// SNGizmoRotAxis frag\n" + frag;

    m_torusShader = m_shCol->add("SNGizmoRotAxis", vert, frag);
}
#endif

void SNGizmoRotAxis::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (pass == renderPass::gizmo || pass == renderPass::objectMap) {
#ifdef GIZMO_ROT_SHADER_ONLY
        // init shaders
        if (!m_torusShader) {
            m_shCol = &m_glbase.shaderCollector();
            m_quad  = sceneData::inst()->getStdQuad();
            initShader();
        }

        if (!m_rotMatInited) {
            m_gRot         = glm::rotate(float(M_PI) * -0.5f, vec3(1.f, 0.f, 0.f));
            m_rotMatInited = true;
        }

        m_locPvm = s_cs->getProjectionMatr() * s_cs->getModelMatr() * *getModelMat() * m_gRot;
        m_invCamMat = glm::inverse(m_locPvm) * glm::scale(vec3(s_cs->getActFboSize()->y / s_cs->getActFboSize()->x, 1.f, 1.f));

        m_torusShader->begin();
        m_torusShader->setUniformMatrix4fv("invprojview", &m_invCamMat[0][0]);
        m_torusShader->setUniformMatrix4fv("projview", &m_locPvm[0][0]);
        m_torusShader->setUniform2i("iResolution", s_cs->getActFboSize()->x, s_cs->getActFboSize()->y);
        m_torusShader->setUniform4f("color", gColor.r * emisBright, gColor.g * emisBright, gColor.b * emisBright, 1.f);
        m_torusShader->setUniform1f("near", s_cs->getNear());
        m_torusShader->setUniform1f("far", s_cs->getFar());

        m_quad->draw();
        _shader->begin();
#else

        // material
        shader->setUniform1i("hasTexture", 0);
        shader->setUniform1i("lightMode", 0);
        shader->setUniform4f("ambient", 0.f, 0.f, 0.f, 0.f);
        shader->setUniform4f("emissive", m_gColor.r * m_emisBright, m_gColor.g * m_emisBright, m_gColor.b * m_emisBright,
                             m_gColor.a * m_emisBright);
        shader->setUniform4fv("diffuse", value_ptr(m_gColor));
        shader->setUniform4f("specular", 1.f, 1.f, 1.f, 1.f);
        shader->setUniform1i("drawGridTexture", 0);

        m_gizVao[pass == renderPass::objectMap ? 1 : 0]->drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, m_totNrIndices);
#endif
    }
}

}  // namespace ara