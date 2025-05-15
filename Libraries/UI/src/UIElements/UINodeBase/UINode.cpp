//
// Created by user on 5/5/25.
//


#include <WindowManagement/WindowManager.h>
#include <UIElements/UINodeBase/UINode.h>
#include <UISharedRes.h>
#include <UIApplication.h>
#include <GLBase.h>

using namespace std;
using namespace glm;

namespace ara {

UINode::UINode() {
    setName(getTypeName<UINode>());
}

UINode* UINode::getNode(const std::string& name) {
    UINode* fn = nullptr;
    getNodeIt(this, &fn, name);
    return fn;
}

UINode* UINode::addChild(std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::addChild failed, child empty!";
        return nullptr;
    }
    m_children.emplace_back(std::move(child));
    reqTreeChanged(true);
    initChild(m_children.back().get(), this);
    return m_children.back().get();
}

UINode* UINode::insertChild(int position, std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::insertChild failed, child empty!";
        return nullptr;
    }
    auto it = m_children.insert(m_children.begin() + position, std::move(child));
    reqTreeChanged(true);
    initChild(it->get(), this);
    return it->get();
}

UINode* UINode::insertAfter(const std::string& name, std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::insertAfter failed, child empty!";
        return nullptr;
    }

    auto r = ranges::find_if(m_children, [&name](auto& it){
        return name == it->getName();
    });

    if (r != m_children.end()
        && static_cast<int>(std::distance(m_children.begin(), r) +1) <= static_cast<int>(m_children.size())) {
        const auto it = m_children.insert(std::next(r, 1), std::move(child));
        m_reqTreeChanged = true;
        initChild(it->get(), this);
        return it->get();
    } else {
        return nullptr;
    }
}

UINode* UINode::insertChild(const std::string& name, std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::insertAfter failed, child empty!";
        return nullptr;
    }

    auto r = ranges::find_if(m_children, [&name](auto& it){
        return name == it->getName();
    });

    if (r != m_children.end()) {
        const auto it = m_children.insert(r, std::move(child));
        m_reqTreeChanged = true;
        initChild(it->get(), this);
        return it->get();
    } else {
        return nullptr;
    }
}

void UINode::moveChildTo(int position, UINode* node) {
    if (!node->getParent()) {
        return;
    }

    auto nodeIt = ranges::find_if(node->getParent()->getChildren(),
                                  [node](const auto& it) { return node == it.get(); });
    if (nodeIt == node->getParent()->getChildren().end()) {
        return;
    }

    m_children.insert(m_children.begin() + position, make_move_iterator(node->getParent()->getChildren().begin()),
                      make_move_iterator(node->getParent()->getChildren().begin() + 1));

    node->getParent()->getChildren().erase(nodeIt, nodeIt + 1);

    m_reqTreeChanged = true;
    (*nodeIt)->setParent(this);
}

void UINode::remove_child(UINode* node) {
    // be sure the node to delete is really a child of this node
    auto it = ranges::find_if(m_children,
                              [node](const auto& n) { return n.get() == node; });

    if (it != m_children.end()) {
        (*it)->removeFocus();
        m_children.erase(it);
    }

    m_reqTreeChanged = true;
}

void UINode::clearChildren() {
    removeFocus();
    m_reqTreeChanged = true;
    m_children.clear();
}

void UINode::initChild(UINode* child, UINode* parent) {
    // check if viewport is initialised
    if (isViewportValid()) {
        child->setViewport(getViewport());
    }

    child->setSharedRes(parent->getSharedRes());
    child->setParent(parent);
}

uint32_t UINode::getMinChildId(uint32_t minId) {
    auto nextMinId = m_objIdMin ? std::min(m_objIdMin, minId) : minId;

    for (const auto& child : m_children) {
        if (child->getMinId() < nextMinId) {
            nextMinId = child->getMinId();
        }
        nextMinId = child->getMinChildId(nextMinId);
    }

    return nextMinId;
}

