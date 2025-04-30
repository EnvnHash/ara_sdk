//
//  SceneNode.cpp
//
//  Created by Sven Hahne on 17.07.14.
//

#include "SceneNode.h"

#include "CameraSets/CameraSet.h"
#include "WindowManagement/WindowBase.h"

using namespace glm;
using namespace std;

namespace ara {
SceneNode::SceneNode(sceneData* sd) : s_sd(sd), m_glbase(sd ? sd->glbase : nullptr) {
    // by default enable the node for all renderPasses
    for (auto i = 0; i < GLSG_NUM_RENDER_PASSES; i++) {
        m_renderPassEnabled[static_cast<renderPass>(i)] = true;
    }
}

void SceneNode::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    if (m_vao && m_renderPassEnabled[pass]) {
        uint texCnt = 0;
        glBlendFuncSeparate(m_blendSrc, m_blendDst, m_blendSrcAlpha, m_blendDstAlpha);

#ifndef ARA_USE_GLES31
        if (!m_polyFill) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            if (pass == GLSG_OBJECT_MAP_PASS) {
                glDisable(GL_BLEND);
            }
        }
#endif

        if (!getTextures()->empty()) {
            for (uint t = 0; t < int(getTextures()->size()); t++) {
                glActiveTexture(GL_TEXTURE0 + t);
                shader->setUniform1i("tex" + std::to_string(t), t);
                getTextures()->at(t)->bind();
                ++texCnt;
            }

            shader->setUniform1i("hasTexture", 1);
        } else {
            shader->setUniform1i("hasTexture", 0);
        }

        shader->setUniform2iv("flipTc", &m_flipTc[0]);
        shader->setUniform2iv("invertTc", &m_invertTc[0]);
        shader->setUniform1i("normTexCoord", (int)m_useNormalizedTexCoord);
        shader->setUniform2fv("texMin", &m_minTexC[0]);
        shader->setUniform2fv("texMax", &m_maxTexC[0]);
        shader->setUniform2f("resolution", cs->getActFboSize()->x, cs->getActFboSize()->y);
        shader->setUniform1i("lumaKey", (int)m_lumaKey);
        shader->setUniform1f("lumaThresLow", m_lumaKeySSLow);
        shader->setUniform1f("lumaThresHigh", m_lumaKeySSHigh);
        shader->setUniform1i("isYuv", m_isYuv);    // y
        shader->setUniform1i("isNv12", m_isNv12);  // y

        glActiveTexture(GL_TEXTURE0);
        shader->setUniform1i("hasTexture", static_cast<int>(m_extTexId != 0));

        shader->setUniform1i("tex0", 0);
        glBindTexture(GL_TEXTURE_2D, m_extTexId);

        glActiveTexture(GL_TEXTURE1);
        shader->setUniform1i("tex1", 1);
        glBindTexture(GL_TEXTURE_2D, m_extTexId1);

        glActiveTexture(GL_TEXTURE2);
        shader->setUniform1i("tex2", 2);
        glBindTexture(GL_TEXTURE_2D, m_extTexId2);

        shader->setUniform1i("polyFill", (int)m_polyFill);
        shader->setUniform1i("drawGridTexture", m_drawGridTex && m_polyFill);

        if (m_drawGridTex) {
            shader->setUniform2fv("gridNrSteps", &m_gridNrSteps[0]);
            shader->setUniform1f("gridBgAlpha", m_gridBgAlpha);
            shader->setUniform1f("gridLineThickness", m_gridLineThick);
        }

        m_cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        m_depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);

        if (m_winding != GL_CCW) {
            glFrontFace(m_winding);
        }

        m_drawIndexed ? m_vao->drawElements(GL_TRIANGLES) : m_vao->draw(GL_TRIANGLES);

#ifndef ARA_USE_GLES31
        if (!m_polyFill) {
            shader->setUniform1i("polyFill", 1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (pass == GLSG_OBJECT_MAP_PASS) glEnable(GL_BLEND);
        }
#endif

        if (m_winding != GL_CCW) glFrontFace(GL_CCW);

        shader->setUniform1i("hasTexture", 0);
    }

    if (m_drawEndCb) {
        m_drawEndCb(time);
    }
}

