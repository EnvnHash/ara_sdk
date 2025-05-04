#include "Gizmo.h"

#include <Asset/AssetImageBase.h>
#include <SceneNodes/SNGizmoRotAxisLetter.h>

#include "UIWindow.h"


#include <CameraSets/CsPerspFbo.h>
#include <Asset/AssetColor.h>
#include <SceneNodes/SNGizmo.h>
#include "DrawManagers/DrawManagerGizmo.h"
#include <GizmoAxisLabel.h>
#include <Image.h>
#include <Utils/Camera.h>
#include <Utils/TrackBallCam.h>

#include "GeoPrimitives/Cube_TexMap.h"

using namespace glm;
using namespace std;

namespace ara {

Gizmo::Gizmo() {
    m_canReceiveDrag = true;
    setName(getTypeName<Gizmo>());
    setFocusAllowed(false);
    setScissorChildren(false);
}

Gizmo::Gizmo(const std::string& styleClass) : Image(styleClass) {
    m_canReceiveDrag = true;
    setName(getTypeName<Gizmo>());
    setFocusAllowed(false);
    setScissorChildren(false);
}

void Gizmo::init() {
    m_drawMan = make_unique<DrawManagerGizmo>(m_glbase);

    // Since usually the gizmo widget will be small in relation to the whole
    // window mouse inputs will be quite coarse, resulting in jittering when
    // rotating apply a scaling to mouse input to minimize this effect
    m_rotScale.x = 0.1f;
    m_rotScale.y = m_rotScale.x * m_viewPort.z / m_viewPort.w;

    // get backgroundcolor
    if (getSharedRes()->res) {
        m_bkColor = getSharedRes()->res->findNode<AssetColor>(getStyleClass() + ".bkcolor");
    }

    // get width height from stylesheets
    auto node           = getSharedRes()->res->findNode(getStyleClass());
    int32_t viewWidth   = 120, viewHeight = 120;
    if (node) {
        viewWidth  = node->value<int32_t>("width", 120);
        viewHeight = node->value<int32_t>("height", 120);
    }

    // get drag handle size
    node = getSharedRes()->res->findNode("defaults");
    if (node) {
        m_axisLabelSize.x = node->value<int32_t>("gizmoDragHandleSize", 20);
        m_axisLabelSize.y = m_axisLabelSize.x;
    }

    m_cam.setType(camType::perspective);
    m_cam.setFov(static_cast<float>(M_PI * 0.3f));
    m_cam.setScreenWidth(viewWidth);
    m_cam.setScreenHeight(viewHeight);
    m_cam.setCamPos(0.f, 0.f, 3.f); // gizmo size is [2,2,2], camPos.z = 2.f would result in the gizmo fitting
                                    // exactly into the widget. put it to 3 to have some border
    m_cam.buildMatrices();

    // set trackball interaction mode
    // m_cam.setTrackBallMode(TrackBallCam::mode::arcBallShoe);
    m_cam.setTrackBallMode(TrackBallCam::mode::orbitNoRoll);
    //    m_cam.setRotExp(1.5f);
    m_cam.setVirtRotCenter(vec3(0.f, 0.f, 0.f));
    m_cam.setTransInteraction(TrackBallCam::TransMapping::none);
    m_cam.setRotateInteraction(TrackBallCam::RotateMapping::leftDrag);

    // register a function to unblock HID events, on camera fade end
    m_cam.getAnimTrans().setEndFunc([this] {
        m_blockHID = false;
        for (const auto& it : m_axisLabels) {
            it->setHIDBlocked(false);
        }
    });

    // if there was a model camera set, sync now
    if (m_modelCam) {
        m_cam.setTrackBallRot(&m_modelCam->getTrackBallRot());
        m_cam.updateFromExternal();
    }

    m_drawMan->setShaderCollector(m_shCol);
    m_stdTex = initTexDepthShdr();
    m_DrawShader = initLineShdr2();

    for (auto& it : m_fbo) {
        it = make_unique<FBO>(FboInitParams{m_glbase,
                                            static_cast<int>(viewWidth * getWindow()->getPixelRatio()),
                                            static_cast<int>(viewHeight * getWindow()->getPixelRatio()),
                                            1,
                                            GL_RGBA8, GL_TEXTURE_2D, true, 2, 1, 1,
                                            GL_REPEAT, false});
        it->setMinFilter(GL_NEAREST, 1);
        it->setMagFilter(GL_NEAREST, 1);
    }

    m_sd.glbase       = m_glbase;
    m_sd.uiWindow     = static_cast<void *>(getWindow());
    m_sd.contentScale = vec2{getWindow()->getPixelRatio(), getWindow()->getPixelRatio()};
    m_sd.debug        = false;
    m_sd.dataPath     = m_sharedRes->dataPath.string() + "/";

    m_sd.winViewport.x = getWinPos().x;
    m_sd.winViewport.y = getWindow()->getSize().y - getWinPos().y - getSize().y;
    m_sd.winViewport.z = getSize().x * m_sd.contentScale.x;
    m_sd.winViewport.w = getSize().y * m_sd.contentScale.y;

    // create a small local scenegraph for rendering the gizmo
    m_rootNode.setScene((WindowBase*)getWindow());
    m_rootNode.setSceneData(&m_sd);
    m_rootNode.m_calcMatrixStack = true;

    m_gizmoSN = dynamic_cast<SNGizmo *>(m_rootNode.addChild(make_unique<SNGizmo>(transMode::rotate_axis, &m_sd)));

    // arrays for the basic axis cross
    m_crossPos.resize(12);
    m_crossColor.resize(12);
    m_crossAux0.resize(12);

    float bVal = 0.28f;
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 2; i++) {
            float cVal = i == 0 ? 0.92f : 0.70f;
            float dir  = i == 0 ? 1.f : -1.f;
            for (int k = 0; k < 2; k++) {
                int ind = (j * 2 + i) * 2 + k;

                m_crossColor[ind] = vec4{j == 1 ? cVal : bVal, j == 2 ? cVal : bVal, j == 0 ? cVal : bVal, 1.f};
                m_crossPos[ind]   = k == 0
                                        ? vec4{0.f, 0.f, 0.f, 1.f}
                                        : vec4{static_cast<float>(j == 1) * dir, static_cast<float>(j == 2) * dir, static_cast<float>(j == 0) * dir, 1.f};

                m_crossAux0[ind]   = vec4{0.f};
                m_crossAux0[ind].x = (1.f + static_cast<float>(j == 1 ? i * 3 : j == 2 ? i * 3 + 1 : i * 3 + 2)) * 0.1f;
            }
        }
    }

    m_crossVao = new VAO("position:4f,color:4f,aux0:4f", GL_DYNAMIC_DRAW);
    m_crossVao->upload(CoordType::Position, &m_crossPos[0][0], 12);
    m_crossVao->upload(CoordType::Color, &m_crossColor[0][0], 12);
    m_crossVao->upload(CoordType::Aux0, &m_crossAux0[0][0], 12);

    m_auxViewport = vec4(0, 0, viewWidth, viewHeight);
    m_auxUIRoot.setViewport(&m_auxViewport);
    m_auxSharedRes          = UISharedRes(*getSharedRes());
    m_auxOrtho              = ortho(0.f, static_cast<float>(viewWidth), static_cast<float>(viewHeight), 0.f);
    m_auxSharedRes.orthoMat = &m_auxOrtho;
    m_auxUIRoot.setSharedRes(&m_auxSharedRes);

    m_axisLabelX = m_auxUIRoot.addChild<GizmoAxisLabel>(getStyleClass() + ".labelX");
    m_axisLabelX->setName("x-pos");
    m_axisLabelX->setHasLabel(true);

    m_axisLabelY = m_auxUIRoot.addChild<GizmoAxisLabel>(getStyleClass() + ".labelY");
    m_axisLabelY->setName("y-pos");
    m_axisLabelY->setHasLabel(true);

    m_axisLabelZ = m_auxUIRoot.addChild<GizmoAxisLabel>(getStyleClass() + ".labelZ");
    m_axisLabelZ->setName("z-pos");
    m_axisLabelZ->setHasLabel(true);

    m_axisLabelXNeg = m_auxUIRoot.addChild<GizmoAxisLabel>(getStyleClass() + ".labelXN");
    m_axisLabelXNeg->setName("x-neg");
    m_axisLabelYNeg = m_auxUIRoot.addChild<GizmoAxisLabel>(getStyleClass() + ".labelYN");
    m_axisLabelYNeg->setName("y-neg");
    m_axisLabelZNeg = m_auxUIRoot.addChild<GizmoAxisLabel>(getStyleClass() + ".labelZN");
    m_axisLabelZNeg->setName("z-neg");

    m_axisLabels.emplace_back(m_axisLabelX);
    m_axisLabels.emplace_back(m_axisLabelY);
    m_axisLabels.emplace_back(m_axisLabelZ);
    m_axisLabels.emplace_back(m_axisLabelXNeg);
    m_axisLabels.emplace_back(m_axisLabelYNeg);
    m_axisLabels.emplace_back(m_axisLabelZNeg);

    uint32_t i = 1;
    for (const auto& it : m_axisLabels) {
        it->setGizmoParent(this);
        it->setSize(m_axisLabelSize.x, m_axisLabelSize.y);
        it->setAxisFlag(static_cast<TrackBallCam::snap>(i - 1));
        it->updtMatrIt(nullptr);
        it->setDrawInmediate(false);
        it->m_presetObjId = ++i;  // preset objid
    }

    // add a callback for including the axisLabels into the id tree iteration
    m_outOfTreeObjId = [this](ObjPosIt& opi) {
        getWinPos();

        // transform mouse pos into gizmo space
        vec2 gsPos = opi.pos - m_winRelPos;

        std::list<GizmoAxisLabel*> inBounds;

        for (const auto& it : m_axisLabels)
            if (all(greaterThanEqual(gsPos, it->getPos())) &&
                all(lessThanEqual(gsPos, it->getPos() + it->getSize())))
                inBounds.emplace_back(it);

        if (inBounds.empty()) return false;

        // sort by zPos
        inBounds.sort([](const GizmoAxisLabel* a, const GizmoAxisLabel* b) { return a->getZPos() < b->getZPos(); });

        opi.foundId   = (*inBounds.begin())->getId();
        opi.foundNode = (*inBounds.begin());

        return true;
    };
}