uint32_t UINode::getMaxChildId(uint32_t maxId) {
    auto nextMaxId = std::max(m_objIdMax, maxId);

    for (const auto& child : m_children) {
        if (child->getMaxId() > nextMaxId) {
            nextMaxId = child->getMaxId();
        }
        nextMaxId = child->getMaxChildId(nextMaxId);
    }

    return nextMaxId;
}

UINode* UINode::getNodeById(uint32_t searchID) {
    for (const auto& child : m_children) {
        if (child->getId() == searchID) {
            return child.get();
        }
        if (auto cN = child->getNodeById(searchID)) {
            return cN;
        }
    }
    return nullptr;
}

uint32_t UINode::getSubNodeCount() {
    uint32_t count = 0;
    getSubNodeCountIt(this, &count);
    return count;
}

void UINode::getSubNodeCountIt(UINode* node, uint32_t* count) {
    for (const auto& it : node->m_children) {
        ++(*count);
        getSubNodeCountIt(it.get(), count);
    }
}

bool UINode::getNodeIt(UINode* node, UINode** fn, const std::string& name) {
    if (node->m_name == name) {
        *fn = node;
        return true;
    } else {
        return ranges::any_of(node->m_children, [&](auto& it){ return getNodeIt(it.get(), fn, name); });
    }
}

UINode* UINode::getRoot() {
    UINode* out = this;
    while (out->getParent()) {
        out = out->getParent();
    }
    return out;
}

bool UINode::isInBounds(glm::vec2& pos) {
    if (!m_visible || m_referenceDrawing) {
        return false;
    }

    return UINodeGeom::isInBounds(pos);
}

// node argument refers to a child node
bool UINode::recChildrenBoundBox(glm::vec4& ref) {
    if (!m_visible || m_excludeFromParentContentTrans) {
        return false;
    }

    m_bb.x = !m_children.empty() ? m_pos.x + m_childBoundBox.x : m_pos.x;
    m_bb.y = !m_children.empty() ? m_pos.y + m_childBoundBox.y : m_pos.y;
    m_bb.z = !m_children.empty() ? m_pos.x + std::max<float>(m_size.x, m_childBoundBox.z) : m_pos.x + m_size.x;
    m_bb.w = !m_children.empty() ? m_pos.y + std::max<float>(m_size.y, m_childBoundBox.w) : m_pos.y + m_size.y;

    ref.x = m_bb.x < ref.x ? m_bb.x : ref.x;
    ref.y = m_bb.y < ref.y ? m_bb.y : ref.y;
    ref.z = m_bb.z > ref.z ? m_bb.z : ref.z;
    ref.w = m_bb.w > ref.w ? m_bb.w : ref.w;

    return true;
}

void UINode::drawAsRoot(uint32_t& objId) {
#ifdef ARA_DEBUG
    postGLError();
#endif
    m_scissorStack.stack.clear();
    m_scissorStack.stack.emplace_back(0, 0, m_viewPort.z * getPixRatio(), m_viewPort.w * getPixRatio());

    // first update all matrices and calculate the children's bounding boxes
    updtMatrIt(&m_scissorStack);

    // scissor stack must be in hardware pixel coordinates, 0|0 is bottom left
    glScissor(0, 0, static_cast<GLsizei>(m_viewPort.z * getPixRatio()), static_cast<GLsizei>(m_viewPort.w * getPixRatio()));

    if (m_reqTreeChanged) {
        m_treeChanged    = true;
        m_reqTreeChanged = false;
    }

    if (m_treeChanged && m_drawMan) {
        m_drawMan->clear();
        m_drawMan->addSet();
    }

    // iterate again through the ui-graph and draw
    bool skip = true;
    drawIt(m_scissorStack, objId, m_treeChanged, skip);

    if (m_treeChanged) {
        m_drawMan->rebuildVaos();
        m_treeChanged = false;
    } else {
        m_drawMan->update();
    }
}

