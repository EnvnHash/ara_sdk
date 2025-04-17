#include "Shaders/ShaderPrototype/SPObjectSelector.h"

#include "CameraSets/CameraSet.h"
#include "SceneNodes/SNGridFloor.h"

using namespace glm;
using namespace std;

#define SPOBJSEL_ROT_SIMPLE  // don't try to rotate along the mouse movement,
                             // instead simplify to either up/down or left/right

namespace ara {

SPObjectSelector::SPObjectSelector(sceneData* sd) : ShaderProto(sd) {
    s_name = getTypeName<SPObjectSelector>();

    if (!s_sd) return;
    s_scrWidth  = (int)s_sd->winViewport.z;
    s_scrHeight = (int)s_sd->winViewport.w;

#ifdef ARA_USE_GLES31
    m_resFbo = make_unique<FBO>(sd->glbase, 1, 1, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
#else
    m_resFbo = make_unique<FBO>(sd->glbase, 1, 1, GL_R32F, GL_TEXTURE_1D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
#endif
    m_resFbo->setMinFilter(GL_NEAREST);
    m_resFbo->setMagFilter(GL_NEAREST);

    m_point        = make_unique<VAO>("position:4f", GL_STATIC_DRAW);
    GLfloat pos[4] = {0.f, 0.f, 0.f, 1.f};
    m_point->upload(CoordType::Position, &pos[0], 1);

    initDepthGenShader(1);
    initDepthReadShader();
}

void SPObjectSelector::buildFbo(uint nrCameras) {
    if (s_scrWidth != 0 && s_scrHeight != 0) {
        if (m_fbo) {
            m_fbo->remove();
            m_fbo.reset();
        }

        m_fbo = make_unique<FBO>(s_glbase, s_scrWidth, s_scrHeight, nrCameras, GL_R32F, GL_TEXTURE_2D_ARRAY, true, 1, 1,
                                 1, GL_CLAMP_TO_EDGE, false);
        m_fbo->setMinFilter(GL_NEAREST);
        m_fbo->setMagFilter(GL_NEAREST);
    }
}

void SPObjectSelector::initDepthGenShader(uint nrCameras) {
    std::string shdrName = "SPObjectSelector_gen_" + std::to_string(nrCameras);
    if (s_shCol->hasShader(shdrName)) {
        s_shader = s_shCol->get(shdrName);
        return;
    }

    // if (s_shader) m_shCol->deleteShader("SPObjectSelector_gen");

    std::string vert = s_shCol->getShaderHeader();
    vert +=
        "// SPObjectSelector_gen Prototype\n"
        "layout(location = 0) in vec4 position; \n"
        "uniform mat4 " +
        getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
        "; \n"
        "void main() { \n"
        "gl_Position = " +
        getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
        " * position; \n"
        "}";

    std::string geom = s_shCol->getShaderHeader() + "layout(triangles, invocations=" + to_string(nrCameras) +
                       ") in;\n"
                       "layout(triangle_strip, max_vertices=3) out;\n"
                       "uniform mat4 " +
                       getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] + "[" + to_string(nrCameras) +
                       "];\n"
                       "uniform mat4 " +
                       getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[" + to_string(nrCameras) +
                       "];\n"
                       "void main() {\n"
                       "for (int i = 0; i < gl_in.length(); i++) \n"
                       "{ \n"
                       "gl_Layer = gl_InvocationID; \n"
                       "gl_Position = " +
                       getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[gl_InvocationID] * " +
                       getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] +
                       "[gl_InvocationID] * gl_in[i].gl_Position; \n"
                       "EmitVertex(); \n"
                       "}\n "
                       "EndPrimitive(); \n"
                       "}";

    std::string frag = s_shCol->getShaderHeader();
    frag += "// SPObjectSelector Light Prototype\n";

    frag += STRINGIFY(
        uniform float objID; \n out vec4 fragColor; \n void main() { fragColor = vec4(objID, 0.0, 0.0, 1.0); });

    if (nrCameras > 1)
        s_shader = s_shCol->add(shdrName, vert, geom, frag);
    else
        s_shader = s_shCol->add(shdrName, vert, frag);
}

// TODO: expand to arbitray number of layers, at the moment, only use first
// layer
void SPObjectSelector::initDepthReadShader() {
    std::string vert = s_shCol->getShaderHeader();
    vert += "// SPObjectSelector_read Prototype\n";

    vert += STRINGIFY(layout(location = 0) in vec4 position; \n void main() {
        \n gl_Position = position;
        \n
    });

    std::string frag = s_shCol->getShaderHeader();
    frag += "// SPObjectSelector_read Prototype\n";

#ifdef ARA_USE_GLES31
    frag += STRINGIFY(highp uniform sampler2D objMap; uniform ivec2 mousePos;\n out vec4 fragColor; \n void main() {
        fragColor = texelFetch(objMap, mousePos, 0);
        \n
    });
#else
    // NOTE: highp
    // frag += STRINGIFY(highp uniform sampler2DArray objMap;
    frag += STRINGIFY(uniform sampler2DArray objMap; uniform ivec2 mousePos;\n out vec4 fragColor; \n void main() {
        fragColor = texelFetch(objMap, ivec3(mousePos, 0), 0);
        \n
    });
#endif

    m_read_shader = s_shCol->add("SPObjectSelector_read", vert, frag);
}

bool SPObjectSelector::zoomSelectedObject(trackballPreset* camPreset, bool toObjCenter, bool adjRotation) {
    if (m_selectedObjectNode && m_cs) {
        zoomToBounds(camPreset, *m_selectedObjectNode->getBoundingBoxMin(), *m_selectedObjectNode->getBoundingBoxMax(),
                     *m_selectedObjectNode->getTransVec(), toObjCenter, adjRotation);
        return true;
    }

    return false;
}

void SPObjectSelector::zoomToBounds(trackballPreset* camPreset, vec3& bbMin, vec3& bbMax, glm::vec3& transOffs,
                                    bool toObjCenter, bool adjRotation) {
    // calculate the distance the object has to have to the camera to cover the
    // whole width
    m_tempBoxSize = bbMax - bbMin;

    float fovX      = m_cs->getFov() * (m_cs->getActFboSize()->x / m_cs->getActFboSize()->y);
    vec3  distToCam = vec3(0.f, 0.f, 0.f);

    if (!toObjCenter) {
        if (m_tempBoxSize.x > m_tempBoxSize.y)
            distToCam.z = ((m_tempBoxSize.x * 0.5f) / std::tan(fovX * 0.5f));
        else
            distToCam.z = ((m_tempBoxSize.y * 0.5f) / std::tan(m_cs->getFov() * 0.5f));

        distToCam.z += m_tempBoxSize.z * 0.75f;

        // get cam view dir
        vec3 viewDir = vec3(glm::eulerAngleXYZ(camPreset->rotEuler.x, camPreset->rotEuler.y, camPreset->rotEuler.z) *
                            vec4(0.f, 0.f, -1.f, 0.f));
        distToCam    = viewDir * distToCam.z;
    } else {
        distToCam.z = -1.f;
    }

    if (adjRotation && m_selectedObjectNode) {
        quat nodeRotQuat    = glm::angleAxis(m_selectedObjectNode->getRotAngle(), *m_selectedObjectNode->getRotAxis());
        camPreset->rotEuler = eulerAngles(nodeRotQuat);
        distToCam           = nodeRotQuat * vec3(0.f, 0.f, 1.f);
    }

    // the camera has to move the scene into the opposite direction
    camPreset->pos = transOffs - distToCam;
    // camPreset->pos = *m_selectedObjectNode->getTransVec() - distToCam;
}

void SPObjectSelector::mouseDownLeft(float x, float y, SceneNode* sceneTree) {
    if (!s_sd || !m_cs) return;

    m_mouseDown.x   = x * s_sd->contentScale.x;
    m_mouseDown.y   = y * s_sd->contentScale.y;
    m_mouseDownNorm = vec2(m_mouseDown.x / m_cs->getActFboSize()->x * 2.f - 1.f,
                           (1.f - m_mouseDown.y / m_cs->getActFboSize()->y) * 2.f - 1.f);
    m_mouseDragged  = false;
    m_gizmoDragged  = false;

    m_lastSceneTree = sceneTree;
    m_lastGizmoTree = sceneTree->getNode("gizmoTree");
    m_lastSceneData = s_sd;

    // don't download the whole depthmap for performance reasons
    // just emit one point at the position of the mouse and write the color
    // at that point of the Depth Map into a 2x2 Texture
    m_resFbo->bind();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
#ifndef ARA_USE_GLES31
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif

    m_read_shader->begin();
    m_read_shader->setUniform1i("objMap", 0);
    m_read_shader->setUniform2i("mousePos", (int)m_mouseDown.x, (int)s_scrHeight - (int)m_mouseDown.y);

    glActiveTexture(GL_TEXTURE0);
#ifdef ARA_USE_GLES31
    glBindTexture(GL_TEXTURE_2D, m_fbo->getColorImg());
#else

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_fbo->getColorImg());
#endif

    m_point->draw(GL_POINTS);

    m_read_shader->end();
    m_resFbo->unbind();

    //------------------------------------------------------------

    m_readVal = 0.f;
#ifdef ARA_USE_GLES31
    glesGetTexImage(m_resFbo->getColorImg(), GL_TEXTURE_2D, GL_RED, GL_FLOAT, 1, 1, (GLubyte*)&m_readVal);
#else
    glBindTexture(GL_TEXTURE_1D, m_resFbo->getColorImg());
    glGetTexImage(GL_TEXTURE_1D, 0, GL_RED, GL_FLOAT, (void*)&m_readVal);
#endif

    // cast the texture pixel value to an integer
    if (m_readVal >= 0.f) selectObj(static_cast<int>(m_readVal), false);
}

void SPObjectSelector::mouseUpLeft(SceneNode* sceneTree) {
    // if we were dragging, save the new transVector of the moved object and of
    // the gizmo
    if (m_selectedObjectNode && m_gizmoDragged && s_sd) {
        m_mouseDownObjTransVec = *m_selectedObjectNode->getTransVec();
        m_mouseDownObjScaleVec = *m_selectedObjectNode->getScalingVec();
        m_mouseDownObjRot      = angleAxis(m_selectedObjectNode->getRotAngle(), *m_selectedObjectNode->getRotAxis());

        auto parentNode = m_selectedNode->getParentNode(m_cs->s_selectObj);
        if (parentNode) {
            m_gizmoMouseDownTransVec = *parentNode->getTransVec();
            m_gizmoMouseDownRot      = angleAxis(parentNode->getRotAngle(), *parentNode->getRotAxis());
            m_gizmoMouseDownScaleVec = *parentNode->getScalingVec();
        }

        s_sd->reqRenderPasses->at(GLSG_OBJECT_MAP_PASS) = true;
        s_sd->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
    }

    // cast the texture pixel value to an integer
    if (!m_mouseDragged) {
        if (m_readVal >= 0.f)
            selectObj(static_cast<int>(m_readVal), true);
        else {
            if (s_sd && s_sd->deselectAll) s_sd->deselectAll();
        }
    }

    m_mouseDragged = false;
    m_gizmoDragged = false;
    m_state        = objSelState::idle;
}

void SPObjectSelector::mouseUpRight(SceneNode* sceneTree) { m_state = objSelState::idle; }

/**
 * 	if an Object was selected, translate it in object space
 */
void SPObjectSelector::mouseMove(float x, float y) {
    if (!s_sd) return;

    vec3       offsetVec;
    float      mouseMovedSign = 0;
    SceneNode* gizmoNode      = nullptr;
    vec2       mouseScaled{x * s_sd->contentScale.x, y * s_sd->contentScale.y};
    m_mouseDragged = !glm::all(glm::equal(m_mouseDown, mouseScaled));

    // if there is no active object, don't continue
    if (m_readVal < 0.f) return;

    // 3d space coordinates multiplied by the pvm and divided by the w-component
    // of its result is NDC (Normalized Device Space) which reaches from
    // (-1|-1|-1) to (1|1|1) for opengl A point in 3d space at (0|0|camPos.z -
    // nearPlane) will result in (0|0|-1) A point in 3d space at (0|0|camPos.z -
    // nearPlane - farPlane) will result in (0|0|1) (in standard opengl
    // coordinates space, -z is towards the screen)

    if (m_cs && m_selectedNode) gizmoNode = m_selectedNode->getParentNode(m_cs->s_selectObj);

    if (m_gizmoselected != 0 && m_selectedNode && gizmoNode && m_selectedObjectNode) {
        g_norm_center = vec4(0.f, 0.f, 0.f, 1.f);

        if (gizmoNode->m_nameFlag & GLSG_TRANS_GIZMO) {
            g_norm_end = vec4(float((m_gizmoselected & GLSG_TRANS_GIZMO_X) != 0),
                              float((m_gizmoselected & GLSG_TRANS_GIZMO_Y) != 0),
                              float((m_gizmoselected & GLSG_TRANS_GIZMO_Z) != 0), 1.f);
        } else if (gizmoNode->m_nameFlag & GLSG_SCALE_GIZMO) {
            g_norm_end = vec4(float((m_gizmoselected & GLSG_SCALE_GIZMO_X) != 0),
                              float((m_gizmoselected & GLSG_SCALE_GIZMO_Y) != 0),
                              float((m_gizmoselected & GLSG_SCALE_GIZMO_Z) != 0), 1.f);
        } else if (gizmoNode->m_nameFlag & GLSG_ROT_GIZMO) {
            g_norm_end =
                vec4(float((m_gizmoselected & GLSG_ROT_GIZMO_X) != 0), float((m_gizmoselected & GLSG_ROT_GIZMO_Y) != 0),
                     float((m_gizmoselected & GLSG_ROT_GIZMO_Z) != 0), 1.f);
        }

        //----------------------------------------------------------------

        calcGizmoAxisNDC();
        calcMouseMoveVec(mouseScaled.x, mouseScaled.y, &mouseMovedSign);
        calcIntersectPlane(mouseScaled.x, mouseScaled.y, (gizmoNode->m_nameFlag & GLSG_ROT_GIZMO) > 0);

        if ((gizmoNode->m_nameFlag & GLSG_TRANS_GIZMO) || (gizmoNode->m_nameFlag & GLSG_SCALE_GIZMO))
            calcOffsVecTransAndScale(&offsetVec, &mouseMovedSign);

        //----------------------------------------------------------------

        if (gizmoNode->m_nameFlag & GLSG_ROT_GIZMO) {
            float offsAngle = 0.f;

#ifndef SPOBJSEL_ROT_SIMPLE
            // take the selected gizmo axis as the normal of a plane
            // if this plane is orthogonal to the screen, we can't calculate the
            // z-position of the mouseDown and have to estimate the desired
            // rotation
            vec3 g_axis_plane_vec = normalize(vec3(g_norm_axis_mod - g_norm_center_mod));

            // calculate angle to screen normal
            vec3 screenNorm = vec3(0.f, 0.f, 1.f);

            float g_axis_plane_screen_angle = glm::angle(g_axis_plane_vec, screenNorm);
            if (g_axis_plane_screen_angle > float(HALF_PI))
                g_axis_plane_screen_angle = float(HALF_PI) - std::fmod(g_axis_plane_screen_angle, float(HALF_PI));

            m_skipMoveAnalogToMouse = g_axis_plane_screen_angle < 0.2f;

            vec4 mouseDown3D{(m_mouseDown.x / m_cs->getActFboSize()->x) * 2.f - 1.f,
                             (1.f - (m_mouseDown.y / m_cs->getActFboSize()->y)) * 2.f - 1.f, -1.f, 1.f};

            vec4 actMousePos{(x / m_cs->getActFboSize()->x) * 2.f - 1.f,
                             (1.f - (y / m_cs->getActFboSize()->y)) * 2.f - 1.f, -1.f, 1.f};

            if (!m_skipMoveAnalogToMouse) {
                vec3 gizmoCenterCam      = vec3(m_gizmo_camModelMat * g_norm_center);
                vec3 planeNormalCamSpace = normalize(vec3(m_gizmo_camModelMat * vec4(m_planeNormal, 0.f)));

                // get the Position of the MouseDown int 3D Space
                vec4 unProjMouseDown =
                    unProjectMouse(mouseDown3D, m_cs->getCamPos(), gizmoCenterCam, planeNormalCamSpace);

                // remove gizmo and camera transformations
                unProjMouseDown = inverse(m_gizmo_camModelMat) * unProjMouseDown;

                vec4 unProjActMouse =
                    unProjectMouse(actMousePos, m_cs->getCamPos(), gizmoCenterCam, planeNormalCamSpace);

                // remove gizmo transformations
                unProjActMouse = inverse(m_gizmo_camModelMat) * unProjActMouse;

                // get angle relative to gizmo center
                offsAngle = orientedAngle(normalize(vec3(unProjMouseDown)), normalize(vec3(unProjActMouse)),
                                          normalize(m_planeNormal));

                if (fine) offsAngle *= 0.1f;
            } else {
#endif
                // take the gizmo vector mapped into NDC and check if it is
                // oriented more horizontally or more vertically increment
                // rotation around that angle according to the corresponding
                // mouse move vector component
                // float angleToNdcX = std::fabs(glm::orientedAngle(vec2(1.f,
                // 0.f), vec2(m_gizmoMoveVec2D))); bool horMouseMapping =
                // m_skipMoveAnalogToMouse || (angleToNdcX > float(HALF_PI) *
                // 0.5f && angleToNdcX < float(HALF_PI) * 1.5f);
                // offsAngle = horMouseMapping ? m_mouseMoved2D.x :
                // m_mouseMoved2D.y;
                offsAngle = m_mouseMoved2D.x;

#ifndef SPOBJSEL_ROT_SIMPLE
            }
#endif

            if (m_cfState == cfState::fine) offsAngle *= 0.1f;

            // rotate the object
            mat4 newRot     = toMat4(m_mouseDownObjRot) * rotate(offsAngle, m_planeNormal);
            quat newRotQuat = toQuat(newRot);
            m_selectedObjectNode->rotate(angle(newRotQuat), glm::axis(newRotQuat));

            newRot     = toMat4(m_gizmoMouseDownRot) * rotate(offsAngle, m_planeNormal);
            newRotQuat = toQuat(newRot);
            gizmoNode->rotate(angle(newRotQuat), glm::axis(newRotQuat));

            m_gizmoDragged = length(offsAngle) > m_mouseDragToleranceRot;
            m_state        = objSelState::rotating;
        } else if (gizmoNode->m_nameFlag & GLSG_TRANS_GIZMO) {
            if (m_cfState == cfState::fine) offsetVec *= 0.1f;

            // offset the gizmo
            m_newTransVec = m_gizmoMouseDownTransVec + offsetVec;
            gizmoNode->translate(&m_newTransVec);

            // offset the object to move, must be same offset
            m_newTransVec = m_mouseDownObjTransVec + offsetVec;
            m_selectedObjectNode->translate(&m_newTransVec);

            m_gizmoDragged = length(offsetVec) > m_mouseDragToleranceTrans;
            m_state        = objSelState::translating;
        } else if (gizmoNode->m_nameFlag & GLSG_SCALE_GIZMO && !(m_selectedObjectNode->m_transFlag & GLSG_NO_SCALE) &&
                   !m_skipMoveAnalogToMouse) {
            if (m_cfState == cfState::fine) offsetVec *= 0.1f;

            // the mouse movement is interpreted as the difference between the
            // limits of the scaled and unscaled bounding box of the object at
            // the selected axis
            ivec3 scaleAxisIndex{m_gizmoselected & GLSG_SCALE_GIZMO_X ? 1 : 0,
                                 m_gizmoselected & GLSG_SCALE_GIZMO_Y ? 1 : 0,
                                 m_gizmoselected & GLSG_SCALE_GIZMO_Z ? 1 : 0};

            vec3 maxAxis{m_selectedObjectNode->getBoundingBoxMax()->x - m_selectedObjectNode->getBoundingBoxMin()->x,
                         m_selectedObjectNode->getBoundingBoxMax()->y - m_selectedObjectNode->getBoundingBoxMin()->y,
                         m_selectedObjectNode->getBoundingBoxMax()->z - m_selectedObjectNode->getBoundingBoxMin()->z};

            ivec3 checkScalingPosible;
            for (short i = 0; i < 3; i++) checkScalingPosible[i] = scaleAxisIndex[i] > 0 && maxAxis[i] == 0.f;

            maxAxis *= m_mouseDownObjScaleVec;

            if (compAdd(checkScalingPosible) == 0) {
                // if we are scaling in a plane, take the mouseMovesSign instead
                // of the offsetSign
                if (compAdd(g_norm_end) == 3.f) m_offsetSign = mouseMovedSign;

                int scaleAxisIndex = m_gizmoselected & GLSG_SCALE_GIZMO_X   ? 0
                                     : m_gizmoselected & GLSG_SCALE_GIZMO_Y ? 1
                                     : m_gizmoselected & GLSG_SCALE_GIZMO_Z ? 2
                                                                            : 0;

                float newDim                = maxAxis[scaleAxisIndex] + length(offsetVec) * m_offsetSign * 2.f;
                vec3  newScaleVec           = m_mouseDownObjScaleVec;
                newScaleVec[scaleAxisIndex] = newDim / ((*m_selectedObjectNode->getBoundingBoxMax())[scaleAxisIndex] -
                                                        (*m_selectedObjectNode->getBoundingBoxMin())[scaleAxisIndex]);

                if (newScaleVec[scaleAxisIndex] > 0.f) {
                    float scaleOffsetAmt = newScaleVec[scaleAxisIndex] / m_mouseDownObjScaleVec[scaleAxisIndex];

                    // if the GLSG_PROPO_SCALE flag is set, all dimensions must
                    // have the same scaling offset
                    if (m_selectedObjectNode->m_transFlag & GLSG_PROPO_SCALE) {
                        // apply this offset to all axis, if the dimension in
                        // this axis is > 0
                        for (uint i = 0; i < 3; i++)
                            if (((*m_selectedObjectNode->getBoundingBoxMax())[i] -
                                 (*m_selectedObjectNode->getBoundingBoxMin())[i]) > 0.f)
                                newScaleVec[i] = m_mouseDownObjScaleVec[i] * scaleOffsetAmt;
                            else
                                newScaleVec[i] = 1.f;

                        // scaling in a plane, take one of the axes and set the
                        // scaling into the other axis accordingly
                    } else if (compAdd(g_norm_end) == 3.f) {
                        uint64_t axes[3] = {GLSG_SCALE_GIZMO_X, GLSG_SCALE_GIZMO_Y, GLSG_SCALE_GIZMO_Z};

                        for (uint i = 0; i < 3; i++)
                            if ((((*m_selectedObjectNode->getBoundingBoxMax())[i] -
                                  (*m_selectedObjectNode->getBoundingBoxMin())[i]) > 0.f) &&
                                (m_gizmoselected & axes[i]))
                                newScaleVec[i] = m_mouseDownObjScaleVec[i] * scaleOffsetAmt;
                            else
                                newScaleVec[i] = 1.f;
                    }
                    m_selectedObjectNode->scale(&newScaleVec);
                }
            }

            m_gizmoDragged = length(offsetVec) > 0.f;
            m_state        = objSelState::scaling;
        }

        m_lastSceneData->reqRenderPasses->at(GLSG_OBJECT_MAP_PASS) = true;
        m_lastSceneData->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
    }
}

void SPObjectSelector::keyDown(hidData* data) {
    if (data->key == GLSG_KEY_LEFT_CONTROL || data->key == GLSG_KEY_RIGHT_CONTROL) m_cfState = cfState::fine;

    if (data->key == GLSG_KEY_LEFT_SHIFT || data->key == GLSG_KEY_LEFT_SHIFT) m_cfState = cfState::coarse;

    // if there is a selected object (that is a gizmo placed ontop of it) switch
    // the selected axis first rotate through the simple axes, then in case of a
    // translation gizmo, through the planes
    if (m_selectedObjectNode && m_cs && m_cs->s_activeGizmo && data->key == GLSG_KEY_TAB) {
        if (data->shiftPressed)
            m_cs->s_activeGizmo->selectPrevAxis();
        else
            m_cs->s_activeGizmo->selectNextAxis();
    }
}

void SPObjectSelector::keyUp(hidData* data) {
    if (data->key == GLSG_KEY_LEFT_CONTROL || data->key == GLSG_KEY_RIGHT_CONTROL || data->key == GLSG_KEY_LEFT_SHIFT ||
        data->key == GLSG_KEY_LEFT_SHIFT)
        m_cfState = cfState::normal;
}

// is called from the opengl context and the event-queue
void SPObjectSelector::selectObj(int inObjId, bool onMouseUp) {
    int objId = inObjId;

    if (m_lastSceneTree && objId) {
        // get the corresponding SceneNode
        m_selectedNode = m_lastSceneTree->getNodeWithID(objId);
        if (!onMouseUp && (!m_selectedNode || (m_selectedNode && !m_selectedNode->dragSelectedAllowed()))) return;

        int outVal      = -1;
        m_gizmoselected = 0;  // reset the gizmo-selected-flag

        // if there is a selected object, deselect it
        if (m_selectedObjectNode) m_selectedObjectNode->setSelected(false);

        if (m_selectedNode->getName() == getTypeName<SNGridFloor>()) {
            if (s_sd && s_sd->deselectAll) s_sd->deselectAll();

            if (m_addGizmoCb) m_addGizmoCb(transMode::none);
        }

        if (m_selectedNode) {
            auto parentNode = m_selectedNode->getParentNode(objId);

            if (parentNode) {
                // if a light object's scene mesh was selected, get its parent
                // object (light objects can't have more the one Node SubLevel)
                if (m_selectedNode->m_nodeType == GLSG_SNT_LIGHT_SCENE_MESH ||
                    m_selectedNode->m_nodeType == GLSG_SNT_CAMERA_SCENE_MESH) {
                    m_selectedNode =
                        m_selectedNode->getIdGroup() ? m_selectedNode : m_selectedNode->getParentNode(objId);
                    // set the objId to the parent Node, in this case there will
                    // be for sure only one
                    objId = m_selectedNode->getObjId(m_selectedNode->getFirstParentNode());
                }
            }

            // if a SceneNode was selected and it was not a Gizmo, save it
            if (!parentNode ||
                (!(parentNode->m_nameFlag & GLSG_TRANS_GIZMO) && !(parentNode->m_nameFlag & GLSG_SCALE_GIZMO) &&
                 !(parentNode->m_nameFlag & GLSG_ROT_GIZMO))) {
                m_selectedObjectNode = m_selectedNode;
            }

            // if the SceneNode was already selected, deselect it
            if ((objId == m_cs->s_selectObj && m_selectedNode->m_nodeType != GLSG_GIZMO) ||
                !m_selectedNode->m_selectable) {
                outVal = -1;
                if (m_selectedNode) {
                    m_selectedNode->setSelected(false);
                    m_selectedNode->deleteModelMatChangedCb(this);
                }

                // on selecting a SceneNode, a gizmo is always placed ontop of
                // it, so remove it, when deselecting
                if (m_cs->s_activeGizmo && m_lastGizmoTree) {
                    m_cs->s_activeGizmo->setVisibility(false);
                    if (m_addGizmoCb) m_addGizmoCb(transMode::none);
                    m_cs->s_activeGizmo = nullptr;
                }

                m_cs->s_selectObj = -2;                // set to something obviously invalid, but not -1,
                                                       // because objId could be the same and so this if
                                                       // clause would work in a contrary way
            } else if (objId != -1 && m_selectedNode)  // if we have a valid ObjectID
            {
                // get the center of the SceneNode and add a Gizmo

                // check if the selected node is part of an id group, if this is
                // the case, take the owner's id
                outVal            = m_selectedNode->getObjId();
                m_cs->s_selectObj = outVal;

                // if the parentnode of the selectedNode is a gizmo, then an
                // axis or plane of a gizmo was selected for dragging
                if (parentNode &&
                    ((parentNode->m_nameFlag & GLSG_TRANS_GIZMO) || (parentNode->m_nameFlag & GLSG_SCALE_GIZMO) ||
                     (parentNode->m_nameFlag & GLSG_ROT_GIZMO))) {
                    // if there was an SceneNode selected, before the gizmo was
                    // clicked, reselect this SceneNode
                    if (m_selectedObjectNode && !m_selectedObjectNode->setSelected(true)) return;

                    m_gizmoselected |= m_selectedNode->m_nameFlag;
                    m_gizmoMouseDownTransVec = *parentNode->getTransVec();
                    m_gizmoMouseDownScaleVec = *parentNode->getScalingVec();
                    m_gizmoMouseDownRot      = angleAxis(parentNode->getRotAngle(), *parentNode->getRotAxis());
                    m_gizmoMouseDownModelMat = *parentNode->getModelMat();

                    // deselect all parts of the gizmo
                    parentNode->setSelected(false);
                    m_selectedNode->setSelected(true);

                    return;
                }

                // if an object was clicked, save the translation vector of the
                // selected object (we do it here right after the MouseDown)
                m_mouseDownObjTransVec = *m_selectedNode->getTransVec();
                m_mouseDownObjScaleVec = *m_selectedNode->getScalingVec();
                m_mouseDownObjRot      = angleAxis(m_selectedNode->getRotAngle(), *m_selectedNode->getRotAxis());

                // in case the SceneNode is not selectable, don't add the gizmo
                if (m_selectedNode->setSelected(true)) addGizmo(m_actTransMode);
            }

            m_cs->s_selectObj = outVal;

        } else
            LOGE << " ERROR SPObjectSelector:: selectedNode returned from "
                    "getNodeWithID("
                 << objId << ") is nullptr";
    }
}

/**
 * called during a Object dragging. Calculates Vector by which the Gizmo will be
 * moved in NDC space (as it appears on the screen)
 */
void SPObjectSelector::calcGizmoAxisNDC() {
    m_gizmo_camModelMat = m_cs->getModelMatr() * m_gizmoMouseDownModelMat;
    m_gizmo_projViewMat = m_cs->getProjectionMatr() * m_cs->getViewMatr();
    m_invVP             = inverse(m_gizmo_projViewMat);

    // apply the gizmo offset and the camera transformation to the gizmo
    // axis-vector
    g_norm_center_mod = m_gizmo_camModelMat * g_norm_center;
    g_norm_axis_mod   = m_gizmo_camModelMat * g_norm_end;

    // project the gizmo center and the end of the selectes axis into NDC
    m_gizmoCenter2D = m_gizmo_projViewMat * g_norm_center_mod;
    m_gizmoCenter2D /= m_gizmoCenter2D.w;

    m_gizmoAxis2D = m_gizmo_projViewMat * g_norm_axis_mod;
    m_gizmoAxis2D /= m_gizmoAxis2D.w;

    // calculate the direction vector of the gizmo, projected into NDC
    m_gizmoMoveVec2D = m_gizmoAxis2D - m_gizmoCenter2D;

    // calculate the length of the gizmo axis vector on the screen, if it is too
    // short, it will get impossible to calculate a reasonable movement from it.
    m_gizmoDragAxisLength = length(vec2(m_gizmoMoveVec2D.x, m_gizmoMoveVec2D.y));

    m_skipMoveAnalogToMouse = m_gizmoDragAxisLength < 0.04f;

    m_gizmoMoveVec2D   = m_gizmoAxis2D - m_gizmoCenter2D;
    m_gizmoMoveVec2D.z = 0.f;
    m_gizmoMoveVec2D.w = 0.f;
    m_gizmoMoveVec2D   = glm::normalize(m_gizmoMoveVec2D);
}

/**
 * called during a Object dragging. Calculates the mouse Offset Vector in NDC
 * space (as it appears on the screen)
 */
void SPObjectSelector::calcMouseMoveVec(float x, float y, float* mouseMovedSign) {
    // get the vector of the MouseDown StartPosition and the actual Position
    // in normalized screen coordinates
    m_mouseMoved2D = vec2((x - m_mouseDown.x) / m_cs->getActFboSize()->x * 2.f,
                          -(y - m_mouseDown.y) / m_cs->getActFboSize()->y * 2.f);

    *mouseMovedSign = std::atan2(m_mouseMoved2D.y, m_mouseMoved2D.x) / float(M_PI);
    if (*mouseMovedSign > 0.f) {
        if (*mouseMovedSign > 0.5f) *mouseMovedSign = 1.f - *mouseMovedSign;
    } else {
        if (*mouseMovedSign < -0.5f) *mouseMovedSign = (*mouseMovedSign + 1.f) * -1.f;
    }
    *mouseMovedSign *= 2.f;
}

/**
 * called during a Object dragging. Calculates the plane, which contains the
 * gizmo axis, we are moving along and which is most parallel to the screen
 * plane
 */
void SPObjectSelector::calcIntersectPlane(float x, float y, bool skipPlaneSorting) {
    bool singleAxisTrans = compAdd(g_norm_end) == 2.f;

    // select the planenormal for intersecting with the calculated ray we need
    // the most parallel plane relative to the screen plane define the screen
    // plane: normal-vector: (m_cs.camLookAt - m_cs.s_camPos) and ref-point
    // m_cs.s_camPos take the gizmo normals transformed by the model mat and
    // calculate their distance to this plane

    if (!skipPlaneSorting) {
        vec3 planeNormals[3] = {vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f)};
        m_sortedNormals.clear();

        for (int i = 0; i < 3; i++) {
            // offset planeNormal by gizmoModelMat
            vec4 planeNormModel = normalize((m_gizmo_camModelMat * vec4(planeNormals[i], 1.f)) - g_norm_center_mod);

            // offset the s_camPos by the planeNormal
            vec3 camPlaneNormModel = m_cs->getCamPos() + vec3(planeNormModel);

            // shoot a ray from this position in the negative direction of the
            // screenplane normal intersect it with the screenplane
            vec3 negScreenNormal = normalize(m_cs->getCamPos() - m_cs->getLookAtPoint());

            float dist = 0.f;
            m_doesInters =
                intersectRayPlane(camPlaneNormModel, negScreenNormal, m_cs->getCamPos(), -negScreenNormal, dist);

            m_sortedNormals[std::fabs(dist)] = planeNormals[i];
        }

        m_planeNormal                 = (*(--m_sortedNormals.end())).second;
        m_planeNormalLength           = (*(--m_sortedNormals.end())).first;
        m_planeWithGizmoAxisMostOrtho = (*(--m_sortedNormals.end())).second;

        if (singleAxisTrans) {
            if (m_planeWithGizmoAxisMostOrtho == vec3(g_norm_end))
                m_planeWithGizmoAxisMostOrtho = (*(--(--m_sortedNormals.end()))).second;

        } else {
            m_planeWithGizmoAxisMostOrtho =
                vec3(g_norm_end.x == 1.f ? 0.f : 1.f, g_norm_end.y == 1.f ? 0.f : 1.f, g_norm_end.z == 1.f ? 0.f : 1.f);
        }

    } else
        m_planeNormal = normalize(vec3(g_norm_end));
}