void Gizmo::setModelCam(TrackBallCam* cam) {
    m_modelCam = cam;

    // initial sync
    m_cam.setTrackBallRot(&cam->getTrackBallRot());
    m_cam.updateFromExternal();

    // on model camera changes, update the gizmo
    cam->addTrackBallUpdtCb(this, [this](TbModData& data) {
        if (data.transType != mouseDragType::zooming) cbUpdt(&m_cam, data, false);
    });

    // on gizmo changes, update the model camera
    m_cam.addTrackBallUpdtCb(
        this, [this](TbModData& data) { cbUpdt(m_modelCam, data, data.transType == mouseDragType::rotating); });
}

void Gizmo::cbUpdt(TrackBallCam* cam, TbModData& data, bool mapByMouseRot) {
    if (mapByMouseRot) {
        if (data.dragStart) cam->setInteractionStart();
        if (data.mouseRot) cam->setTbMouseRot(data.mouseRot);

        if (static_cast<TrackBallCam::mode>(data.mode) == TrackBallCam::mode::fixAxisMapping)
            cam->dragFixMouseAxisMapping();
        else if (static_cast<TrackBallCam::mode>(data.mode) == TrackBallCam::mode::orbitNoRoll)
            cam->dragOrbitNoRoll();

        cam->updateSuperClass(false);

    } else {
        if (data.transType == mouseDragType::snapToAxis) {
            if (data.fadeStart) {
                // block all HID input
                m_blockHID = true;
                for (const auto& it : m_axisLabels) it->setHIDBlocked(true);

                cam->setLookAtBlendSrcPos(&cam->getTrackBallTrans());
                cam->setLookAtBlendSrcLookAt(cam->getLookAtBlendSrcCamPos() +
                                             vec3(inverse(cam->getModelMatr()) * vec4(0.f, 0.f, -1.f, 0.f)));
                cam->setLookAtBlendSrcUpVec(vec3(inverse(cam->getModelMatr()) * vec4(0.f, 1.f, 0.f, 0.f)));

                float d = distance(cam->getLookAtBlendSrcCamPos(), cam->getSnapAxisRotCenter());
                cam->setLookAtBlendDstPos(cam->getSnapAxisRotCenter() + m_cam.getLookAtBlendDstCamPos() * d);
                cam->setLookAtBlendDstLookAt(cam->getLookAtBlendDstCamPos() - m_cam.getLookAtBlendDstCamPos());
                cam->setLookAtBlendDstUpVec(&m_cam.getLookAtBlendDstUpVec());
            }

            cam->lookAtBlend(m_cam.getAnimTrans().getPercentage(), &cam->getTrackBallRot(), &cam->getTrackBallTrans());
        } else {
            cam->setTrackBallRot(data.rotEuler);
        }

        cam->updateFromExternal();
    }

    m_camChanged = true;
}