void UINode::drawIt(scissorStack& ss, uint32_t& objId, bool treeChanged, bool& skip) {
    // draw to the screen and the object map
    if (!m_visible || !m_inited || m_isOutOfParentBounds || m_referenceDrawing) {
        return;
    }

    if (!skip) {
        if (!glm::all(glm::equal(m_sc, ss.active))) {
            ss.active = m_sc;
            if (m_drawImmediate) {
                glScissor(static_cast<GLint>(m_sc.x), static_cast<GLint>(m_sc.y), static_cast<GLint>(m_sc.z), static_cast<GLint>(m_sc.w));
            }
        }

        if (m_objIdMin != objId && (m_drawImmediate || (!m_drawImmediate && treeChanged))) {
            m_objIdMin = m_objIdMax = objId;
            m_drawParamChanged      = true;
        }

        // matrices may be changed within a draw loop, so be sure everything is up to date
        if (m_inited && m_geoChanged) {
            updateMatrix();
        }

        // if the node is being drawn indirectly, check if the DivData needs to be updated
        if (m_drawParamChanged) {
            updateDrawData();

            // if drawing indirectly with an unchanged tree, update the DrawSet at the position of this specific UINode
            if (!m_drawImmediate && !treeChanged) {
                pushVaoUpdtOffsets();
            }

            m_drawParamChanged = false;
        }

        if ((m_drawImmediate && draw(objId))
            || (!m_drawImmediate && treeChanged && drawIndirect(objId))) {
            ++objId;  // increase objId
        }

    } else {
        skip = false;
    }

    for (const auto& it : m_children) {
        it->drawIt(ss, objId, treeChanged, skip);
    }

    // Note: although the recursive call to drawIt is not the last call, difference in performance is not relevant
}

void UINode::updtMatrIt(scissorStack* ss) {
    if (!m_visible && !m_forceInit) {
        return;
    }

    checkInit();
    checkStyles();
    immediateScissoring(ss);
    updateMatrix();  // calculate the nodes' matrices. this must be done for drawing and child bounding box calculation
    checkScissoring(ss);

    // if we are drawing in indirect mode and an UINode has switched from out-of-bounds to in-bounds or visa-versa,
    // the indirect drawing tree has to be updated
    m_oob = isOutOfParentBounds();
    if (m_isOutOfParentBounds != m_oob) {
        reqUpdtTree();
    }
    m_isOutOfParentBounds = m_oob;

    // calculate a bounding box around all children, in node relative coordinates
    if (m_children.empty()) {
        memset(&m_childBoundBox[0], 0, sizeof(float) * 4);
    } else {
        m_childBoundBox.x = std::numeric_limits<float>::max();
        m_childBoundBox.y = std::numeric_limits<float>::max();
        m_childBoundBox.z = std::numeric_limits<float>::min();
        m_childBoundBox.w = std::numeric_limits<float>::min();
    }

    // continue iterating through the children
    for (const auto& it : m_children) {
        it->updtMatrIt(ss);
        // keep track of the children's boundingBox
        if (it->isVisible()) {
            it->recChildrenBoundBox(m_childBoundBox);
        }
    }

    // if the node was pushing a viewport to the scissor-stack, remove it here again
    if (ss && m_ScissorChildren && !ss->stack.empty()) {
        ss->stack.pop_back();
    }
}

void UINode::checkInit() {
    // make sure the node is inited
    if (!m_inited) {
        if (m_parent) {
            // check if there is a correct viewport in the child, if this is not the case, try to set it
            if (m_parent->isViewportValid()) {
                setViewport(m_parent->getViewport());
            }

            // in case a UINode has been added in a constructor but needs the sharedRes, pass it down from the parent here
            if (!getSharedRes() && m_parent->getSharedRes()) {
                setSharedRes(m_parent->getSharedRes());
            }
        }

        // be sure init always done in default state
        setState(state::none);  // sync states

        init();  // the Node may create children in its init function, which might depend on another node's size
        // so to be sure the calculation won't turn out wrong, since the
        // depending on nodes might change after init set the node to "changed" again

        rebuildCustomStyle();

        // restore selected state, if set before init. Also needed to sync states if set like this in UINode derivative
        if (m_lastState != state::none) {
            if (m_lastState == state::selected) {
                setSelected(true);  // sync states
            } else {
                m_state = m_lastState;
            }
        }
        m_inited = true;
    }
}