/**
 * called during a Object dragging (translation, rotation, scaling). Since the
 * movement of the mouse can be in any direction, we need to calculate the
 * relative movement along the desired axis from it.
 */
void SPObjectSelector::calcOffsVecTransAndScale(vec3* offsetVec, float* mouseMovedSign) {
    bool singleAxisTrans = compAdd(g_norm_end) == 2.f;

    if (!m_skipMoveAnalogToMouse) {
        // transformation by a single axis
        if (singleAxisTrans) {
            // calculate a NDC relative move vector with its length
            // corresponding to length of the mouse movement

            // get the angle to rotate the gizmo vector into the x-Axis vector
            // (1, 0, 0) the z-coordinate doesn't matter and will be left out,
            // so make sure that we have a pure 2D-rotation
            m_rotToXAxis =
                RotationBetweenVectors(vec3{m_gizmoMoveVec2D.x, m_gizmoMoveVec2D.y, 0.f}, vec3{1.f, 0.f, 0.f});

            // and also get the inverse rotation
            m_invRotToXAxis =
                RotationBetweenVectors(vec3{1.f, 0.f, 0.f}, vec3{m_gizmoMoveVec2D.x, m_gizmoMoveVec2D.y, 0.f});

            // rotate the mouseMovedVector by the same rotation that moves the
            // gizmo vector into the x-axis vector in NDC Space
            m_rotMouseVec = m_rotToXAxis * vec4(m_mouseMoved2D, 0.f, 0.f);

            m_offsetSign = glm::sign(m_rotMouseVec.x);
            m_offsetSign = m_offsetSign == 0.f ? 1.f : m_offsetSign;

            // now take only the x coordinate of the mousevector and build a new
            // vector pointing in the direction of the x-axis with the length of
            // the x-coordinate, the z-coordinate is unknown
            m_gizmoDstTrans2D   = m_gizmoMoveVec2D * m_rotMouseVec.x;
            m_gizmoDstTrans2D.z = 0.f;
            m_gizmoDstTrans2D.w = 0.f;

        } else
            m_gizmoDstTrans2D = vec4(m_mouseMoved2D, 0.f, 1.f);  // transformation by a plane

        // move center of the gizmo by gizmoDstTrans2D, this should be the
        // destination point in 2d space where we have to move the gizmo in 3D
        // space to
        m_gizmoDst2D = m_gizmoCenter2D + m_gizmoDstTrans2D;

        // now we have the destination position in NDC, but we don't know its 3D
        // Position We can imagine a line from the camera position through the
        // Destination Point in NDC if we take the most parallel plane of the 3
        // gizmos plane with the moved axis as the plane Vector and intersect
        // this plane with the imagined line we got the resulting point in 3D
        // space
        t_planeOrig   = vec3(m_cs->getModelMatr() * vec4(m_gizmoMouseDownTransVec, 1.f));
        t_planeNormal = normalize(
            vec3(m_cs->getModelMatr() * toMat4(m_gizmoMouseDownRot) * vec4(m_planeWithGizmoAxisMostOrtho, 0.f)));
        t_unProjMouseVec = unProjectMouse(m_gizmoDst2D, m_cs->getCamPos(), t_planeOrig, t_planeNormal);

        // transform the found 3d position from camera space to world space
        t_unProjMouseVec = inverse(m_cs->getModelMatr()) * t_unProjMouseVec;

        // calculate the offset vector between the new and the old gizmo axis
        // tip position
        g_center_offset3D = vec3(t_unProjMouseVec) - m_gizmoMouseDownTransVec;

        if (singleAxisTrans) {
            // since this whole calculation may always have a small error, here
            // we make sure, that the movement is strictly in the direction of
            // the normal
            float offsetLength = length(g_center_offset3D);

            vec3 gizmoAxisWorld = normalize(vec3(toMat4(m_gizmoMouseDownRot) * g_norm_end));

            float angle = dot(gizmoAxisWorld, normalize(g_center_offset3D));
            angle       = glm::sign(angle);

            offsetLength *= angle;
            *offsetVec = gizmoAxisWorld * offsetLength;

        } else {
            *offsetVec = g_center_offset3D;
        }

    } else {
        // since the axis selected to move is pointing towards (or directly)
        // from the screen we can�t estimate the 3d position of the offset by
        // crossing a line through a plane alternatively we will take the mouse
        // movement and interpret up towards the screen and down as outwards the
        // screen. we will interpret the movement as a 3d movement of the gizmo
        // center in 3d space and calculate the length of the resulting offset.
        // then we move the gizmo and the object by this amount in the direction
        // of the selected axis

        // offset the gizmo center by the movement of the mouse in NDC
        vec4 gizmoOffs2D = m_gizmoCenter2D + vec4(m_mouseMoved2D, 0.f, 0.f);

        // unproject this point into the 3D space
        mat4 invVP  = inverse(m_gizmo_projViewMat);
        gizmoOffs2D = invVP * gizmoOffs2D;
        gizmoOffs2D /= gizmoOffs2D.w;

        // remove the camera trackball manipulations
        vec4 gizmoOffs2DNoCam    = inverse(m_cs->getViewMatr()) * gizmoOffs2D;
        vec4 g_norm_center_NoCam = inverse(m_cs->getViewMatr()) * g_norm_center_mod;

        // calculate the length of the offset from gizmo center to gizmoOffs2D
        vec3  g_offs_3D_space  = vec3(gizmoOffs2DNoCam - g_norm_center_NoCam);
        float g_offs_3D_length = length(g_offs_3D_space) * *mouseMovedSign;

        // now that we have an offset that it consistent with the cameraview
        // (always same relations) we can add a multplier for convenience
        g_offs_3D_length *= 10.f;
        vec4 g_offs_diff = inverse(m_cs->getModelMatr()) * (g_norm_axis_mod - g_norm_center_mod);

        *offsetVec = g_offs_diff * g_offs_3D_length;
    }
}