Shaders* Gizmo::initLineShdr() const {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n
        uniform mat4 m_pvm; \n
        void main() {\n
            gl_Position = m_pvm * position;\n
    });
    vert             = "// gizmo line shader, vert\n" + ShaderCollector::getShaderHeader() + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 glFragColor; \n
         uniform vec4 color; \n
         void main() {\n
             glFragColor = color;\n);

        frag = "// gizmo line shader, frag\n" + ShaderCollector::getShaderHeader() + m_shCol->getUiObjMapUniforms() + frag +
               m_shCol->getUiObjMapMain() + "}";

        return m_shCol->add("Gizmo_lineahder", vert, frag);
}

Shaders* Gizmo::initLineShdr2() const {
        std::string vert = STRINGIFY(layout(location = 0) in vec4 position; \n
        layout(location = 3) in vec4 color; \n
        layout(location = 6) in vec4 aux0; \n   // objId
        uniform mat4 m_pvm;
        out VS_FS {\n
            vec4 color;\n
            float axisId; \n
        } vout;\n
        void main() {\n
            vout.color = color;\n
            vout.axisId = aux0.x;\n
            gl_Position = m_pvm * position; \n
        });
        vert = "// gizmo line shader2, vert\n" + ShaderCollector::getShaderHeader() + vert;

        std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor; \n
        layout(location = 1) out vec4 objMap; \n
        in VS_FS {\n
             vec4 color;\n
             float axisId; \n
        } vin;\n
        void main() { \n
            objMap = vec4(vin.axisId, gl_FragCoord.z, 0.0, float(vin.axisId > 0.0));
            fragColor = vin.color; \n
        });

        frag = "// gizmo line shader2, frag\n" + ShaderCollector::getShaderHeader() + frag;

        return m_shCol->add("Gizmo_initLineShdr2", vert, frag);
}