void SceneNode::update(double time, double dt, CameraSet* cs) {
    if (m_staticNDCSize > 0.f || m_maxNDCSize > 0.f) {
        // calculate the node's center in camera space
        m_tempProj2DVecStart = cs->getModelMatr() * modelMat * vec4(0.f, 0.f, 0.f, 1.f);
        // offset the node's center parallel to the screen, the resulting vector
        // corresponds to the bounding sphere around the node in camera space
        m_tempProj2DVecEnd = m_tempProj2DVecStart + vec4(1.f, 0.f, 0.f, 0.f);

        // project the node's center from camera space to NDC
        m_tempCamProjViewMat = cs->getProjectionMatr();
        m_tempProj2DVecStart = m_tempCamProjViewMat * m_tempProj2DVecStart;
        if (m_tempProj2DVecStart.w < 0.0001f) return;
        m_tempProj2DVecStart /= m_tempProj2DVecStart.w;

        // project the vectors end into NDC
        m_tempProj2DVecEnd = m_tempCamProjViewMat * m_tempProj2DVecEnd;
        if (m_tempProj2DVecEnd.w < 0.0001f) return;
        m_tempProj2DVecEnd /= m_tempProj2DVecEnd.w;

        // calculate the vector's length in NDC
        m_tempDiffVec = m_tempProj2DVecEnd - m_tempProj2DVecStart;

        // in pixels
        if (!s_sd) return;
        float actScreenLength = length(vec2(m_tempDiffVec) * 0.5f * *cs->getActFboSize() / s_sd->contentScale);

        // limit or adjust the node's scaling to the desired values
        float scaleFact = 1.f;
        if (m_staticNDCSize > 0.f)
            scaleFact = m_staticNDCSize / actScreenLength;
        else if (m_maxNDCSize > 0.f)
            scaleFact = std::min<float>(m_maxNDCSize, actScreenLength) / actScreenLength;

        if (m_scaleVec.x != scaleFact || m_scaleVec.y != scaleFact || m_scaleVec.z != scaleFact) {
            scale(scaleFact, scaleFact, scaleFact);

            // force an update of the gizmos model matrices
            rebuildModelMat();
            for (const auto& child : *getChildren()) child->m_hasNewModelMat = true;
        }
    }
}

void SceneNode::assignTexUnits(Shaders* _shader) {
    for (const auto& it : m_auxTex) _shader->setUniform1i(it.name, it.unitNr);  // muss gesetzt werden, sonst glsl fehler
}

void SceneNode::useTextureUnitInd(int _unit, int _ind, Shaders* _shader, TFO* _tfo) {
    assignTexUnits(_shader);

    glActiveTexture(GL_TEXTURE0 + _unit);
    glBindTexture(GL_TEXTURE_2D, _ind);

    if (_tfo) _tfo->addTexture(_unit, _ind, GL_TEXTURE_2D, "texs");

    for (const auto& it : m_auxTex) {
        glActiveTexture(GL_TEXTURE0 + it.unitNr);
        glBindTexture(it.target, it.texNr);

        if (_tfo) _tfo->addTexture(it.unitNr, it.texNr, it.target, it.name);
    }
}

void SceneNode::setParentNode(SceneNode* _parent) {
    // check if there is an invalid parent
    list<list<SceneNode*>::iterator> toKill;
    for (auto it = m_parents.begin(); it != m_parents.end(); ++it) {
        auto foundSelf = std::find((*it)->m_children.begin(), (*it)->m_children.end(), this);
        if (foundSelf == (*it)->m_children.end()) toKill.emplace_back(it);
    }

    for (const auto& it : toKill) {
        m_parents.erase(it);
    }

    // if this parent hasn't been registered, do it now
    auto it = std::find(m_parents.begin(), m_parents.end(), _parent);
    if (it == m_parents.end()) m_parents.push_back(_parent);

    // recheck children for correct rootNode
    for (const auto& it : m_children) {
        it->setRootNode(nullptr);
    }

    for (const auto& it : m_changedParentCb) {
        it();
    }
}

SceneNode* SceneNode::addChild(bool updtNodeIds) {
    m_int_children.push_back(make_unique<SceneNode>(s_sd));
    m_int_children.back()->setParentNode(this);
    m_int_children.back()->setScene(m_scene);
    m_children.push_back(m_int_children.back().get());
    if (updtNodeIds && getRootNode()) regenNodeIds(m_rootNode->getObjId(this));

    return m_int_children.back().get();
}

