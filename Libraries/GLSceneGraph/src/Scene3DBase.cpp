#include <Lights/LIProjector.h>
#include <SceneNodes/SNGridFloor.h>
#include <UIWindow.h>
#include <WindowManagement/WindowBase.h>
#include <glb_common/glb_common.h>

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
#include <Windows/DisplayScaling.h>
#endif

#include "Scene3DBase.h"
#include "Utils/BoundingBoxer.h"

using namespace glm;
using namespace std;

namespace ara {

Scene3DBase::Scene3DBase() : Div() {
    Div::setName(getTypeName<Scene3DBase>());
    s_sd.netCameras = (void*)&m_netCameras;
    setForceInit(true);
    setCanReceiveDrag(true);
}

void Scene3DBase::init() {
    Div::init();
    initScene();
    for (auto& it : m_initCb) {
        it();
    }
}

bool Scene3DBase::initScene() {
    if (!getWindow()) {
        return false;
    }

    m_shCol = &m_glbase->shaderCollector();

    initSceneData();
    initBoundingBoxer();
    initSceneTree();
    initGizmos();
    initSsao();
    initCamSet();
    initShaderProtos();  // must be called after cam init
    loadTypo();
    loadBasicSceneModels();

    m_normQuad = make_unique<Quad>(-1.f, -1.f, 2.f, 2.f, vec3(0.f, 0.f, 1.f), 1.f, 0.f, 0.f, 1.f, nullptr, 1, false);
    m_inited   = true;
    m_colShdr  = m_shCol->getStdCol();

    return true;
}

void Scene3DBase::initSceneData() {
#ifdef ARA_USE_ASSIMP
    for (auto& it : m_aImport) {
        it = make_unique<AssimpImport>(m_glbase);
        it->setSceneData(&s_sd);
        it->setUseGL32(m_glbase->getUseFallback());
        it->setScene((WindowBase*)getWindow());
    }
    s_sd.aImport = (void*)&m_aImport;
#endif

    s_sd.glbase          = m_glbase;
    s_sd.reqRenderPasses = &m_reqRenderPasses;
    s_sd.roomDim         = &m_roomDim;
    s_sd.uiWindow        = (void*)getWindow();
    s_sd.contentScale    = vec2{getWindow()->getPixelRatio(), getWindow()->getPixelRatio()};
    s_sd.scene3D         = (void*)this;
    s_sd.debug           = false;
    s_sd.dataPath        = m_sharedRes->dataPath.string() + "/";
    s_sd.deselectAll     = [this] { deselectAll(); };
    s_sd.winViewport.x   = getWinPos().x;
    s_sd.winViewport.y   = getWindow()->getSize().y - getWinPos().y - getSize().y;
    s_sd.winViewport.z   = getSize().x * s_sd.contentScale.x;
    s_sd.winViewport.w   = getSize().y * s_sd.contentScale.y;

    // init the renderPass maps
    for (auto i = 0; i < GLSG_NUM_RENDER_PASSES; i++) {
        m_renderPasses[(renderPass)i]    = false;
        m_reqRenderPasses[(renderPass)i] = true;
    }
}

void Scene3DBase::initBoundingBoxer() {
    m_boundBoxer    = make_unique<BoundingBoxer>(m_glbase);
    s_sd.boundBoxer = (void*)m_boundBoxer.get();
}

void Scene3DBase::initSceneTree() {
    // init NodeTree structure
    m_rootNode = make_unique<SceneNode>(&s_sd);
    m_rootNode->setName("root");
    m_rootNode->setScene((WindowBase*)this);
    m_rootNode->setGLBase(m_glbase);
    m_rootNode->m_calcMatrixStack = true;

    // add a child that will contain all regular Scene Elements
    m_sceneTreeCont = m_rootNode->addChild(false);
    m_sceneTreeCont->setName("sceneTreeCont");
    m_sceneTreeCont->setVisibility(true);

    if (m_enableGrid) {
        // in order to have the possibility to blend onto the grid, but also to have
        // it half transparent, draw it two times one time above and a second time
        // below, the first time without writing to the depth buffer
        m_gridFloorPre = m_sceneTreeCont->addChild<SNGridFloor>();
        m_gridFloorPre->setVisibility(true);
        m_gridFloorPre->setDepthMask(false);

        // x and z axis on the grid floor
        m_gridFloorAxesPre = m_sceneTreeCont->addChild<SNGridFloorAxes>();
        m_gridFloorAxesPre->setVisibility(true);
        m_gridFloorAxesPre->setDepthMask(false);
    }

    if (m_enableSkybox) enableSkyBox(m_enableSkybox);

    m_sceneTree = m_sceneTreeCont->addChild<SceneNode>();
    m_sceneTree->setName("sceneTree");
    m_sceneTree->setVisibility(true);

    // Grid Floor
    if (m_enableGrid) {
        m_gridFloor = m_sceneTreeCont->addChild<SNGridFloor>();
        m_gridFloor->setVisibility(true);

        // x and z axis on the grid floor
        m_gridFloorAxes = m_sceneTreeCont->addChild<SNGridFloorAxes>();
        m_gridFloorAxes->setVisibility(true);
    }
}

void Scene3DBase::initGizmos() {
    m_gizmoFbo =
        make_unique<FBO>(m_glbase, (int)(getSize().x * s_sd.contentScale.x), (int)(getSize().y * s_sd.contentScale.y),
                         GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

    // add a child that will contain the gizmos
    gizmoTree = m_rootNode->addChild(false);
    gizmoTree->setName("gizmoTree");

    // construct all 3 possible gizmos for translation, rotation and scaling
    for (uint i = 0; i < 3; i++)
        m_gizmos.push_back((SNGizmo*)gizmoTree->addChild(make_unique<SNGizmo>((transMode)i, &s_sd), false));
}

void Scene3DBase::initSsao() {
    if (m_useSsao)
        m_ssao = make_unique<SSAO>(m_glbase, (int)getSize().x, (int)getSize().y, SSAO::ALGORITHM_HBAO_CACHEAWARE, true,
                                   2.f, 20.f);
}

void Scene3DBase::loadBasicSceneModels() {
    if (!m_loadBasicSceneModels) return;
#ifdef ARA_USE_ASSIMP
    netCamSN    = make_unique<SceneNode>();
    projectorSN = make_unique<SceneNode>();
    spotLightSN = make_unique<SceneNode>();

    array<bool, 3> loadCnds{{true, true, false}};

    // load standard model for Network Cameras
    std::string modelPath = s_sd.dataPath + "models/camera.3ds";
    if (std::filesystem::exists(modelPath)) {
        netCamSN->setName("StdNetCamContainer");
        m_aImport[2]->load(
            modelPath, netCamSN.get(),
            [this, &loadCnds](SceneNode* modelCont) {
                modelCont->m_hasNewModelMat = true;  // force rebuild of nodes m_absModelMat
                modelCont->iterateNode(netCamSN.get(), [](SceneNode* node) {
                    node->m_nodeType   = GLSG_SNT_CAMERA_SCENE_MESH;
                    node->m_selectable = false;
                    return true;
                });
                modelCont->setVisibility(false);

                loadCnds[2] = true;
            },
            true);
    } else {
        loadCnds[2] = true;
    }

    // be sure that the models are loaded - not really necessary but looks
    // better than seing the objects appear one after another
    bool allReady = false;
    if (!(loadCnds[0] && loadCnds[1] && loadCnds[2])) {
        while (!allReady) {
            getWindow()->iterateGlCallbacks();
            allReady = loadCnds[0] && loadCnds[1] && loadCnds[2];
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
#endif
}

void Scene3DBase::loadTypo() {
    if (filesystem::exists(s_sd.dataPath + "Fonts/open-sans/OpenSans-Regular.ttf")) {
        m_typo     = make_unique<TypoGlyphMap>((int)getSize().x, (int)getSize().y);
        m_fontSize = 18;
        m_typo->loadFont((s_sd.dataPath + "Fonts/open-sans/OpenSans-Regular.ttf").c_str(),
                         &m_glbase->shaderCollector());
        m_typo->setScreenSize((unsigned int)getWindow()->getSize().x, (unsigned int)getWindow()->getSize().y);
    }
}

void Scene3DBase::initShaderProtos() {
    // add the standard ObjectSelector ShaderProtoType to all camera sets
    for (auto& cIt : m_camSet) {
        m_objSel = dynamic_cast<SPObjectSelector*>(cIt->addShaderProto(getTypeName<SPObjectSelector>(),
                                                          {GLSG_OBJECT_MAP_PASS}));  // create ObjectSelector
        m_objSel->setGizmoNodes(&m_gizmos);
        if (m_addGizmoCb) m_objSel->setAddGizmoCb(std::move(m_addGizmoCb));

        cIt->addShaderProto(getTypeName<SPSpotLightShadowVsm>(), {GLSG_SHADOW_MAP_PASS, GLSG_SCENE_PASS, GLSG_GIZMO_PASS});
        // cIt->addShaderProto(getTypeName<SPGridFloor>(), {GLSG_SCENE_PASS});
        // cIt->addShaderProto(getTypeName<SPGridFloorAxes>(), {GLSG_SCENE_PASS});
        // cIt->addShaderProto(getTypeName<SPWorldAxes>(), {GLSG_SCENE_PASS});
        // cIt->addShaderProto(getTypeName<SPSkyBox>(), {GLSG_SCENE_PASS});
    }
}

void Scene3DBase::enableSkyBox(bool val) {
    m_enableSkybox = val;
    if (val && m_sceneTreeCont && !m_skyBox) {
        m_skyBox = m_sceneTreeCont->insertChild<SNSkyBox>(0);
        m_skyBox->setName("skyBox");
        m_skyBox->setVisibility(true);
    }
}

void Scene3DBase::freeGLResources() {
    m_rootNode->setScene(nullptr);
    m_rootNode->iterateNode(m_rootNode.get(), [&](SceneNode* thisNode) {
        thisNode->setScene(nullptr);
        return true;
    });
    m_rootNode->clearChildren();

    if (m_ssao) m_ssao.reset();
    if (m_typo) m_typo.reset();
    if (m_boundBoxer) m_boundBoxer.reset();
    if (m_gizmoFbo) m_gizmoFbo.reset();
}

bool Scene3DBase::draw(uint32_t* objId) {
    Div::draw(objId);
    return drawFunc(objId);
}

bool Scene3DBase::drawIndirect(uint32_t* objId) {
    Div::drawIndirect(objId);

    if (m_sharedRes && m_sharedRes->drawMan) {
        m_tempObjId = *objId;
        m_sharedRes->drawMan->pushFunc([this] {
            m_dfObjId = m_tempObjId;
            drawFunc(&m_dfObjId);
        });
    }

    return true;
}

bool Scene3DBase::drawFunc(uint32_t* objId) {
    if (!m_inited) return false;

    if (!m_startTimeSet) {
        m_startTime    = chrono::system_clock::now();
        m_startTimeSet = true;
    }

    m_intTime  = chrono::duration<double>(chrono::system_clock::now() - m_startTime).count();  // in seconds
    dt         = m_intTime - m_lastTime;
    m_lastTime = m_intTime;

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);

    // the result of the rendered scene may be used to be rendered into other
    // contexts in case something was rendered with alpha < 1.0 onto a colored
    // background, with glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) the
    // resulting fragment may have an alpha value of < 1.0 and look different
    // when rendered onto a background of different color. For this reason
    // always sum up alpha values

    // copy the requested m_renderPasses
    for (auto& pIt : m_reqRenderPasses) {
        m_renderPasses[pIt.first] = static_cast<bool>(pIt.second.load());
    }

    // reset the m_reqRenderPasses
    for (auto& it : m_reqRenderPasses) {
        it.second = false;
    }

    // always set the scene render pass
    m_renderPasses[GLSG_SCENE_PASS]      = true;
    m_renderPasses[GLSG_SHADOW_MAP_PASS] = true;

    // check if the root has a recalcMatrix flag set, if this is the case apply
    // it to the gizmo and sceneNode (this is faster than always recurse the
    // whole sceneTree and look for changes)
    if (m_rootNode->m_calcMatrixStack.load()) {
        if (m_sceneTreeCont) {
            m_sceneTreeCont->m_calcMatrixStack = true;
        }

        if (gizmoTree) {
            gizmoTree->m_calcMatrixStack = true;
        }

        m_rootNode->m_calcMatrixStack = false;
    }

    // main rendering loop
    for (auto& cIt : m_camSet) {
        for (auto& pIt : m_renderPasses) {
            if (pIt.second) {
                switch (pIt.first) {
                    case GLSG_OBJECT_ID_PASS:  // generate an id for each node (in linear order)
                        m_rootNode->regenNodeIds(1);
                        break;

                    case GLSG_OBJECT_MAP_PASS:  // generate a "depth map" with ids
                        cIt->clearScreen(pIt.first);
                        cIt->renderTree(m_sceneTreeCont, m_intTime, dt, 0, pIt.first);
                        // clear the obj Ids s_fbo's depthbuffer to have the
                        // gizmo always rendered above everything same as in the
                        // visible representation
                        if (m_objSel) m_objSel->clearDepth();
                        cIt->renderTree(gizmoTree, m_intTime, dt, 0, pIt.first);
                        break;

                    case GLSG_SHADOW_MAP_PASS:  // generate shadow maps
                        cIt->clearScreen(pIt.first);
                        cIt->renderTree(m_sceneTreeCont, m_intTime, dt, 0, pIt.first);
                        cIt->postRender(GLSG_SHADOW_MAP_PASS, nullptr);
                        break;

                    case GLSG_SCENE_PASS:  // generate the visible result
                        cIt->clearScreen(pIt.first);

                        if (m_useSsao) {
                            m_ssao->bind();
                            m_ssao->clear();
                        }

                        cIt->renderTree(m_sceneTreeCont, m_intTime, dt, 0, pIt.first);
                        cIt->postRender(GLSG_SCENE_PASS, m_extDrawMatrCb ? m_extDrawMatrCb() : getNormMatPtr());

                        if (m_useSsao) {
                            m_ssao->proc(cIt);
                            m_ssao->drawAlpha(cIt, 1.f);
                            m_ssao->blitDepthBuffer(cIt);
                        }
                        break;
                    default: break;
                }
            }
        }
    }

    // set all m_renderPasses to false
    for (auto& it : m_renderPasses) it.second = false;

    //----------------------------------------------------------------------------------------------

    // render the visible part of the gizmos, this is done in a separate pass,
    // to avoid the casting of shadows of the m_gizmos by the m_ssao
    if (m_gizmoFbo) {
        glDepthMask(true);  // be sure that a valid depth mask is generated - this
                            // switch might be set to false in the above operations
        m_gizmoFbo->bind();
        m_gizmoFbo->clear();
        glClearDepthf(1.f);
        glClear(GL_DEPTH_BUFFER_BIT);

        for (auto& cIt : m_camSet) {
            cIt->renderTree(gizmoTree, m_intTime, dt, 0, GLSG_GIZMO_PASS);  // 0 = pre-render step
        }

        m_gizmoFbo->unbind();
    }

    // draw gizmo fbo without writing to the depth buffer
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw gizmo fbo without writing to the depth buffer
    if (m_drawGizmoFbo) {
        m_shCol->getStdTex()->begin();
        m_shCol->getStdTex()->setUniformMatrix4fv("m_pvm", getNormMatPtr());
        m_shCol->getStdTex()->setUniform1i("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_gizmoFbo->getColorImg());
        s_sd.getStdQuad()->draw();
        Shaders::end();
    }

    if (m_drawFps && m_typo) {
        vec4   typocol{1.f, 0.f, 0.f, 1.f};
        string fpsStr  = std::to_string(1000.0 / watch.getDt()) + " fps";
        m_typo->print(-0.95f, 0.9f, fpsStr, m_fontSize, &typocol[0]);
    }

    if (m_permRedraw && getSharedRes()) getSharedRes()->requestRedraw = true;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_drawFps) {
        watch.setEnd();
        // watch.print("Scene3DBase", true);
        // LOG << "dt: " << 1000.0 / watch.getDt();
        watch.setStart();
    }

    return true;  // count up objId
}

void Scene3DBase::setPermRedraw(bool val) {
    m_permRedraw = val;
    if (getSharedRes()) getSharedRes()->requestRedraw = true;
}

void Scene3DBase::updateMatrix() {
    if (!m_geoChanged || m_updating) return;

    Div::updateMatrix();

    if (m_sceneRenderCam && m_sceneRenderCam->getFbo() &&
        ((int)m_sceneRenderCam->getFbo()->getWidth() != (int)(m_size.x * getWindow()->getPixelRatio()) ||
         (int)m_sceneRenderCam->getFbo()->getHeight() != (int)(m_size.y * getWindow()->getPixelRatio()))) {
        updateScene3DBaseViewport(0.f, 0.f, m_size.x * getWindow()->getPixelRatio(),
                                  m_size.y * getWindow()->getPixelRatio());
        getSharedRes()->requestRedraw = true;
    }
}

void Scene3DBase::keyDown(hidData* data) {
    // if there is an ObjectSelector, get it
    SPObjectSelector* objSel = nullptr;
    for (auto& cIt : m_camSet)
        if (cIt->s_shaderProto.find(getTypeName<SPObjectSelector>()) != cIt->s_shaderProto.end()) {
            objSel = dynamic_cast<SPObjectSelector*>(cIt->s_shaderProto[getTypeName<SPObjectSelector>()].get());
            objSel->keyDown(data);
        }

    if (m_sceneRenderCam->getInteractCam())
        m_sceneRenderCam->getInteractCam()->keyDown(data->key, data->shiftPressed, data->altPressed, data->ctrlPressed);

    if (data->key == GLSG_KEY_W && objSel) {
        objSel->setTransMode(transMode::translate);
        return;
    }

    ///> scale mode
    if (data->key == GLSG_KEY_E && objSel) {
        objSel->setTransMode(transMode::scale);
        return;
    }

    ///> rotate mode
    if (data->key == GLSG_KEY_R && objSel) {
        objSel->setTransMode(transMode::rotate);
        return;
    }

    // dump the sceneTree
    if (data->key == GLSG_KEY_D) {
        m_rootNode->dump();
        // if(m_sceneTree) m_sceneTree->dump();
        return;
    }

    if (data->key == GLSG_KEY_F) {
        m_drawFps = !m_drawFps;
        return;
    }

    moveObjectByArrowKeys(data);

    /*
        ///> zoom to object
        if (data->key  == GLSG_KEY_Z && objSel)
        {
            trackballPreset p;
            if (m_sceneCam && objSel->zoomSelectedObject(&p))
            {
                m_sceneCam->fadeTo(p.rotEuler, p.pos, 0.5);

                // add a callback which will be executed on every draw until the
       sceneCam fade stopped addGlCb("animCam", [this](){ if (m_sceneCam)
       m_sceneCam->updateFade(); getSharedRes()->requestRedraw = true; return
       m_sceneCam->fadeStopped();
                });
            }

            return;
        }

       // p, add a projector
       if (data->key == GLSG_KEY_P) {
            addLightObj(objSel, getTypeName<LIProjector>());
            return;
        }

        if (data->key == GLSG_KEY_B) {
            setProjectorShowBright();
            return;
        }

        if (keyNum == GLSG_KEY_L) {
            addLightObj(objSel, "StandardSpot");
            return;
        }
    */

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    setDrawFlag();
}

void Scene3DBase::keyUp(hidData* data) {
    // if there is an ObjectSelector, get it
    for (auto& cIt : m_camSet)
        if (cIt->s_shaderProto.find(getTypeName<SPObjectSelector>()) != cIt->s_shaderProto.end()) {
            auto objSel = dynamic_cast<SPObjectSelector*>(cIt->s_shaderProto[getTypeName<SPObjectSelector>()].get());
            objSel->keyUp(data);
        }

    if (m_sceneRenderCam->getInteractCam())
        m_sceneRenderCam->getInteractCam()->keyUp(data->key, data->shiftPressed, data->altPressed, data->ctrlPressed);
}

void Scene3DBase::moveObjectByArrowKeys(hidData* data) {
    SPObjectSelector* objSel = nullptr;
    for (auto& cIt : m_camSet)
        if (cIt->s_shaderProto.find(getTypeName<SPObjectSelector>()) != cIt->s_shaderProto.end())
            objSel = dynamic_cast<SPObjectSelector*>(cIt->s_shaderProto[getTypeName<SPObjectSelector>()].get());

    ///> move a selected object via the keyboard, in case a single gizmo axis is
    /// selected
    if ((data->key == GLSG_KEY_DOWN || data->key == GLSG_KEY_UP || data->key == GLSG_KEY_LEFT ||
         data->key == GLSG_KEY_RIGHT) &&
        objSel && objSel->getSelectedNode() && objSel->getSelectedObjectNode() && !m_gizmos.empty()) {
        // check if any of the gizmo axes is selected
        for (auto& g : *m_gizmos[(int)transMode::translate]->getChildren()) {
            if (g->isSelected()) {
                // auto gizAxis = objSel->getSelectedNode();
                auto object = objSel->getSelectedObjectNode();

                vec4 moveVec{0.f};

                if (g->getName() == getTypeName<SNGizmo>() + "_Y_Z") {
                    moveVec.y = data->key == GLSG_KEY_LEFT ? -1.f : data->key == GLSG_KEY_RIGHT ? 1.f : 0.f;
                    moveVec.z = data->key == GLSG_KEY_DOWN ? -1.f : data->key == GLSG_KEY_UP ? 1.f : 0.f;
                } else if (g->getName() == getTypeName<SNGizmo>() + "_X_Z") {
                    moveVec.x = data->key == GLSG_KEY_LEFT ? -1.f : data->key == GLSG_KEY_RIGHT ? 1.f : 0.f;
                    moveVec.z = data->key == GLSG_KEY_DOWN ? -1.f : data->key == GLSG_KEY_UP ? 1.f : 0.f;
                } else if (g->getName() == getTypeName<SNGizmo>() + "_X_Y") {
                    moveVec.x = data->key == GLSG_KEY_LEFT ? -1.f : data->key == GLSG_KEY_RIGHT ? 1.f : 0.f;
                    moveVec.y = data->key == GLSG_KEY_DOWN ? -1.f : data->key == GLSG_KEY_UP ? 1.f : 0.f;
                } else if (g->m_nameFlag & GLSG_TRANS_GIZMO_X)
                    moveVec.x = (data->key == GLSG_KEY_DOWN || data->key == GLSG_KEY_LEFT) ? -1.f : 1.f;
                else if (g->m_nameFlag & GLSG_TRANS_GIZMO_Y)
                    moveVec.y = (data->key == GLSG_KEY_DOWN || data->key == GLSG_KEY_LEFT) ? -1.f : 1.f;
                else if (g->m_nameFlag & GLSG_TRANS_GIZMO_Z)
                    moveVec.z = (data->key == GLSG_KEY_DOWN || data->key == GLSG_KEY_LEFT) ? -1.f : 1.f;

                // rotate selected axis into object space and multiply by
                // gizmoAxis parent (the gizmo container) scaleFact moveVec =
                // *object->getRotMat() * vec4((vec3(moveVec) *
                // *gizAxis->getFirstParentNode()->getScalingVec()), 0.f);
                moveVec = *object->getRotMat() * vec4(vec3(moveVec) * m_keyTransStep[(int)m_cfState], 0.f);

                // offset
                object->translate(*object->getTransVec() + vec3(moveVec));

                if (object->getName() == getTypeName<LICamera>()) dynamic_cast<LICamera*>(object)->setup();
            }
        }

        for (auto& g : *m_gizmos[(int)transMode::rotate]->getChildren()) {
            if (g->isSelected()) {
                auto      object = objSel->getSelectedObjectNode();
                glm::vec3 rotAxis{(float)(g->m_nameFlag & GLSG_ROT_GIZMO_X), (float)(g->m_nameFlag & GLSG_ROT_GIZMO_Y),
                                  (float)(g->m_nameFlag & GLSG_ROT_GIZMO_Z)};

                float     offs = m_keyRotStep[(int)m_cfState];
                glm::mat4 newRot =
                    *object->getRotMat() *
                    glm::rotate((data->key == GLSG_KEY_DOWN || data->key == GLSG_KEY_LEFT) ? -offs : offs, rotAxis);

                // offset
                object->rotate(newRot);
                if (object->getName() == getTypeName<LICamera>()) dynamic_cast<LICamera*>(object)->setup();
            }
        }
    }

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    setDrawFlag();
}

void Scene3DBase::mouseDown(hidData* data) {
    // process the object map at the position of the mouse
    if (m_objSel)
        m_objSel->mouseDownLeft((float)data->mousePos.x - getWinPos().x, (float)data->mousePos.y - getWinPos().y,
                                m_rootNode.get());

    s_mousePos = data->mousePos - getWinPos();

    if (m_sceneRenderCam->getInteractCam())
        m_sceneRenderCam->getInteractCam()->mouseDownLeft((float)data->mousePosNodeRel.x / getSize().x,
                                                          (float)data->mousePosNodeRel.y / getSize().y);

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
    m_leftClick                             = true;
    setDrawFlag();

    data->consumed = true;
}

void Scene3DBase::mouseUp(hidData* data) {
    bool forceResetMouse = false;

    // process the object map at the position of the mouse
    if (m_objSel) {
        // if stopping object translation, show mouse on actual object position
        if (m_objSel->getTransMode() == transMode::translate && m_objSel->getSelectedObjectNode() &&
            m_objSel->getGizmoNode() && m_objSel->gizmoWasDragged()) {
            vec4 offsetVec{0.f, 0.f, 0.f, 1.f};

            float planeScale = 0.29f;
            auto  gizNode    = m_objSel->getGizmoNode();
            auto  selNode    = m_objSel->getSelectedNode();

            if (selNode->getName() == getTypeName<SNGizmo>() + "_Y_Z") {
                offsetVec = vec4{0.f, planeScale, planeScale, 1.f};
            } else if (selNode->getName() == getTypeName<SNGizmo>() + "_X_Z") {
                offsetVec = vec4{planeScale, 0.f, planeScale, 1.f} * planeScale;
            } else if (selNode->getName() == getTypeName<SNGizmo>() + "_X_Y") {
                offsetVec = vec4{planeScale, planeScale, 0.f, 1.f} * planeScale;
            } else if (gizNode->m_nameFlag & GLSG_TRANS_GIZMO) {
                float scaleToArrowCenter = 0.85f;
                offsetVec =
                    vec4(float((m_objSel->getGizmoselected() & GLSG_TRANS_GIZMO_X) != 0) * scaleToArrowCenter,
                         float((m_objSel->getGizmoselected() & GLSG_TRANS_GIZMO_Y) != 0) * scaleToArrowCenter,
                         float((m_objSel->getGizmoselected() & GLSG_TRANS_GIZMO_Z) != 0) * scaleToArrowCenter, 1.f);
            }

            vec4 objPos = m_sceneRenderCam->getProjectionMatr() * m_sceneRenderCam->getModelMatr() *
                          *gizNode->getModelMat() * offsetVec;

            objPos /= objPos.w;
            m_dragStartPos = (vec2(objPos.x, -objPos.y) * 0.5f + 0.5f) * getSize() + getWinPos();

#if !defined(USE_GLFW) && defined(_WIN32)
            if (getWindow()->extGetWinOffs()) m_dragStartPos += getWindow()->extGetWinOffs()();
#endif
            forceResetMouse = true;
        }

        m_objSel->mouseUpLeft(m_rootNode.get());
    }

    if (m_sceneRenderCam->getInteractCam() && data)
        m_sceneRenderCam->getInteractCam()->mouseUpLeft((float)data->mousePosNodeRel.x / getSize().x,
                                                        (float)data->mousePosNodeRel.y / getSize().y);

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    if (getWindow()) {
        // getWindow()->setMouseCursorVisible(true);

        if (data && data->dragging && (!m_leftClick || forceResetMouse || m_leftClickViewDrag)) {
            //  resetMousePos();
            m_leftClickViewDrag = false;
        }
    }

    m_leftClick = false;
    setDrawFlag();

    data->consumed = true;
}

void Scene3DBase::mouseDownRight(hidData* data) {
    s_mousePos = data->mousePos - getWinPos();

    if (m_sceneRenderCam->getInteractCam())
        m_sceneRenderCam->getInteractCam()->mouseDownRight((float)data->mousePosNodeRel.x / getSize().x,
                                                           (float)data->mousePosNodeRel.y / getSize().y);

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    setDrawFlag();

    data->consumed = true;
}

void Scene3DBase::mouseUpRight(hidData* data) {
    if (m_objSel) m_objSel->mouseUpRight(m_rootNode.get());

    if (m_sceneRenderCam->getInteractCam())
        m_sceneRenderCam->getInteractCam()->mouseUpRight((float)data->mousePosNodeRel.x / getSize().x,
                                                         (float)data->mousePosNodeRel.y / getSize().y);

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    if (getWindow()) {
        // getWindow()->setMouseCursorVisible(true);
        // if (data->dragging) resetMousePos();
    }

    setDrawFlag();

    data->consumed = true;
}

void Scene3DBase::mouseDrag(hidData* data) {
    s_mousePos = data->mousePos - getWinPos();

    if (!m_objSel) return;

    if (data->mousePressed && !data->shiftPressed && !data->altPressed && !data->ctrlPressed)  // left mouse
        m_forceMouseUp = true;  // process the object map at the position of the mouse

    if (data->mousePressed) m_objSel->mouseMove(s_mousePos.x, s_mousePos.y);

    if (m_sceneRenderCam->getInteractCam()) {
        if (data->mousePressed && !data->shiftPressed && !data->altPressed && data->ctrlPressed &&
            m_forceMouseUp)  // left mouse with ctrl pressed
        {
            m_forceMouseUp = false;
            mouseUp(nullptr);
        }

        // avoid clashing of object transformation and view dragging
        if (m_objSel->getState() == SPObjectSelector::objSelState::idle) {
            m_sceneRenderCam->getInteractCam()->mouseDrag(data->mousePosNodeRel.x / m_size.x,
                                                          data->mousePosNodeRel.y / m_size.y, data->shiftPressed,
                                                          data->altPressed, data->ctrlPressed, m_mouseRotScale);
            m_leftClickViewDrag = true;
        }
    }

    if (data->dragStart) {
        // if (getWindow())
        //     getWindow()->setMouseCursorVisible(false);

        m_dragStartPos = data->mousePos;
    }

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    setDrawFlag();
    getSharedRes()->requestRedraw = true;

    data->consumed = true;
}

void Scene3DBase::mouseWheel(hidData* data) {
    if (m_sceneRenderCam->getInteractCam()) {
        m_sceneRenderCam->getInteractCam()->setInteractionStart();
        m_sceneRenderCam->getInteractCam()->mouseWheel((float)data->degrees * 0.5f);
    }

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;

    getSharedRes()->requestRedraw = true;

    data->consumed = true;
}

void Scene3DBase::resetMousePos() {
#ifdef ARA_USE_GLFW
    runOnMainThread([this] {
        glfwSetCursorPos(getWindow()->getWinHandle()->getCtx(), m_dragStartPos.x, m_dragStartPos.y);
        return true;
    });
#elif _WIN32
    glm::vec2 winOffs;
    if (getWindow()->extGetWinOffs()) winOffs = getWindow()->extGetWinOffs()();

    // in contrast to GLFW this must be relative to the screen not the window
    SetCursorPos((int)winOffs.x + (int)m_dragStartPos.x * s_sd.contentScale.x,
                 (int)winOffs.y + (int)m_dragStartPos.y * s_sd.contentScale.y);
#endif
}

// input in virtual pixels
void Scene3DBase::setViewport(glm::vec4* viewport) { UINode::setViewport(viewport); }

/**  window resizing takes a bit, so don't rearrange everything immediately */
void Scene3DBase::setViewport(float x, float y, float width, float height) { UINode::setViewport(x, y, width, height); }

void Scene3DBase::updateScene3DBaseViewport(float x, float y, float width, float height) {
    glm::vec4 newVp{x, y, width, height};

    if (glm::all(glm::equal(s_sd.winViewport, newVp))) return;

    memcpy(&s_sd.winViewport[0], &newVp[0], sizeof(glm::vec4));

    // avoid the recalculation of FBO being done on every event
    // unfortunately m_camSet and m_netCamera iteration also has to be done on
    // mouseup
    for (auto& it : m_camSet)
        it->setViewport(0, 0, (uint)s_sd.winViewport.z, (uint)s_sd.winViewport.w,
                        true);  // set the viewport and resize the ShaderProto

    // virtual cameras (NetCameras) have their own Matrix setup
    // (LICamera::setup) since by default Camera sets inside a CameraSets are
    // rebuilt, but those parameters are not available we have to call the
    // NetCameras setup again here
    for (auto& cam : m_netCameras) cam->setup();

    // LOG << "updateScene3DBaseViewport " << x << ", " << y << ", "<< width <<
    // ", "<< height; update camera matrices
    for (auto& c : *m_sceneRenderCam->getCameras()) c.first->updateMatrices();

    if (m_useSsao && m_ssao) m_ssao->resize((uint)s_sd.winViewport.z, (uint)s_sd.winViewport.w);

    if (m_gizmoFbo) m_gizmoFbo->resize((int)s_sd.winViewport.z, (int)s_sd.winViewport.w);

    m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
}

void Scene3DBase::scaleGizmos(float gizmoScale) {
    if (gizmoTree) {
        for (auto& it : *gizmoTree->getChildren()) {
            auto* giz = (SNGizmo*)it;
            giz->setGizmoScreenSize(gizmoScale);
            m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
        }
    }
}

void Scene3DBase::addLightObj(SPObjectSelector* objSel, const string& _type) {
    /*
    // get position to insert
    // get raw depth val at screen center
    float zAtCenter = objSel->getDepthAtScreen(0.f, 0.f,
    m_sceneCamSet->getNear(0), m_sceneCamSet->getFar(0)); if (zAtCenter > 50.f)
            zAtCenter = 0.f;

    // get the distance half the way between the camera and the depthVal
    zAtCenter = -std::abs((m_sceneCamSet->getCamPos(0).z -
    m_sceneCamSet->getNear(0)) + zAtCenter) * 0.5f +
    m_sceneCamSet->getCamPos(0).z; vec4 createPoint = vec4(0.f, 0.f,
    zAtCenter, 1.f);

    // "substract" the cameraMatrix
    createPoint = inverse((*m_camSet.begin())->getModelMatr(0)) * createPoint;

    LIFactory lif;
    lightObjs.push_back((Light*) m_sceneTree->insertChild(0, lif.Create(_type))
    );

    //projectors.push_back(new LIProjector(&scd));
    lightObjs.back()->setVisibility(true);
    lightObjs.back()->setDirection(0.f, 0.f, -1.f);
    lightObjs.back()->translate(createPoint.x, createPoint.y, createPoint.z);

    // quick and dirty s_fbo asigning
    if ( !strcmp(_type.c_str(), "Projector"))
            lightObjs.back()->setColTex(surfManTexs[(lightObjs.size() - 1) %
    surfManTexs.size()]);


    for (auto &cIt : m_camSet)
    {
            ShaderProto *proto = cIt->getShaderProto("SpotLightShadowVsm");
            proto->addLight(lightObjs.back());
    }

    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_ID_PASS] = true;
    */
}

bool Scene3DBase::addCamToSet(LICamera* netCam) {
    if (!m_sceneRenderCam) return false;

    // set the scenefbo size
    netCam->getCamDef()->m_screenWidth  = m_sceneRenderCam->getViewport()->z;
    netCam->getCamDef()->m_screenHeight = m_sceneRenderCam->getViewport()->w;

    // add the camera, remember the iterator position for later deletion
    auto camIt = m_sceneRenderCam->addCamera(netCam->getCamDef(), (void*)netCam);
    netCam->setCsIt(camIt);

    int camInd = (int)(camIt - m_sceneRenderCam->s_cam.begin());

    // register the drawing mutex of this scene to the SceneCameras FBO, this
    // way it can be used to sync drawing, when sharing
    // m_sceneRenderCam->getFbo()->setSharedDrawMtx(&s_drawMtx);

    // since we share one layer of the scenes cameras fbo which may change
    // resolution or be remove we need to update the shared reference, when
    // changes happen
    m_sceneRenderCam->addUpdtCb([this, netCam, camInd]() { netCam->setCsFboPtr(m_sceneRenderCam->getFbo(), camInd); },
                                netCam);

    // get a view onto the FBO corresponding to this camera, this is a
    // multilayer fbo
    netCam->setCsFboPtr(m_sceneRenderCam->getFbo(), camInd);

    // mark the Camera's visible representation to be skipped during rendering
    // when it is used (avoid rendering the camera's visible representation into
    // it's own framebuffer)
    netCam->iterateNodeParent(netCam, netCam->getFirstParentNode(), [camInd](SceneNode* node, SceneNode* parent) {
        node->m_skipForCamInd[parent] = camInd;
        return true;
    });

    // have the gizmos not being rendered into anything else than the base view
    gizmoTree->iterateNodeParent(gizmoTree, m_rootNode.get(), [camInd](SceneNode* node, SceneNode* parent) {
        node->m_skipForCamInd[parent] = camInd;
        return true;
    });

    // set a callback in the shaderproptype, when the camera setup was called -
    // that is the camera configuration has changed
    netCam->clearSetupCb();
    netCam->pushSetupCb([this]() { m_sceneRenderCam->buildCamMatrixArrays(); });

    m_netCameras.push_back(netCam);

    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
    return true;
}

void Scene3DBase::removeCamFromSet(LICamera* netCam) {
    if (m_netCameras.empty() || !netCam) return;

    m_sceneRenderCam->removeCamera((void*)netCam);

    auto c =
        std::find_if(m_netCameras.begin(), m_netCameras.end(), [netCam](const LICamera* lic) { return lic == netCam; });
    if (c != m_netCameras.end()) m_netCameras.erase(c);
}

void Scene3DBase::addCameraViewRelative(SPObjectSelector* objSel) {
    // get position to insert
    // get raw depth val at screen center
    float zAtCenter = objSel->getDepthAtScreen(0.f, 0.f, m_sceneRenderCam->getNear(), m_sceneRenderCam->getFar());
    if (zAtCenter > 50.f) zAtCenter = 0.f;

    // get the distance half the way between the camera and the depthVal
    zAtCenter = -std::abs((m_sceneRenderCam->getCamPos().z - m_sceneRenderCam->getNear()) + zAtCenter) * 0.5f +
                m_sceneRenderCam->getCamPos().z;
    vec4 createPoint = vec4(0.f, 0.f, zAtCenter, 1.f);

    // "substract" the cameraMatrix
    createPoint = inverse((*m_camSet.begin())->getModelMatr()) * createPoint;

    m_netCameras.push_back((LICamera*)m_sceneTree->insertChild(0, make_unique<LICamera>()));
    m_netCameras.back()->setVisibility(true);
    m_netCameras.back()->translate(createPoint.x, createPoint.y, createPoint.z);

    m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
    m_reqRenderPasses[GLSG_OBJECT_ID_PASS]  = true;
}

void Scene3DBase::setFloorVisibility(bool val) {
    if (m_gridFloor) m_gridFloor->setVisibility(val);
    if (m_gridFloorPre) m_gridFloorPre->setVisibility(val);
    if (m_gridFloorAxes) m_gridFloorAxes->setVisibility(val);
    if (m_gridFloorAxesPre) m_gridFloorAxesPre->setVisibility(val);
}

void Scene3DBase::setWorldAxesVisibility(bool val, float ndcSize) {
    m_worldAxisNdcSize = ndcSize;

    if (m_sceneGizmoPre) m_sceneGizmoPre->m_staticNDCSize = ndcSize;

    if (m_sceneGizmo) m_sceneGizmo->m_staticNDCSize = ndcSize;

    setWorldAxesVisibility(val);
}

void Scene3DBase::setWorldAxesVisibility(bool val) {
    m_worldAxisVisibility = val;

    if (m_worldAxes) m_worldAxes->setVisibility(val);
    if (m_worldAxesPre) m_worldAxesPre->setVisibility(val);
    if (m_sceneGizmoPre) m_sceneGizmoPre->setVisibility(val);
    if (m_sceneGizmo) m_sceneGizmo->setVisibility(val);
}

void Scene3DBase::swapCameras(TrackBallCam* cam1, TrackBallCam* cam2) {
    if (!m_sceneRenderCam || !cam1 || !cam2) return;
    m_sceneRenderCam->swapCameras(cam1, cam2);

    auto camSet = m_sceneRenderCam->getCameras();

    list<TrackBallCam*> camList = {cam1, cam2};
    for (auto& cam : camList) {
        // get the pointer of the cam in the scene camera set
        auto camIt = std::find_if(camSet->begin(), camSet->end(),
                                  [cam](const pair<Camera*, void*>& p) { return p.first == cam; });
        if (camIt == camSet->end()) continue;

        // don't process the default scene camera (which contains the
        // m_sceneCamSet pointer as second entry)
        if (camIt->second && camIt->second != m_sceneRenderCam.get()) {
            int  camInd = (int)(camIt - camSet->begin());
            auto netCam = static_cast<LICamera*>(camIt->second);

            // mark the Camera's visible representation to be skipped during
            // rendering when it is used (avoid rendering the camera's visible
            // representation into its own framebuffer)
            netCam->iterateNodeParent(netCam, netCam->getFirstParentNode(),
                                      [camInd](SceneNode* node, SceneNode* parent) {
                                          node->m_skipForCamInd[parent] = camInd;
                                          return true;
                                      });

            // skip the gizmo drawing when the LICamera is rendered
            if (gizmoTree)
                gizmoTree->iterateNodeParent(gizmoTree, m_rootNode.get(), [camInd](SceneNode* node, SceneNode* parent) {
                    node->m_skipForCamInd[parent] = camInd;
                    return true;
                });

            if (cam == m_sceneRenderCam->getInteractCam()) {
                // trackball needs to update the cameras SceneNodes modelmatrix
                cam->setUptCamSceneNodeCb([netCam](TbModData& data) {
                    if (data.trans) netCam->translate(data.trans);
                    if (data.rotQ) netCam->rotate(glm::angle(*data.rotQ), glm::axis(*data.rotQ));
                });
            } else {
                cam->removeUpdtCamSceneNodeCb();
            }
        }
    }

    m_sceneRenderCam->buildCamMatrixArrays();

    // HACK: hide scene world axis gizmo for the not scene cam ... needs a
    // generic solution
    if (m_sceneGizmo && m_sceneGizmoPre) {
        int skipInd = m_sceneRenderCam->getInteractCam() == m_sceneCam ? 1 : 0;

        m_sceneGizmo->m_skipForCamInd[m_sceneTreeCont] = skipInd;
        for (auto& it : *m_sceneGizmo->getChildren()) it->m_skipForCamInd[m_sceneGizmo] = skipInd;

        m_sceneGizmoPre->m_skipForCamInd[m_sceneTreeCont] = skipInd;
        for (auto& it : *m_sceneGizmoPre->getChildren()) it->m_skipForCamInd[m_sceneGizmoPre] = skipInd;
    }
}

void Scene3DBase::addShaderProto(const std::string* shdrName) {
    for (auto& cIt : m_camSet) cIt->addShaderProto(*shdrName, {GLSG_SHADOW_MAP_PASS, GLSG_SCENE_PASS, GLSG_GIZMO_PASS});

    //-----------------------------------------------------------------------
    // there might be parameter changes, before the scene has been m_inited, if
    // this is the case, process them now.
    for (auto& it : m_preSceneShdrProtoPar) {
        if (!strcmp(it.first.c_str(), shdrName->c_str())) {
            for (auto& cb : it.second) cb(this);
            it.second.clear();
        }
    }
}

void Scene3DBase::setBasePlane(basePlane bp) {
    m_basePlane = bp;

    if (m_gridFloorPre) m_gridFloorPre->setBasePlane(bp);
    if (m_gridFloorAxesPre) m_gridFloorAxesPre->setBasePlane(bp);
    if (m_gridFloor) m_gridFloor->setBasePlane(bp);
    if (m_gridFloorAxes) m_gridFloorAxes->setBasePlane(bp);
}

ShaderProto* Scene3DBase::getShaderProto(uint ind, string&& name) {
    if (m_camSet.size() > ind)
        return m_camSet[ind]->getShaderProto(name);
    else
        return nullptr;
}

SceneNode* Scene3DBase::getModelSN(string&& name) {
    if (!strcmp(name.c_str(), "SpotLight"))
        return spotLightSN.get();
    else if (name == getTypeName<LIProjector>())
        return projectorSN.get();
    else if (name == getTypeName<LICamera>())
        return netCamSN.get();
    else
        return nullptr;
}

SceneNode* Scene3DBase::getModelSN(string* name) {
    if (!strcmp(name->c_str(), "SpotLight"))
        return spotLightSN.get();
    else if ((*name) == getTypeName<LIProjector>())
        return projectorSN.get();
    else if ((*name) == getTypeName<LICamera>())
        return netCamSN.get();
    else
        return nullptr;
}

void Scene3DBase::selectObj(int objId) {
    if (m_inited)
        for (auto& cIt : m_camSet) {
            auto objSelector = (SPObjectSelector*)cIt->getShaderProto(getTypeName<SPObjectSelector>());
            if (!objSelector->getLastSceneTree()) objSelector->setLastSceneTree(m_sceneTree);
            objSelector->selectObj(objId, true);
            m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
        }
}

void Scene3DBase::setAddGizmoCb(std::function<void(transMode)> f) {
    if (m_inited)
        for (auto& cIt : m_camSet) {
            auto objSelector = (SPObjectSelector*)cIt->getShaderProto(getTypeName<SPObjectSelector>());
            if (objSelector) objSelector->setAddGizmoCb(std::move(f));
        }
    else
        m_addGizmoCb = std::move(f);
}

void Scene3DBase::addPassiveGizmo(SceneNode* node, float objRelativeSize, float worldScale) {
    if (node) {
        // construct a translation gizmo
        auto gizmo = node->addChild(make_unique<SNGizmo>(transMode::passive));
        gizmo->setScene((WindowBase*)getWindow());
        gizmo->setVisibility(true);
        gizmo->setName("passive_gizmo");

        // position onto the node's center
        gizmo->translate(
            ((*node->getBoundingBoxMax() - *node->getBoundingBoxMin()) * 0.5f + *node->getBoundingBoxMin()) *
            worldScale);

        // scale relative to node
        float diag = std::sqrt(glm::length(*node->getBoundingBoxMax() - *node->getBoundingBoxMin())) * objRelativeSize *
                     worldScale;
        gizmo->scale(diag, diag, diag);

        // set a screen relative limit for the size of the gizmo
        gizmo->m_maxNDCSize = 0.2f;
    }
}

void Scene3DBase::removePassiveGizmo(SceneNode* node) {
    if (node) {
        auto ptr = std::find_if(node->getChildren()->begin(), node->getChildren()->end(),
                                [](SceneNode* nd) { return nd->getName() == "passive_gizmo"; });
        if (ptr != node->getChildren()->end()) node->removeChild(*ptr);
    }
}

void Scene3DBase::deselectAll() {
    for (auto& cIt : m_camSet)
        if (cIt->s_shaderProto.find(getTypeName<SPObjectSelector>()) != cIt->s_shaderProto.end())
            ((SPObjectSelector*)cIt->s_shaderProto[getTypeName<SPObjectSelector>()].get())->deselect();

    if (getSceneTree()) getSceneTree()->setSelected(false, nullptr, false);

    if (m_deselectAllCb) m_deselectAllCb();
}

void Scene3DBase::setCfState(cfState cf) {
    if (m_cfState == cf) return;

    m_cfState = cf;

    for (auto& cIt : m_camSet)
        if (cIt->s_shaderProto.find(getTypeName<SPObjectSelector>()) != cIt->s_shaderProto.end()) {
            auto objSel = dynamic_cast<SPObjectSelector*>(cIt->s_shaderProto[getTypeName<SPObjectSelector>()].get());
            if (objSel->getCfState() != cf) objSel->setCfState(cf);
        }
}

Scene3DBase::~Scene3DBase() { freeGLResources(); }

}  // namespace ara