// everything has to be in camera space
vec4 SPObjectSelector::unProjectMouse(vec4& inPoint, vec3 camPos, vec3& planeOrig, vec3& planeNormal) {
    // we can't know the z-coordinate since we move the gizmo by the mouse in 2D
    // space. But our result must lie on the line through this Point with this
    // x,y coordinates on the near plane, so set the z-coordinate to -1.f by
    // this we know that the result will lie at s_camPos.z - nearPlane in 3D
    // Space
    vec4 mouseDown3D = inPoint;
    mouseDown3D.z    = -1.f;
    mouseDown3D.w    = 1.f;

    // unproject (with the inverse projection, view and model matrix of the
    // camera)
    mouseDown3D = inverse(m_cs->getProjectionMatr() * m_cs->getViewMatr()) * mouseDown3D;
    mouseDown3D /= mouseDown3D.w;

    // normalize the result, take it as a direction vector from the camera
    // position that point towards the Mouse Offset in 3D Space
    vec3  dir = normalize(vec3(mouseDown3D) - m_cs->getCamPos());
    float d   = dot(dir, planeNormal);
    m_rayDist = dot(planeOrig - camPos, planeNormal) / d;

    return vec4(camPos + dir * m_rayDist, 1.f);
}

void SPObjectSelector::addGizmo(transMode gMode) {
    if (m_selectedNode) {
        // be sure all gizmos are deselected
        for (auto& g : *m_gizmos)
            if (g) g->setSelected(false);

        // add a Gizmo to the end of the SceneTree with the position of the
        // center of the selected SceneNode if the Gizmo is a Scaling Gizmo and
        // the NO_SCALE flag is set, don't add anything
        if (!m_cs->s_activeGizmo && gMode != transMode::none && gMode != transMode::twWidget) {
            m_cs->s_activeGizmo = m_gizmos->at((int)gMode);

            // if requested, immediately select the first axis of the gizmo,
            // when it is added
            if (m_addGizmoSelectsAxis) m_cs->s_activeGizmo->selectAxis(0);
        }

        SNGizmo* actGiz = nullptr;
        if (m_cs->s_activeGizmo) {
            actGiz = m_cs->s_activeGizmo;
            actGiz->setVisibility(true);

            if (!m_selectedNode->isFixGizmoRot()) {
                actGiz->rotate(m_selectedNode->getRotAngle(), m_selectedNode->getRotAxis());
                // actGiz->rotate(m_selectedNode->getRotAngle() * (gMode !=
                // transMode::translate ? 1.f:0.f),
                // m_selectedNode->getRotAxis());
            }

            actGiz->translate(m_selectedNode->getTransVec());

            // register a callback to update the gizmo when the corresponding
            // sceneNode is moved from code
            m_selectedNode->pushModelMatChangedCb(this, [actGiz, this](SceneNode* node) {
                if (!m_gizmoDragged && s_sd)  // avoid a translation feedback loop, otherwise this
                                              // will be also called when the gizmo is moved via
                                              // the mouse
                {
                    actGiz->translate(node->getTransVec());
                    if (!node->isFixGizmoRot()) actGiz->rotate(node->getRotAngle(), node->getRotAxis());

                    s_sd->reqRenderPasses->at(renderPass::GLSG_OBJECT_MAP_PASS) = true;
                    if (node->m_nodeType == GLSG_SNT_LIGHT)
                        s_sd->reqRenderPasses->at(renderPass::GLSG_SHADOW_MAP_PASS) = true;

                    m_mouseDownObjTransVec = *node->getTransVec();
                    m_mouseDownObjScaleVec = *node->getScalingVec();
                    m_mouseDownObjRot      = angleAxis(node->getRotAngle(), *node->getRotAxis());
                }

                return false;  // keep this callback after execution
            });

            // note: scaling is always relative to screen which is done inside
            // Gizmo::draw() dynamically, so it is not set here

            if (m_lastSceneData) m_lastSceneData->reqRenderPasses->at(GLSG_OBJECT_MAP_PASS) = true;
        }

        if (m_addGizmoCb) m_addGizmoCb(gMode);

        m_actTransMode = gMode;
    }
}