SceneNode* SceneNode::addChild(unique_ptr<SceneNode>&& newNode, bool updtNodeIds) {
    m_int_children.push_back(std::move(newNode));
    m_int_children.back()->setParentNode(this);
    m_int_children.back()->setScene(m_scene);
    m_int_children.back()->setSceneData(s_sd);
    m_children.push_back(m_int_children.back().get());
    if (updtNodeIds && getRootNode()) {
        regenNodeIds(m_rootNode->getObjId(this));
    }
    return m_int_children.back().get();
}

void SceneNode::addChildRef(SceneNode* newNode, bool updtNodeIds) {
    m_children.push_back(newNode);
    m_children.back()->setParentNode(this);
    m_children.back()->setScene(m_scene);
    m_children.back()->setSceneData(s_sd);
    if (updtNodeIds && getRootNode()) {
        regenNodeIds(m_rootNode->getObjId(this));
    }
}

SceneNode* SceneNode::insertChild(uint _ind, bool updtNodeIds) {
    auto newIt = m_int_children.insert(m_int_children.begin() + _ind, make_unique<SceneNode>());
    (*newIt)->setParentNode(this);
    (*newIt)->setScene(m_scene);
    (*newIt)->setSceneData(s_sd);
    m_children.push_back((*newIt).get());
    if (updtNodeIds && getRootNode()) {
        regenNodeIds(m_rootNode->getObjId(this));
    }
    return newIt->get();
}

SceneNode* SceneNode::insertChild(uint _ind, unique_ptr<SceneNode> newScene, bool updtNodeIds) {
    auto newIt = m_int_children.insert(m_int_children.begin() + _ind, std::move(newScene));
    (*newIt)->setParentNode(this);
    (*newIt)->setScene(m_scene);
    (*newIt)->setSceneData(s_sd);
    m_children.push_back((*newIt).get());
    if (updtNodeIds && getRootNode()) {
        regenNodeIds(m_rootNode->getObjId(this));
    }
    return newIt->get();
}

void SceneNode::clearChildren() {
    // clear bottom up
    for (const auto& it : m_int_children) {
        it->clearChildren();
    }

    if (!m_int_children.empty()) {
        m_int_children.clear();
    }

    if (!m_children.empty()) {
        m_children.clear();
    }

    unregister();
}

void SceneNode::removeChild(SceneNode* node) {
    // remove the nodes' reference from the m_children array
    iterateNode(this, [&node](SceneNode* thisNode) {
        bool contIt = true;
        auto it     = std::find(thisNode->getChildren()->begin(), thisNode->getChildren()->end(), node);
        if (it != thisNode->getChildren()->end()) {
            contIt = false;
            thisNode->getChildren()->erase(it);
            node = nullptr;
        }

        return contIt;
    });

    if (!node) return;

    // remove the nodes in case it is part of the m_int_children array
    iterateNode(this, [node](SceneNode* thisNode) {
        auto intChildren = thisNode->getIntChildren();
        if (intChildren->empty()) return false;
        bool contIt = true;

        auto it = std::find_if(intChildren->begin(), intChildren->end(),
                               [node](const unique_ptr<SceneNode>& i) { return i.get() == node; });
        if (it != intChildren->end()) {
            contIt = false;
            it     = intChildren->erase(it);
        }
        return contIt;
    });
}

void SceneNode::removeChildDontKill(SceneNode* node) {
    auto it = std::find(m_children.begin(), m_children.end(), node);

    if (it != m_children.end())
        m_children.erase(it);
    else
        LOGE << "SceneNode::removeChildDontKill Error, couldn't find sceneNode "
                "to kill";
}

void SceneNode::removeChild(std::string searchName) {
    // remove the nodes' reference from the m_children array
    iterateNode(this, [searchName](SceneNode* thisNode) {
        vector<SceneNode*>* children = thisNode->getChildren();
        bool                contIt   = true;
        for (vector<SceneNode*>::iterator childIt = children->begin(); childIt != children->end(); ++childIt)
            if ((*childIt)->getName() == searchName) {
                contIt = false;
                children->erase(childIt);
            }

        return contIt;
    });

    // remove the nodes in case it is part of the m_int_children array
    iterateNode(this, [searchName](SceneNode* thisNode) {
        vector<unique_ptr<SceneNode>>* intChildren = thisNode->getIntChildren();
        bool                           contIt      = true;
        for (vector<unique_ptr<SceneNode>>::iterator it = intChildren->begin(); it != intChildren->end(); ++it)
            if ((*it)->getName() == searchName) {
                contIt = false;
                intChildren->erase(it);
            }

        return contIt;
    });
}