Shaders* Gizmo::initTexDepthShdr() const {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 2) in vec2 texCoord; \n
        uniform mat4 m_pvm; \n
        out vec2 tex_coord; \n
        void main() {\n
            tex_coord   = texCoord;\n
            gl_Position = m_pvm * position;\n
        });

    vert = "// Gizmo tex_depth_obj shader, vert\n" + ShaderCollector::getShaderHeader() + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex; \n
        uniform sampler2D objId; \n
        uniform sampler2D depth; \n
        in vec2 tex_coord; \n
        layout(location = 0) out vec4 color; \n
        layout(location = 1) out vec4 fragObjId; \n
        void main() {\n
            vec4 col  = texture(tex, tex_coord);
            fragObjId    = texture(objId, tex_coord);
            gl_FragDepth = col.a > 0.01 ? texture(depth, tex_coord).r : 1.0;
            color        = col;\n
        });

    frag = "// Gizmo tex_depth_obj shader, frag\n" + ShaderCollector::getShaderHeader() + frag;

    return m_shCol->add("tex_depth_obj", vert, frag);
}

Shaders* Gizmo::initObjMapTexShdr() const {
    string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 2) in vec2 texCoord; \n
        uniform mat4 m_pvm;\n
        out vec2 tex_coord;\n
        void main() {
            tex_coord   = texCoord;
            gl_Position = m_pvm * position;
        });
    vert = "// gizmo objid tex shader, vert\n" + ShaderCollector::getShaderHeader() + vert;

    string frag = STRINGIFY(
        uniform sampler2D tex;\n
        uniform uint baseObjId;\n
        in vec2 tex_coord;\n
        layout(location = 1) out vec4 frag; \n
        void main() { \n
            vec4    objIdV = texture(tex, tex_coord);\n
            uint objIdUInt = (uint(objIdV.r * 255.0) << 16) + (uint(objIdV.g * 255.0) << 8) + uint(objIdV.b * 255.0);\n
            objIdUInt += baseObjId;\n
            float objId = float(objIdUInt);\n
            frag  = vec4(mod(floor(objId * 1.52587890625e-5), 256.0) / 255.0,
                        mod(floor(objId * 0.00390625), 256.0) / 255.0, mod(objId, 256.0) / 255.0, 1.0);\n;
            });
    frag = "// gizmo objid tex shader, frag\n" + ShaderCollector::getShaderHeader() + frag;

    return m_shCol->add("Gizmo_ObjectMap_Shader", vert, frag);
}