void UINode::checkStyles() {
    if (m_inited && getWindow() && (getWindow()->resChanged() || m_styleChanged || !m_styleClassInited)) {
        updateStyle();
        m_drawParamChanged = true;

        // in case the window was resize before the res.txt was changed, the styledefaults loaded will still in respect
        // of the initial window size, so if this is called from a res update "rescale" the window
        // TODO: default styles should be updated on resize
        if (m_sharedRes && m_sharedRes->win) {
            auto win = static_cast<UIWindow*>(m_sharedRes->win);
            if (win->resChanged()) {
                win->window_size_callback(win->getSize().x, win->getSize().y);
                m_sharedRes->requestRedraw = true;
            }
        }
    }
}

void UINode::immediateScissoring(scissorStack* ss) {
    if (ss && !ss->stack.empty() && m_parent) {
        // if the child is excluded from the parents scissoring, go one more step back in the scissor stack
        m_sc = *(ss->stack.end() - (m_excludeFromParentScissoring && m_parent->getScissorChildren() ? 2 : 1));

        if (!m_drawImmediate) {
            m_scIndDraw = m_sc;
            m_scIndDraw /= getPixRatio();  // convert to virtual pixels
            m_scIndDraw.y = m_viewPort[3] - m_scIndDraw.y - m_scIndDraw.w;  // glScissor Origin is bottom left, change to top, left
        }
    }
}

void UINode::checkScissoring(scissorStack* ss) {
    // push a viewport to the scissor stack if the Node has the scissorchildren flag set std::round() because straight
    // forward casting to int will produce wrong results...
    if (ss && m_ScissorChildren && m_sc.z != 0.f && m_sc.w != 0.f) {
        // here we get the top left corner, but glScissor needs lower left corner scissor area can't exceed actual scissor bounds
        auto t_nodeViewPortGL = getNodeViewportGL();

        m_scissVp.x = std::max(std::floor(std::round(t_nodeViewPortGL.x + static_cast<float>(getBorderWidth()))) * getPixRatio(), m_sc.x);
        m_scissVp.y = std::max(std::floor(std::round(t_nodeViewPortGL.y + static_cast<float>(getBorderWidth()))) * getPixRatio(), m_sc.y);
        m_scissVp.z = std::floor(std::round(t_nodeViewPortGL.z - getBorderWidth() * 2)) * getPixRatio();
        m_scissVp.w = std::floor(std::round(t_nodeViewPortGL.w - getBorderWidth() * 2)) * getPixRatio();

        ss->stack.emplace_back(
                m_scissVp.x, m_scissVp.y,
                m_scissVp.x + m_scissVp.z > m_sc.z ? std::min(m_scissVp.x + m_scissVp.z, m_sc.x + m_sc.z) - m_scissVp.x
                                                   : m_scissVp.z,
                m_scissVp.y + m_scissVp.w > m_sc.w ? std::min(m_scissVp.y + m_scissVp.w, m_sc.y + m_sc.w) - m_scissVp.y
                                                   : m_scissVp.w);
    }
}

void UINode::updateMatrix() {
    if (!m_geoChanged || m_updating) {
        return;
    }

    m_updating     = true;  // avoid infinite recursive call
    m_parentContVp = m_viewPort;

    getParentViewport(); // in the case of the root node, this will stay the window's viewport
    setPositionAndSize();
    setAlignment();

    // store the size of this node (including its padding and border)
    m_size = m_work_size;

    // parent relative content matrix for the node's children
    m_nodeMat[3][0] = m_pos.x + static_cast<float>(m_borderWidth);
    m_nodeMat[3][1] = m_pos.y + static_cast<float>(m_borderWidth);

    m_contentMat[3][0] = m_nodeMat[3][0] + m_padding.x;
    m_contentMat[3][1] = m_nodeMat[3][1] + m_padding.y;

    // calculate the content transformation matrix
    calcContentTransMat();

    m_geoChanged = false;
    m_updating   = false;

    if (!m_drawImmediate) {
        m_drawParamChanged = true;
    }

    if (m_changeCb) {
        m_changeCb();
    }
}