SceneNode* SceneNode::getParentNode(int objId) {
    auto it = ranges::find_if(m_nodeObjId,
                              [objId](const std::pair<SceneNode*, int>& t) -> bool { return t.second == objId; });

    if (it != m_nodeObjId.end())
        return it->first;
    else
        return nullptr;
}

SceneNode* SceneNode::getRootNode() {
    // if the SceneNode's root node is equal to itself, check again
    if (!m_rootNode || (m_rootNode && m_rootNode == this)) {
        // get the root node
        SceneNode* root = this;
        while (root->getFirstParentNode()) {
            root = root->getFirstParentNode();
        }

        // to have the object map generation succeed also before all SceneNodes are assigned, allow also the
        // rootNode to be equal to the SceneNode itself (in case it has not yet been assigned) if (root != this)
        m_rootNode = root;
    }

    return m_rootNode;
}

SceneNode* SceneNode::getNode(std::string searchName) {
    SceneNode* node  = nullptr;
    bool       found = false;
    uint       cntr  = 0;

    while (!found && cntr < m_children.size()) {
        if (m_children[cntr]->m_name == searchName) {
            node  = m_children[cntr];
            found = true;
        }

        ++cntr;
    }

    return node;
}

bool SceneNode::findChild(SceneNode* node) {
    bool found = false;

    iterateNode(this, [node, &found](SceneNode* thisNode) {
        bool contIt = true;
        const auto& it     = ranges::find(*thisNode->getChildren(), node);
        if (it != thisNode->getChildren()->end()) {
            found  = true;
            contIt = false;
        }

        return contIt;
    });

    return found;
}

bool SceneNode::setSelected(bool val, SceneNode* parent, bool procCb) {
    if (m_selectable) {
        if (parent && m_selected.count(parent)) {
            m_selected[parent] = val;
        } else if (!parent && getFirstParentNode() && m_selected.count(getFirstParentNode())) {
            m_selected[getFirstParentNode()] = val;
        }

        // traverse all children down and also set them selected
        for (const auto& child : *getChildren()) {
            child->setSelected(val, this, procCb);
        }

        if (procCb) {
            for (const auto& it : m_clickCb | views::values) {
                it();
            }
        }
        return true;
    } else {
        return false;
    }
}

bool SceneNode::isSelected(SceneNode* parent) {
    if (m_selected.contains(parent)) {
        return m_selected[parent];
    } else if (!parent && getFirstParentNode() && m_selected.count(getFirstParentNode())) {
        return m_selected[getFirstParentNode()];
    } else {
        return false;
    }
}

void SceneNode::setBoundingBox(float minX, float maxX, float minY, float maxY, float minZ, float maxZ) {
    m_boundingBoxMin = vec3(minX, minY, minZ);
    m_boundingBoxMax = vec3(maxX, maxY, maxZ);
    m_vaoDimension   = m_boundingBoxMax - m_boundingBoxMin;
    m_dimension      = m_vaoDimension * m_scaleVec;
    m_hasBoundingBox = true;
    m_recalcCenter   = true;
}

void SceneNode::setBoundingBox(vec3* boundMin, vec3* boundMax) {
    m_boundingBoxMin = *boundMin;
    m_boundingBoxMax = *boundMax;
    m_vaoDimension   = *boundMax - *boundMin;
    m_dimension      = m_vaoDimension * m_scaleVec;
    m_hasBoundingBox = true;
    m_recalcCenter   = true;
}

bool SceneNode::iterateNode(SceneNode* node, itNodeCbFunc cbFunc) {
    // apply function also to the start SceneNode
    bool contIteration = cbFunc(node);
    if (!contIteration) {
        return false;
    }

    // iterate children
    for (const auto& it : *node->getChildren()) {
        if (!iterateNode(it, cbFunc)) {
            return false;
        }
    }

    return contIteration;
}

bool SceneNode::iterateNodeParent(SceneNode* node, SceneNode* parent, itNodeParentCbFunc cbFunc) {
    // apply function also to the start SceneNode
    bool contIteration = cbFunc(node, parent);
    if (!contIteration) {
        return false;
    }

    // set all childrens resetIDs to true
    for (const auto& it : *node->getChildren()) {
        if (!iterateNodeParent(it, node, cbFunc)) {
            return false;
        }
    }

    return contIteration;
}
/**
 * is always done for the whole SceneNode Tree, this Node is part of.
 */