void SPObjectSelector::sendPar(CameraSet* cs, double time, SceneNode* node, SceneNode* parent, renderPass pass,
                               uint loopNr) {
    ShaderProto::sendPar(cs, time, node, parent, pass);

    // set object id
    if (pass == GLSG_OBJECT_MAP_PASS && s_shader) {
        s_shader->setUniform1f("objID", float(node->getObjId(parent)));
    }
}

bool SPObjectSelector::begin(CameraSet* cs, renderPass pass, uint loopNr) {
    if (!m_cs) {
        m_cs     = cs;
        m_aspect = m_cs->getActFboSize()->x / m_cs->getActFboSize()->y;
    }

    if (pass == GLSG_OBJECT_MAP_PASS) {
        m_fbo->bind();
        s_shader->begin();
        m_needUnbindFbo = true;
        return true;

    } else
        return false;
}

bool SPObjectSelector::end(renderPass pass, uint loopNr) {
    if (pass == GLSG_OBJECT_MAP_PASS && m_needUnbindFbo) {
        s_shader->end();
        m_fbo->unbind();
        m_needUnbindFbo = false;
    }

    return false;
}

void SPObjectSelector::clear(renderPass pass) {
    if (pass == GLSG_OBJECT_MAP_PASS) {
        m_fbo->bind();
        glClearColor(-1.0, -1.0, -1.0, 1.0);  // clear Object Id to -1
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_fbo->unbind();
    }
}