void UINode::getParentViewport() {
    if (m_parent) {
        // create a local copy of the parent's matrix
        m_parentMat       = m_parent->getFlatContentMat(m_excludeFromParentContentTrans, m_excludeFromPadding);
        m_parentMatLocCpy = *m_parentMat;

        m_parentContScale.x = (*m_parentMat)[0][0];
        m_parentContScale.y = (*m_parentMat)[1][1];

        if (m_excludeFromPadding) {
            // TODO: check if the parent's content translation, is respected here
            m_parentContVp = m_parent->getNodeViewport();
        } else {
            m_parentContVp.x = (*m_parentMat)[3][0];
            m_parentContVp.y = (*m_parentMat)[3][1];
            m_parentContVp.z = m_parent->getContentSize().x;
            m_parentContVp.w = m_parent->getContentSize().y;
        }

        m_absoluteAlpha = m_parent->getAlpha() * m_alpha;
    }
}

void UINode::setPositionAndSize() {
    m_pos.x       = static_cast<float>(m_posXInt);
    m_pos.y       = static_cast<float>(m_posYInt);
    m_init_size.x = static_cast<float>(m_widthInt);
    m_init_size.y = static_cast<float>(m_heightInt);

    // if there is a negative integer width or height, convert it to size - val
    if (m_widthType == unitType::Pixels && m_widthInt < 0) {
        m_init_size.x = m_parentContVp.z + static_cast<float>(m_widthInt);
    }

    if (m_heightType == unitType::Pixels && m_heightInt < 0) {
        m_init_size.y = m_parentContVp.w + static_cast<float>(m_heightInt);
    }

    // if there are relative position or size coordinates, convert them to
    // pixels
    if (m_posXType == unitType::Percent) {
        m_pos.x = m_posXFloat * m_parentContVp.z;
    }
    if (m_posYType == unitType::Percent) {
        m_pos.y = m_posYFloat * m_parentContVp.w;
    }
    if (m_widthType == unitType::Percent) {
        m_init_size.x = m_widthFloat * m_parentContVp.z;
    }
    if (m_heightType == unitType::Percent) {
        m_init_size.y = m_heightFloat * m_parentContVp.w;
    }

    // get the node's aspect ratio
    m_aspect = m_init_size.x / m_init_size.y;

    m_work_size.x = m_init_size.x;
    m_work_size.y = m_init_size.y;

    // if there is a request aspect process it
    if (m_fixAspect > 0.f) {
        // correct size, always shrink
        if (m_fixAspect < m_aspect) {
            m_work_size.x = m_init_size.y * m_fixAspect;
            m_work_size.y = m_init_size.y;
        } else {
            m_work_size.x = m_init_size.x;
            m_work_size.y = m_init_size.x / m_fixAspect;
        }
    }
}

void UINode::setAlignment() {
    // process x pivot and alignment
    switch (m_alignX) {
        case align::right: m_pos.x += m_parentContVp.z; break;
        case align::center: m_pos.x += m_parentContVp.z * 0.5f; break;
        default: break;
    }

    switch (m_pivX) {
        case pivotX::left: break;
        case pivotX::right: m_pos.x -= m_work_size.x; break;
        case pivotX::center: m_pos.x -= m_work_size.x * 0.5f; break;
    }

    // process x pivot and alignment
    switch (m_alignY) {
        case valign::bottom: m_pos.y += m_parentContVp.w; break;
        case valign::center: m_pos.y += m_parentContVp.w * 0.5f; break;
        default: break;
    }

    switch (m_pivY) {
        case pivotY::top: break;
        case pivotY::bottom: m_pos.y -= m_work_size.y; break;
        case pivotY::center: m_pos.y -= m_work_size.y * 0.5f; break;
    }
}