bool Gizmo::draw(uint32_t* objId) {
    drawToFbo(objId);
    return Image::draw(objId);
}

bool Gizmo::drawIndirect(uint32_t *objId) {
    if (m_sharedRes && m_sharedRes->drawMan) {
        m_dfTempObjId = *objId;
        m_sharedRes->drawMan->pushFunc([this] {
            m_dfObjId = m_dfTempObjId;
            drawToFbo(&m_dfObjId);
        });
    }

    Image::drawIndirect(objId);
    return true;
}

bool Gizmo::drawToFbo(const uint32_t* objId) {
    if (!m_DrawShader || !m_fbo[0] || !m_fbo[1]) {
        return false;
    }

    // update camera fade if necessary
    for (const auto& it : m_axisLabels) {
        it->updateCamFade();
    }

    // first pass cpu only, update matrices
    if (m_camChanged && m_gizmoSN->getChildren() && !m_gizmoSN->getChildren()->empty()) {
        for (const auto& it : *m_gizmoSN->getChildren()) {
            auto gixAx = dynamic_cast<SNGizmoRotAxisLetter*>(it);
            if (gixAx && gixAx->m_gizVao) {
                m_pvm = m_cam.getMVP() * gixAx->getRotMat();

                for (int i = 0; i < 2; i++) {
                    // update labels
                    axIndx   = (gixAx->m_nameFlag & GLSG_ROT_AXIS_GIZMO_X)
                                   ? 0
                                   : ((gixAx->m_nameFlag & GLSG_ROT_AXIS_GIZMO_Y) ? 1 : 2);
                    m_axlPos = getAxisEnd(m_pvm, i == 0 ? 1.f : -1.f);
                    axIndx   = i == 0 ? axIndx : axIndx + 3;

                    if (!all(equal(m_axisLabels[axIndx]->getPos(), vec2(m_axlPos)))) {
                        m_axisLabels[axIndx]->setPos(static_cast<int>(m_axlPos.x), static_cast<int>(m_axlPos.y), state::none);
                        m_axisLabels[axIndx]->setPos(static_cast<int>(m_axlPos.x), static_cast<int>(m_axlPos.y), state::highlighted);
                        m_axisLabels[axIndx]->setPos(static_cast<int>(m_axlPos.x), static_cast<int>(m_axlPos.y), state::selected);
                    }

                    if (m_axisLabels[axIndx]->getZPos() != m_axlPos.z) {
                        m_axisLabels[axIndx]->setZPos(m_axlPos.z);
                    }

                    m_tempObjId = m_axisLabels[axIndx]->m_presetObjId + m_objIdMin;
                    m_axisLabels[axIndx]->setId(m_tempObjId);
                }
            }
        }

        m_auxUIRoot.updtMatrIt(nullptr);

        // copy axisLabel list and sort
        m_newZSortedAxisLabels.clear();
        ranges::copy(m_axisLabels, std::back_inserter(m_newZSortedAxisLabels));
        m_newZSortedAxisLabels.sort(
            [](const GizmoAxisLabel* a, const GizmoAxisLabel* b) { return a->getZPos() > b->getZPos(); });

        if (m_newZSortedAxisLabels != m_zSortedAxisLabels || m_zSortedAxisLabels.empty()) {
            m_zSortedAxisLabels = m_newZSortedAxisLabels;

            m_drawMan->clear();
            m_drawMan->addSet();

            for (const auto& it : m_zSortedAxisLabels) {
                if (it->getImgBase()) {
                    it->m_texUnit = m_drawMan->pushTexture(it->getImgBase()->getTexID());
                }

                it->updateDrawData();
                it->m_indDrawBlock.drawSet = &m_drawMan->push(it->m_indDrawBlock, it);

                if (it->hasLabel()) {
                    it->m_imgDB.drawSet = &m_drawMan->push(it->m_imgDB, it);
                }
            }

            m_drawMan->rebuildVaos();
        } else {
            m_updtDrawData = true;
        }

        m_camChanged = false;
    }

    if (m_updtDrawData) {
        // update only
        for (const auto& it : m_zSortedAxisLabels) {
            it->updateDrawData();
            it->pushVaoUpdtOffsets();
        }
        m_drawMan->update();
        m_updtDrawData = false;
    }

    // -------------------------------------------------------------------------------------
    // seconds pass draw!

    m_objIdMin = m_objIdMax = *objId;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glLineWidth(2.f * getWindow()->getPixelRatio());

    if (getWindow() && getWindow()->getSceneFbo()) {
        m_sceneFbo = getWindow()->getSceneFbo();
        m_sceneFbo->unbind(false);
    }

    m_fbo[0]->bind(false);

    // clear buffer 0
    if (m_bkColor) {
        m_fbo[0]->clearToColor(m_bkColor->rgba[0], m_bkColor->rgba[1], m_bkColor->rgba[1], 0.f, 0);
    } else {
        m_fbo[0]->clearToColor(0.235f, 0.235f, 0.235f, 0.f, 0);
    }

    m_fbo[0]->clearToColor(0.f, 1.f, 0.f, 0.f, 1);  // clear buffer 1, y = depth value must be cleared to 1
    m_fbo[0]->clearDepth();     // clear depth

#ifndef ARA_USE_GLES31
    glBlendFunci(1, GL_ONE, GL_ZERO);
#else
    glBlendFunc(GL_ONE, GL_ZERO);
#endif

    // draw lines write depth values and axis id
    m_DrawShader->begin();
    m_DrawShader->setUniformMatrix4fv("m_pvm", &m_cam.getMVP()[0][0]);
    m_crossVao->draw(GL_LINES, 0, 12, nullptr, GL_LINES);

    // ------- copy the result to s_fbo[1] ---------------------------

    // bind s_fbo[1]
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[1]->getFbo());

    std::array<GLenum, 1> bufs{GL_COLOR_ATTACHMENT1};  // GLES compatibility
    glDrawBuffers(1, &bufs[0]);

    // attach the source texture to color attachment0
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo[0]->getColorImg(), 0);

    // attach the destination texture layer to color attachment1
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_fbo[1]->getColorImg(), 0);

    // blit
    glBlitFramebuffer(0, 0, m_fbo[0]->getWidth(), m_fbo[0]->getHeight(), 0, 0, m_fbo[0]->getWidth(),
                      m_fbo[0]->getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // attach back s_fbo[1] to  color attachment0
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo[1]->getColorImg(), 0);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_fbo[1]->getColorImg(1), 0);

    glDrawBuffers(2, &m_fbo[1]->getBufModes()->at(0));

    // ------- copy the result to s_fbo[1] ---------------------------

    glDisable(GL_DEPTH_TEST);