uint SceneNode::regenNodeIds(uint idOffs) {
    uint objIdCntr = idOffs;

    SceneNode* root = getRootNode();
    if (root) {
        root->m_sceneNodeMap.clear();

        iterateNodeParent(root, nullptr, [root, &objIdCntr](SceneNode* node, SceneNode* parent) {
            node->m_nodeObjId[parent] = objIdCntr;
            node->m_selected[parent]  = node->isSelected(parent);
            if (node->getExtIdGroup()) node->getExtIdGroup()->ids[node] = objIdCntr;
            root->m_sceneNodeMap[objIdCntr] = node;

            ++objIdCntr;
            return true;
        });
        return objIdCntr;
    } else {
        return 0;
    }
}

void SceneNode::setSingleId(bool val) {
    auto ig        = createIdGroup();
    auto startnode = this;

    iterateNode(this, [startnode, ig, val](SceneNode* node) {
        if (startnode != node) {
            node->setIdGroup(val ? ig : nullptr);
        }
        return true;
    });

    if (getRootNode()) {
        regenNodeIds(m_rootNode->getObjId(this));
    }
}

void SceneNode::translate(float x, float y, float z) {
    if (m_transVec.x != x || m_transVec.y != y || m_transVec.z != z) {
        m_transVec.x                     = x;
        m_transVec.y                     = y;
        m_transVec.z                     = z;
        m_transMat                       = glm::translate(mat4(1.f), m_transVec);
        m_hasNewModelMat                 = true;
        m_recalcCenter                   = true;
        getRootNode()->m_calcMatrixStack = true;
    }
}