bool UINode::objPosIt(ObjPosIt& opi) {
    bool inBounds = (*opi.it)->isInBounds(opi.pos);

    // optional callback for additional ui elements there are not part of the regular tree
    if ((*opi.it)->m_outOfTreeObjId) {
        bool foundOot = false;
        foundOot = (*opi.it)->m_outOfTreeObjId(opi);
        if (foundOot) {
            return false;
        }
    }

    if (inBounds && !(*opi.it)->isExcludedFromObjMap() && (*opi.it)->isVisible() && opi.foundTreeLevel < opi.treeLevel) {
            opi.foundNode      = opi.it->get();
        opi.foundId        = (*opi.it)->getId();
        opi.foundTreeLevel = opi.treeLevel;
        // call hid interaction method on node, can optionally stop iteration
    }

    // if within bounds, not excluded from objMap and no more children -> found it!!
    if (inBounds && !(*opi.it)->isExcludedFromObjMap() && (*opi.it)->isVisible() && (*opi.it)->m_children.empty()) {
        return false;
    } else if (inBounds && !(*opi.it)->isExcludedFromObjMap() && (*opi.it)->isVisible() && !(*opi.it)->m_children.empty()) {
        // if within bounds and more children -> step down
        opi.parents.emplace_back(opi.it);
        opi.list = &(*opi.it)->m_children;
        opi.it   = (*opi.it)->m_children.end() - 1;
        ++opi.treeLevel;
    } else {
        // try to step towards front
        if (opi.it != opi.list->begin()) {
            --opi.it;
        } else {
            // if this is the front element, try to go one level up
            if (opi.parents.empty() || !(*opi.parents.back())->getParent()) {
                return false;
            }

            opi.list = &(*opi.parents.back())->getParent()->m_children;
            opi.it   = opi.parents.back();
            opi.parents.pop_back();
            --opi.treeLevel;

            // try to step towards front
            if (opi.it == opi.list->begin()) {
                return false;
            }

            --opi.it;
        }
    }
    return objPosIt(opi);
}

void UINode::reqUpdtTree() const {
    if (!m_drawImmediate && m_sharedRes && m_sharedRes->win) {
        const auto win = static_cast<UIWindow*>(m_sharedRes->win);
        if (win->getRootNode() && win->getRootNode()->getRoot()) {
            win->getRootNode()->getRoot()->reqTreeChanged(true);
        }
    }
}

void UINode::limitDrawVaoToBounds(const vector<DivVaoData>::iterator& dIt, vec2& size, vec2& uvDiff, vec4& scIndDraw, vec4& vp) {
    dIt->pos.y *= -1.f;

    bool limit = glm::compAdd(scIndDraw) > 0.f;
    for (int i = 0; i < 2; i++) {
        // convert to pixels and limit to NDC bounds
        dIt->pos[i] = (dIt->pos[i] * 0.5f + 0.5f) * vp[2 + i];
        uvDiff[i] = dIt->pos[i];  // save initial pixel position for later uv correction

        // limit same as glScissor TODO: this causes problems with border-radius, review
        if (limit) {
            dIt->pos[i] = std::min(std::max(scIndDraw[i], dIt->pos[i]), scIndDraw[i] + scIndDraw[i + 2]);
        }

        // calculate relative change for offsetting texture coordinates accordingly
        uvDiff[i] = std::abs(uvDiff[i] - dIt->pos[i]) / size[i];

        // convert back to normalized coordinates
        dIt->pos[i] = (dIt->pos[i] / vp[2 + i]) * 2.f - 1.f;
    }

    dIt->pos.y *= -1.f;
}