#ifndef ARA_USE_GLES31
    glBlendFunci(1, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    m_drawMan->setOptTex(m_fbo[0]->getColorImg(1));
    m_drawMan->draw();

    m_fbo[1]->unbind(false);

    if (m_sceneFbo) m_sceneFbo->bind(false);

    glLineWidth(1.f);

    Image::setTexId(m_fbo[1]->getColorImg(), static_cast<int>(m_fbo[1]->getWidth()), static_cast<int>(m_fbo[1]->getHeight()), 32);
    setImgFlags(32);

    m_objIdMax = m_tempObjId;

    return true;  // count up objId
}

vec3 Gizmo::getAxisEnd(const mat4& pvm, float yVal) {
    vec4 axlPos = pvm * vec4(0.f, yVal, 0.f, 1.f);
    axlPos /= axlPos.w;
    axlPos.x = axlPos.x * 0.5f + 0.5f;
    axlPos.y = 1.f - (axlPos.y * 0.5f + 0.5f);
    axlPos.z = axlPos.z * 0.5f + 0.5f;  // for directly writing depth buffer
                                        // values in fragment shader

    return {axlPos.x * getSize().x - static_cast<float>(m_axisLabelSize.x) * 0.5f,
                     axlPos.y * getSize().y - static_cast<float>(m_axisLabelSize.y) * 0.5f, axlPos.z};
}

void Gizmo::mouseDrag(hidData* data) {
    if (data->mouseRightPressed) {
        return;
    }

    // check if the clicked object id is within this UINode (the same node
    // or any of its children)
    for (const auto& it : m_axisLabels) {
        if (data->clickedObjId == it->getId()) {
            it->mouseDrag(data);
        }
    }

    drag(data);
    data->consumed = true;
}