void SceneNode::translate(vec3&& transVec) {
    if (!glm::all(glm::equal(transVec, m_transVec))) {
        m_transVec.x     = transVec.x;
        m_transVec.y     = transVec.y;
        m_transVec.z     = transVec.z;
        m_transMat       = glm::translate(mat4(1.f), m_transVec);
        m_hasNewModelMat = true;
        m_recalcCenter   = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::translate(vec3* transVec) {
    if (!glm::all(glm::equal(*transVec, m_transVec))) {
        m_transVec.x     = transVec->x;
        m_transVec.y     = transVec->y;
        m_transVec.z     = transVec->z;
        m_transMat       = glm::translate(mat4(1.f), m_transVec);
        m_hasNewModelMat = true;
        m_recalcCenter   = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::rotate(float angle, float x, float y, float z) {
    m_rotAngle  = angle;
    m_rotAxis.x = x;
    m_rotAxis.y = y;
    m_rotAxis.z = z;
    m_newRotMat = glm::rotate(m_rotAngle, m_rotAxis);

    if (!glm::all(glm::equal(m_newRotMat, m_rotMat))) {
        m_rotMat         = m_newRotMat;
        m_hasNewModelMat = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::rotate(float angle, vec3* axis) {
    m_rotAngle  = angle;
    m_rotAxis   = *axis;
    m_newRotMat = glm::rotate(m_rotAngle, m_rotAxis);

    if (!glm::all(glm::equal(m_newRotMat, m_rotMat))) {
        m_rotMat         = m_newRotMat;
        m_hasNewModelMat = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::rotate(float angle, vec3&& axis) {
    m_rotAngle  = angle;
    m_rotAxis   = axis;
    m_newRotMat = glm::rotate(m_rotAngle, m_rotAxis);

    if (!glm::all(glm::equal(m_newRotMat, m_rotMat))) {
        m_rotMat         = m_newRotMat;
        m_hasNewModelMat = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::rotate(mat4& rot) {
    if (!glm::all(glm::equal(rot, m_rotMat))) {
        m_rotMat = rot;

        // convert to angle axis
        quat rotQuat     = toQuat(rot);
        m_rotAngle       = glm::angle(rotQuat);
        m_rotAxis        = glm::axis(rotQuat);
        m_hasNewModelMat = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::scale(float x, float y, float z) {
    if (x != m_scaleVec.x || y != m_scaleVec.y || z != m_scaleVec.z) {
        m_scaleVec.x     = x;
        m_scaleVec.y     = y;
        m_scaleVec.z     = z;
        m_scaleMat       = glm::scale(m_scaleVec);
        m_hasNewModelMat = true;
        m_recalcCenter   = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::scale(vec3* scaleVec) {
    if (!glm::all(glm::equal(m_scaleVec, *scaleVec))) {
        m_scaleVec       = *scaleVec;
        m_scaleMat       = glm::scale(*scaleVec);
        m_hasNewModelMat = true;
        m_recalcCenter   = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

void SceneNode::scale(vec3&& scaleVec) {
    if (!glm::all(glm::equal(m_scaleVec, scaleVec))) {
        m_scaleVec       = scaleVec;
        m_scaleMat       = glm::scale(scaleVec);
        m_hasNewModelMat = true;
        m_recalcCenter   = true;
        if (getRootNode()) {
            m_rootNode->m_calcMatrixStack = true;
        }
    }
}

glm::vec3& SceneNode::getCenter() {
    if (m_recalcCenter) {
        m_recalcCenter = false;
        m_center       = (m_boundingBoxMax - m_boundingBoxMin) * 0.5f + m_boundingBoxMin;
        m_center       = m_center * m_scaleVec + m_transVec;
    }
    return m_center;
}

void SceneNode::rebuildModelMat(SceneNode* parent) {
    // build the relative model matrix (relative to its parent)
    modelMat = m_transMat * m_rotMat * m_scaleMat;

    // build the absolute model matrix
    m_absModelMat[parent]  = m_parentModelMat[parent] * modelMat;
    m_absNormalMat[parent] = inverseTranspose(mat3(m_absModelMat[parent]));

    // update dimension, this is the initial dimension multiplied by scaling
    m_dimension = m_vaoDimension * m_scaleVec;

    // execute optional Callbacks that have been notified
    for (auto& it : m_modelMatChangedCb | views::values) {
        it(this);
    }
}

void SceneNode::setModelMat(glm::mat4& mm) {
    if (!glm::all(glm::equal(modelMat, mm))) {
        vec3 dcTrans, dcScale, dcSkew;
        quat dcOri;
        vec4 dcPersp;
        glm::decompose(inverse(mm), dcScale, dcOri, dcTrans, dcSkew, dcPersp);

        scale(1.f / dcScale);
        translate(-dcTrans);
        auto  conj  = conjugate(dcOri);
        float angle = -glm::angle(dcOri);
        vec3  axis  = glm::axis(dcOri);
        if (!std::isnan(angle) && !std::isnan(axis[0]) && !std::isnan(axis[1]) && !std::isnan(axis[2])) {
            rotate(angle, std::move(axis));
        }

        modelMat         = m_transMat * m_rotMat * m_scaleMat;
        m_hasNewModelMat = true;
    }
}

void SceneNode::setParentModelMat(SceneNode* parent, mat4& _inMat) {
    m_parentModelMat[parent] = _inMat;
    rebuildModelMat(parent);
}

mat4* SceneNode::getModelMat() {
    return !m_parents.empty() ? &m_absModelMat[*m_parents.begin()] : nullptr;
}

mat3* SceneNode::getNormalMat() {
    return !m_parents.empty() ? &m_absNormalMat[*m_parents.begin()] : nullptr;
}

uint SceneNode::getNrSubNodes() {
    uint nrNodes = 0;

    iterateNode(this, [&nrNodes](SceneNode* thisNode) {
        ++nrNodes;
        return true;
    });

    return nrNodes;
}

int SceneNode::getObjId(SceneNode* parent) {
    if (!parent) {
        parent = getFirstParentNode();
    }
    return (m_nodeObjId.empty() || !m_nodeObjId.contains(parent)) ? 0 : m_nodeObjId[parent];
}

SceneNode* SceneNode::getNodeWithID(int id) {
    SceneNode* root = getRootNode();
    if (!root) {
        return nullptr;
    }

    // check if the node belongs to an IDGroup
    if (root->m_sceneNodeMap.size() > 0) {
        auto node = root->m_sceneNodeMap[id];
        // if the node is part of an IDGroup, return the owner of the IdGroup
        return node->getExtIdGroup() ? node->getExtIdGroup()->owner : node;
    } else {
        return nullptr;
    }
}

void SceneNode::deleteGarbage() {
    for (const auto& it : m_sceneNodesToKill) delete it;

    m_sceneNodesToKill.clear();
}

void SceneNode::dumpTreeIt(SceneNode* tNode, uint level) {
    uint newLevel = level + 1;

    for (const auto& it : *tNode->getChildren()) {
        std::string tabs = "";
        for (uint i = 0; i < level - 1; i++) {
            tabs += "\t";
        }

        std::string nt;
        switch (it->m_nodeType) {
            case GLSG_SNT_STANDARD: nt = "GLSG_SNT_STANDARD"; break;
            case GLSG_SNT_LIGHT: nt = "GLSG_SNT_LIGHT"; break;
            case GLSG_SNT_LIGHT_SCENE_MESH: nt = "GLSG_SNT_LIGHT_SCENE_MESH"; break;
            case GLSG_GIZMO: nt = "GLSG_GIZMO"; break;
            default: nt = ""; break;
        }

        LOG << tabs << "[" << it->getName() << "], nodeType: " << nt << " " << it;
        tabs += "\t";

        LOG << tabs << "  visible: " << it->m_visible;
        LOG << tabs << "  selectable: " << it->m_selectable;
        LOG << tabs << "  hasNewModelMat: " << it->m_hasNewModelMat;
        LOG << tabs << "  trans: " << glm::to_string(*it->getTransVec());
        LOG << tabs << "  scale: " << glm::to_string(*it->getScalingVec());
        LOG << tabs << "  rotation: angle: " << it->getRotAngle() << " axis: " << glm::to_string(*it->getRotAxis());
        LOG << tabs << "  dimension: " << glm::to_string(*it->getDimension());

        LOG << tabs << "  objIds: ";
        for (const auto& pit : it->m_nodeObjId)
            LOG << tabs << "    [" << pit.first << "](\"" << (pit.first ? pit.first->getName() : "")
                << "\"): " << pit.second;

        if (it->getIdGroup()) {
            LOG << tabs << "  IdGroup [" << it->getIdGroup()->owner->getName() << "]:";
            for (const auto& g : it->getIdGroup()->ids)
                LOG << tabs << " \t[" << g.first->getName() << ", " << g.second << "]";
        }

        if (it->getExtIdGroup()) {
            LOG << tabs << "  extIdGroup [" << it->getExtIdGroup()->owner->getName() << "]:";
            for (const auto& g : it->getExtIdGroup()->ids) {
                LOG << tabs << " \t[" << g.first->getName() << ", " << g.second << "]";
            }
        }

        LOG << tabs << "  parents: ";
        uint j = 0;
        for (const auto& pit : *it->getParents()) {
            LOG << tabs << "    [" << j << "]: " << pit << " \"" << pit->getName() << "\"";
            ++j;
        }

        LOG << tabs << "  selected: ";
        for (const auto& pit : it->m_selected)
            LOG << tabs << "    [" << pit.first << "](\"" << (pit.first ? pit.first->getName() : "")
                << "\"): " << pit.second;

        dumpTreeIt(it, newLevel);
    }
}

void SceneNode::dump() {
    LOG << "\n####################################";
    LOG << "SceneNode[" << m_name << "] DUMP: ";
    dumpTreeIt(this, 1);
    LOG << "####################################\n";
}

void SceneNode::unregister() {
    // if there are still any callback registered in the scene, delete them now
    if (m_scene) {
        // m_scene->eraseGlCb(this);
    }

    // call the removeCBs, which will release the parent-child relation(s)
    if (!s_removeCb.empty())
        for (const auto& it : s_removeCb)
            for (const auto& f : it.second) f.second();

    // check the whole tree for nodes which do have this node as a reference
    // either in "parents" or in "objIds" also delete removeCB which have this
    // node as name
    if (m_name != "root") {
        getRootNode();

        if (m_rootNode) {
            iterateNode(m_rootNode, [this](SceneNode* node) {
                // check objIds
                if (node && !node->m_nodeObjId.empty()) {
                    std::erase_if(node->m_nodeObjId, [this](const pair<SceneNode*, int> mapItem) { return mapItem.first == this; });
                }

                // check parent references
                if (node && !node->m_parents.empty()) {
                    std::erase_if(node->m_parents, [this](const SceneNode* sn) { return sn == this; });
                }

                // check "selected" references
                if (node && !node->m_selected.empty()) {
                    std::erase_if(node->m_selected, [this](const pair<SceneNode*, bool> mapItem) { return mapItem.first == this; });
                }

                return true;
            });
        }

        // unregister the node from its parents
        if (!m_parents.empty())
            for (const auto& it : m_parents) {
                if (it->m_children.size()) {
                    auto childIt = std::find(it->m_children.begin(), it->m_children.end(), this);
                    if (childIt != it->m_children.end()) it->m_children.erase(childIt);
                }
            }
    }
}

}  // namespace ara