void UINode::limitTexCoordsToBounds(float* tc, int stdQuadVertInd, const vec2& tvSize, const glm::vec2& uvDiff) {
    // limit left side of texture view
    if (stdQuadVertInd == 0 || stdQuadVertInd == 2) {
        tc[0] += tvSize.x * uvDiff.x;
    }

    // limit right side of texture view
    if (stdQuadVertInd == 1 || stdQuadVertInd == 3) {
        tc[0] -= tvSize.x * uvDiff.x;
    }

    // limit bottom side of texture view
    if (stdQuadVertInd == 0 || stdQuadVertInd == 1) {
        tc[1] += tvSize.y * uvDiff.y;
    }

    // limit top side of texture view
    if (stdQuadVertInd == 2 || stdQuadVertInd == 3) {
        tc[1] -= tvSize.y * uvDiff.y;
    }
}

void UINode::runOnMainThread(const std::function<bool()>& func, bool forcePush) const {
    if (m_glbase) {
        m_glbase->runOnMainThread(func, forcePush);
    }
}

void UINode::addGlCbSync(const std::function<bool()>& func) const {
    if (m_glbase) {
        m_glbase->addGlCbSync(func);
    }
}

void UINode::eraseCb(const std::string& identifier) {
    static_cast<UIWindow *>(m_sharedRes->win)->eraseGlCb(this, identifier);
}

bool UINode::hasCb(const std::string& identifier) {
    return static_cast<UIWindow *>(m_sharedRes->win)->hasCb(this, identifier);
}

void UINode::setDrawFlag() const {
    if (m_sharedRes) {
        m_sharedRes->setDrawFlag();
    }
}

void UINode::setChanged(bool val) {
    m_geoChanged = val;
    for (const auto& it : m_children) {
        it->setChanged(val);
    }
}

void UINode::setViewport(float x, float y, float width, float height) {
    UINodeGeom::setViewport(x, y, width, height);
    for (const auto& it : m_children) {
        it->setViewport(x, y, width, height);
    }
}

void UINode::setVisibility(bool val, state st) {
    UINodeStyle::setVisibility(val, st);
    if (st == state::m_state || st == m_state) {
        // object ids won't be updated when not visible - set them all to zero immediately
        if (!val) {
            itrNodes(this, [](UINode* nd) {
                nd->setId(0);
            });
        }

        if (!m_drawImmediate) {
            reqUpdtTree();
        }
    }
}

void UINode::applyStyle() {
    UINodeStyle::applyStyle();
    if (!m_excludeFromStyles) {
        if (m_uniBlock.isInited()) {
            m_uniBlock.update();
        }

        if (!m_setStyleFunc[m_state].empty()) {
            setDrawFlag();
        }
    }
}

void UINode::setSharedRes(UISharedRes* shared) {
    UINodeGeom::setSharedRes(shared);
    if (shared) {
        m_shCol     = shared->shCol;
        m_objSel    = shared->objSel;
        m_quad      = shared->quad;
        m_shdr      = m_shCol->getUIColBorder();
        m_glbase    = shared->glbase;
        m_drawMan   = shared->drawMan;
    }
    setChangeCb([this] { onResize(); });
}

std::filesystem::path UINode::dataPath() {
    return m_sharedRes ? m_sharedRes->dataPath : std::filesystem::current_path();
}

WindowManager*  UINode::getWinMan() {
    return m_glbase->getWinMan();
}

UIApplication* UINode::getApp() const {
    return (m_sharedRes && m_sharedRes->win) ? static_cast<UIWindow*>(m_sharedRes->win)->getApplicationHandle()
                                             : nullptr;
}

void UINode::addGlCb(const std::string& identifier, const std::function<bool()>& f) {
    if (m_sharedRes) {
        static_cast<UIWindow *>(m_sharedRes->win)->addGlCb(this, identifier, f);
    }
}

void UINode::keyDownIt(hidData& data) {
    UINodeHID::keyDownIt(data);

    for (const auto& it : m_children) {
        it->keyDownIt(data);
    }
}

void UINode::onCharIt(hidData& data) {
    UINodeHID::onCharIt(data);

    for (const auto& it : m_children) {
        it->onCharIt(data);
    }
}