void Gizmo::drag(const hidData* data)
{
    if (data->dragStart) {
        if (getWindow()) getWindow()->setMouseCursorVisible(false);
        m_dragStartPos = data->mousePos;
        excludeLabelsFromStyles(true);  // avoid Label highlighting while dragging
        m_resetExcludeFromStyles = true;
    }

    m_cam.mouseDrag(static_cast<float>(data->mousePosNodeRel.x) / getSize().x,
                    static_cast<float>(data->mousePosNodeRel.y) / getSize().y,
                    data->shiftPressed, data->altPressed, data->ctrlPressed, m_rotScale);

    getSharedRes()->requestRedraw = true;
}

void Gizmo::mouseUp(hidData* data) {
    if (!m_leftPressed) {
        return;
    }

    for (const auto& it : m_axisLabels) {
        it->mouseUp(data);
        if (data->consumed && getSharedRes()) {
            getSharedRes()->requestRedraw = true;
            return;
        }
    }

    m_cam.mouseUpLeft(static_cast<float>(data->mousePosNodeRel.x) / getSize().x,
                static_cast<float>(data->mousePosNodeRel.y) / getSize().y);

    if (getWindow()) {
        getWindow()->setMouseCursorVisible(true);
    }

    // reposition the cursor to be exactly over the label
    if (data->dragging) {
#ifdef ARA_USE_GLFW
        runOnMainThread([this] {
            glfwSetCursorPos(getWindow()->getWinHandle()->getCtx(), m_dragStartPos.x * getWindow()->getPixelRatio(),
                             m_dragStartPos.y * getWindow()->getPixelRatio());
            return true;
        });
#elif _WIN32
        vec2 winOffs;
        if (getWindow()->extGetWinOffs()) winOffs = getWindow()->extGetWinOffs()();

        // in contrast to GLFW this must be relative to the screen not the
        // window
        SetCursorPos((int)(winOffs.x + m_dragStartPos.x * getWindow()->getPixelRatio()),
                     (int)(winOffs.y + m_dragStartPos.y * getWindow()->getPixelRatio()));
#endif
    }

    if (m_resetExcludeFromStyles) {
        excludeLabelsFromStyles(false);  // re-enable Label highlighting while dragging
        m_resetExcludeFromStyles = false;
    }

    data->consumed = true;
    m_leftPressed  = false;
}

void Gizmo::mouseDownRight(hidData* data) {
    // check if the clicked object id is within this UINode (the same node or any of its children)
    for (const auto& it : m_axisLabels) {
        it->mouseDownRight(data);
        if (data->consumed) {
            if (m_auxSharedRes.requestRedraw) {
                getSharedRes()->requestRedraw = m_auxSharedRes.requestRedraw;
            }
            return;
        }
    }

    if (getWindow()) {
        getWindow()->setMouseCursorVisible(true);
    }

    m_cam.mouseDownRight(static_cast<float>(data->mousePosNodeRel.x) / getSize().x,
                         static_cast<float>(data->mousePosNodeRel.y) / getSize().y);
    data->consumed = true;
    m_rightPressed = true;
}

void Gizmo::mouseUpRight(hidData* data)
{
    if (!m_rightPressed) {
        return;
    }

    // check if the clicked object id is within this UINode (the same node
    // or any of its children)
    for (const auto& it : m_axisLabels) {
        it->mouseUpRight(data);
        if (data->consumed) {
            if (m_auxSharedRes.requestRedraw) {
                getSharedRes()->requestRedraw = m_auxSharedRes.requestRedraw;
            }
            return;
        }
    }

    if (getWindow()) {
        getWindow()->setMouseCursorVisible(true);
    }

    m_cam.mouseUpRight(static_cast<float>(data->mousePosNodeRel.x) / getSize().x,
                    static_cast<float>(data->mousePosNodeRel.y) / getSize().y);
    data->consumed = true;
    m_rightPressed = false;
}

void Gizmo::mouseMove(hidData* data) {
    for (const auto& it : m_axisLabels) {
        it->mouseMove(data);
        if (data->consumed) {
            if (getSharedRes()) getSharedRes()->requestRedraw = true;
            return;
        }
    }

    m_lastMouseHoverData = data;
}

void Gizmo::mouseDown(hidData* data) {
    m_cam.mouseDownLeft(static_cast<float>(data->mousePosNodeRel.x) / getSize().x,
                        static_cast<float>(data->mousePosNodeRel.y) / getSize().y);
    data->consumed = true;
    m_leftPressed  = true;
}

void Gizmo::excludeLabelsFromStyles(bool val) {
    for (const auto& lbl : m_axisLabels) {
        lbl->excludeFromStyles(val);
    }

    getSharedRes()->requestRedraw = true;
}

}