void SPObjectSelector::clearDepth() {
    m_fbo->bind();
    m_fbo->clearDepth();
    m_fbo->unbind();
}

///> read one pixel of the actual depth buffer, convert depth_map values to
/// world z -values
float SPObjectSelector::getDepthAtScreen(float _x, float _y, float _near, float _far) {
    GLfloat val;

    glReadBuffer(GL_BACK);
    glReadPixels(static_cast<int>((_x * 0.5f + 0.5f) * float(s_scrWidth)),
                 static_cast<int>((_y * 0.5f + 0.5f) * float(s_scrHeight)), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &val);

    float z_n = 2.f * val - 1.f;
    float z_e = 2.f * _near * _far / (_far + _near - z_n * (_far - _near));

    return z_e;
}

void SPObjectSelector::setScreenSize(uint width, uint height) {
    s_scrWidth = width, s_scrHeight = height;
    m_fbo->resize(width, height);
    if (s_sd) s_sd->reqRenderPasses->at(GLSG_OBJECT_MAP_PASS) = true;
}

void SPObjectSelector::setTransMode(transMode trMode) {
    // if the mode is different from the actual one and we have an active gizmo
    // replace it
    if (m_actTransMode != trMode &&
        !(trMode == transMode::scale && m_selectedObjectNode->m_transFlag & GLSG_NO_SCALE)) {
        m_actTransMode = trMode;

        // hide the actual gizmo
        if (m_cs && m_cs->s_activeGizmo) {
            m_cs->s_activeGizmo->setVisibility(false);
            m_cs->s_activeGizmo = nullptr;
        }

        // set the selection back to the corresponding scene Node
        if (m_selectedNode && m_selectedNode->isSelected()) m_selectedNode->setSelected(false);

        m_selectedNode = m_selectedObjectNode;
        if (m_selectedObjectNode && !m_selectedObjectNode->isSelected()) m_selectedObjectNode->setSelected(true);

        // set the actual select object ID to "not found"
        if (m_cs) m_cs->s_selectObj = -2;

        // get the SceneNodes ObjId
        if (m_selectedObjectNode) {
            int objId = m_selectedObjectNode->getObjId(m_selectedNode->getFirstParentNode());
            selectObj(objId, true);  // selectObj again with the same ID and the
                                     // change transMode
        }
    }
}

void SPObjectSelector::deselect() {
    if (m_selectedNode) {
        // setTransMode(transMode::translate);
        if (m_selectedNode->isSelected()) m_selectedNode->setSelected(false);
    }

    m_selectedNode = nullptr;

    if (m_selectedObjectNode) m_selectedObjectNode->setSelected(false);

    m_selectedObjectNode = nullptr;

    if (m_cs) {
        m_cs->s_selectObj = -2;
        if (m_cs->s_activeGizmo) {
            m_cs->s_activeGizmo->setVisibility(false);
            m_cs->s_activeGizmo = nullptr;
        }
    }
}

SceneNode* SPObjectSelector::getGizmoNode() {
    SceneNode* gizmoNode = nullptr;
    if (m_cs && m_selectedNode) gizmoNode = m_selectedNode->getParentNode(m_cs->s_selectObj);

    return gizmoNode;
}

}  // namespace ara