bool UINode::removeFocus() {
    UINodeHID::removeFocus();
    for (const auto& it : m_children) {
        it->removeFocus();
    }
    return false;
}

void UINode::setHIDBlocked(bool val) {
    UINodeHID::setHIDBlocked(val);
    for (const auto& it : m_children) {
        if (it) {
            dynamic_cast<UINodeHID*>(it.get())->setHIDBlocked(val);
        }
    }
}

void UINode::dump() {
    LOG << "----------- UINode Tree: -----------";
    int depth = 0;
    dumpIt(this, &depth, true);
}

void UINode::dumpIt(UINode* node, int* depth, bool dumpLocalTree) {
    string pr;
    string state;

    switch (node->getState()) {
        case state::none: state = "none"; break;
        case state::selected: state = "selected"; break;
        case state::disabled: state = "disabled"; break;
        case state::disabledSelected: state = "disabledSelected"; break;
        case state::disabledHighlighted: state = "disabledHighlighted"; break;
        case state::m_state: state = "m_state"; break;
        case state::highlighted: state = "highlighted"; break;
        default: break;
    }

    for (auto i = 0; i < *depth; i++) pr += "\t";
    LOG << pr << "[" << node->m_name.c_str() << "], \tvisible: " << node->isVisible() << ", \tstate: " << state
        << (node->isVisible()
            ? " \tid: [ min: " + std::to_string(node->getMinId()) + " max: " + std::to_string(node->getMaxId()) +
              (node->isExcludedFromObjMap() ? " EXCLUDED"
                                            : !node->isVisible()         ? " INVISIBLE"
                                                                         : "") +
              " ]"
            : "")
        << ", \tpos:" << glm::to_string(node->m_pos) << ", \tsize:" << glm::to_string(node->m_size);

    ++(*depth);
    for (const auto& it : node->m_children) {
        dumpIt(it.get(), depth, dumpLocalTree);
    }
    --(*depth);
}

UINode::~UINode() {
    // check if the actual is selected as inputFocusNode, if this is the case, set it to nullptr
    if (m_sharedRes) {
        if (!m_drawImmediate && m_drawMan) {
            m_drawMan->removeUINode(this);
        }

        if (m_sharedRes->win) {
            auto win = static_cast<UIWindow*>(m_sharedRes->win);
            win->removeGlCbs(this);  // remove all callbacks issued by this Node
            win->onNodeRemove(this);
        }
    }
}

void UINode::util_FillRect(ivec2 pos, ivec2 size, vec4 col, Shaders* shdr, Quad* quad) {
    if (shdr == nullptr) {
        shdr = m_shdr;
    }

    if (quad == nullptr) {
        quad = m_quad;
    }

    if (shdr == nullptr || quad == nullptr || size.x <= 0 || size.y <= 0) {
        return;
    }

    auto m = *m_orthoMat * m_parentMatLocCpy * translate(vec3(static_cast<float>(pos.x) + getPos().x, static_cast<float>(pos.y) + getPos().y, 0.f));

    shdr->begin();
    shdr->setUniformMatrix4fv("m_pvm", &m[0][0]);
    shdr->setUniform2f("size", static_cast<float>(size.x), static_cast<float>(size.y));
    shdr->setUniform4f("color", col.r, col.g, col.b, col.a);
    shdr->setUniform2f("borderWidth", 0.f, 0.f);
    shdr->setUniform2f("borderRadius", 0.f, 0.f);

    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void UINode::util_FillRect(ivec2 pos, ivec2 size, float* color, Shaders* shdr, Quad* quad) {
    return util_FillRect(pos, size, {color[0], color[1], color[2], color[3]}, shdr, quad);
}

void UINode::util_FillRect(glm::ivec4& r, float* color, Shaders* shdr, Quad* quad) {
    return util_FillRect({r.x, r.y}, {r.z, r.w}, {color[0], color[1], color[2], color[3]}, shdr, quad);
}

}