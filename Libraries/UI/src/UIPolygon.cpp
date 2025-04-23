//
// Created by user on 02.07.2020.
//

#include "UIPolygon.h"

#include <GeoPrimitives/Quad.h>

using namespace glm;
using namespace std;

namespace ara {
UIPolygon::UIPolygon() : UINode(), m_ctrlPointSizePix(17) {
    setName(getTypeName<UIPolygon>());
    setFocusAllowed(false);
    m_canReceiveDrag = true;

    m_lColor      = vector<vec4>(4);
    m_lColor[0]   = vec4(0.0f, 0.333f, 0.47f, 1.f) * 0.8f;
    m_lColor[0].a = 1.f;
    m_lColor[1]   = vec4(0.0f, 0.333f, 0.47f, 1.f) * 1.2f;
    m_lColor[2]   = vec4(1.f, 0.82f, 0.f, 1.f) * 0.8f;
    m_lColor[3]   = vec4(1.f, 0.82f, 0.f, 1.f);

    m_ident = glm::mat4(1.f);

    setChangeCb(std::bind(&UIPolygon::onResize, this));
}

void UIPolygon::init() {
    m_fbo       = make_unique<FBO>(m_glbase, 512, 512, 1);
    m_uiTexShdr = m_shCol->getUITex();

    // create a test polygon
    m_polygon = make_unique<Polygon>(m_shCol);
    m_polygon->addPoint(0, -0.7f, 0.7f);
    m_polygon->addPoint(0, -0.7f, -0.7f);
    m_polygon->addPoint(0, 0.7f, -0.7f);
    m_polygon->addPoint(0, 0.7f, 0.7f);

    initCtrlPointShdr();
    tesselate();
}

bool UIPolygon::draw(uint32_t *objId) {
    m_fbo->bind();

    if (!m_drawInv) {
        m_fbo->clear();
        m_shdr->setUniform4fv("color", &m_color[0]);
    } else {
        m_fbo->clearToColor(0.f, 0.f, 0.f, 1.f);
        glBlendFunc(GL_ONE, GL_ZERO);
        m_shdr->setUniform4f("color", 0.f, 0.f, 0.f, 0.f);
    }

    // draw background
    m_shdr->setUniformMatrix4fv("m_pvm", getMVPMatPtr());
    m_shdr->setUniform2fv("size", &getSize()[0]);
    m_polygon->draw();

    m_fbo->unbind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_uiTexShdr->begin();
    m_uiTexShdr->setUniformMatrix4fv("m_pvm", getMVPMatPtr());
    m_uiTexShdr->setUniform2fv("size", &getSize()[0]);
    m_uiTexShdr->setUniform1i("hFlip", 1);

    m_objIdMin = (*objId);
    m_uiTexShdr->setUniform1f("objId", (float)(*objId)++);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo->getColorImg());
    m_quad->drawAsShared();

    drawCtrlPoints(objId);

    return false;  // don't count up objId
}

void UIPolygon::drawCtrlPoints(uint32_t *id) {
    m_ctrPointShdr->begin();

    // draw the warping reference points (Nodes)
    for (auto &it : *m_polygon->getPoints(0)) {
        glDisable(GL_BLEND);
        it.id = *id;

        vec2 refPointSize = (float)m_ctrlPointSizePix /
                            vec2(getParent()->getViewport()->z, getParent()->getViewport()->w) *
                            getContentTransScale2D();

        // Polygon Control Points
        mat4 refPointMat = *getWinRelMat() * translate(vec3(it.position.x, it.position.y, 0.f)) *
                           scale(vec3(refPointSize.x, refPointSize.y, 1.f));

        m_ctrPointShdr->setUniform4fv("color", &m_lColor[it.selected ? 3 : 2][0]);
        m_ctrPointShdr->setUniformMatrix4fv("m_pvm", &refPointMat[0][0]);
        m_ctrPointShdr->setUniform2f("size", 1.f, 1.f);
        m_ctrPointShdr->setUniform1i("round", 1);
        m_ctrPointShdr->setUniform1i("useTex", 0);
        m_ctrPointShdr->setUniform1f("objId", (float)(*id));

        m_quad->drawAsShared();

        glEnable(GL_BLEND);
        m_objIdMax = (*id);
        (*id)++;
    }
}

void UIPolygon::initCtrlPointShdr() {
#ifndef __EMSCRIPTEN__
    string vert = STRINGIFY(
        layout(location = 0) in vec4 position;\n layout(location = 2) in vec2 texCoord;\n uniform mat4 m_pvm;\n out vec2 tex_coord;\n void
            main() {
                \n tex_coord   = texCoord;
                \n gl_Position = m_pvm * position;
                \n
            });

    vert = "// control point shader, vert\n" + m_shCol->getShaderHeader() + vert;

    string frag = STRINGIFY(uniform vec4 color;\n
                                    uniform int round;\n
                                    in vec2 tex_coord;\n
                                    layout(location = 0) out vec4 glFragColor;\n
                                    void main() {
        \n glFragColor = (round == 0 ? 1 : min(max((1.0 - length(tex_coord * 2.0 - 1.0)) * 5.0, 0.0), 1.0)) * color;
    );

    frag = "// control point shader, frag\n" + m_shCol->getShaderHeader() + m_shCol->getUiObjMapUniforms() + frag +
           m_shCol->getUiObjMapMain() + "}";

#else
    string vert = STRINGIFY(precision mediump float; attribute vec4 position; attribute vec2 texCoord;
                            uniform mat4 m_pvm; varying vec2 tex_coord; void main() {
                                tex_coord   = texCoord;
                                gl_Position = m_pvm * position;
                            });

    string frag = STRINGIFY(
        precision mediump float; varying vec2 tex_coord; uniform vec4 color;\n uniform float bright;\n void main() {
            gl_FragColor = (round == 0 ? 1 : min(max((1.0 - length(tex_coord * 2.0 - 1.0)) * 5.0, 0.0), 1.0)) * color;
        });
#endif

    m_ctrPointShdr = m_shCol->add("UIPolygonPointShdr", vert, frag);
}

void UIPolygon::tesselate() {
        m_polygon->tesselate();
}

bool UIPolygon::objIdInPolygon(int id) {
        return id == m_objIdMin || ((uint32_t)id >= m_polygon->getPoints(0)->front().id &&
                                    (uint32_t)id <= m_polygon->getPoints(0)->back().id);
}

void UIPolygon::mouseDrag(hidData *data) {
        // get the relative offset in relation to the root node
        vec2 relMouseOffs = data->mousePosNormFlipY - m_mc_mousePos;

        // now inverse apply this nodes matrix
        relMouseOffs = relMouseOffs * 2.f / vec2((*getWinRelMat())[0][0], (*getWinRelMat())[1][1]);

        if (objIdInPolygon(data->objId) && !(data->altPressed && !data->shiftPressed)) {
            if (data->dragStart) {
                createCopyForUndo();
                data->dragStart = false;

                if (*data->cp_editM == cpEditMode::Move) {
                    addPointToSelection(data->objId, 1, data->procSteps);
                }
            } else {
                if (*data->cp_editM == cpEditMode::Move) {
                    // double check if the dragged point is part of the
                    // selection if the opengl thread is delayed the queue might
                    // not have been cleared if the queue will be cleared later
                    // the ui_vc->cpSelectQueue.size will be 0 and the objId
                    // check will be skipped
                    if (m_cpSelectQueue.size() != 0 && checkCtrlPointInSel(data->objId)) {
                        moveCtrlPoints(relMouseOffs, *data->cp_editM);
                        data->procSteps->at(Select).active = true;
                    } else {
                        // m_polygon->getPoints(0)->at(cpInd).dispPos =
                        // actMousePos[1]; data->procSteps->at(Select).active =
                        // true;
                    }

                    data->procSteps->at(Tesselate).active = true;
                }
            }
        }

        data->consumed = true;
}

// from node iteration
void UIPolygon::mouseDown(hidData *data) {
        m_mc_mousePos = data->mousePosFlipY;

        //  LOG << "UIPolygon clicked this id " << objId;

        if (data->objId >= (int)m_polygon->getPoints(0)->front().id &&
            data->objId <= (int)m_polygon->getPoints(0)->back().id) {
            if (*data->cp_editM != cpEditMode::AddCtrlPoint && *data->cp_editM != cpEditMode::DelCtrlPoint) {
                *data->cp_editM = cpEditMode::Move;

            } else if (*data->cp_editM == cpEditMode::DelCtrlPoint) {
                m_polygon->deletePoint(0, data->objId - (int)m_polygon->getPoints(0)->front().id);
                data->procSteps->at(winProcStep::Tesselate).active = true;
            }
        }

        data->consumed = true;
}

void UIPolygon::onResize() {
        if (m_fbo->getWidth() != getNodeSize().x || m_fbo->getHeight() != getNodeSize().y)
            m_fbo->resize((uint)getNodeSize().x, (uint)getNodeSize().y);
}

void UIPolygon::addPointToSelection(uint32_t clickedObjId, uint32_t level, map <winProcStep, ProcStep> *procSteps) {
        uint32_t cpInd = clickedObjId - m_polygon->getPoints(0)->at(0).id;

        // if there are elements in the selection queue
        // check if the actual dragged CtrlPoint is part of the
        // selection, if not clear the Selection as soon as possible
        if (m_cpSelectQueue.size() != 0 && !checkCtrlPointInSel(clickedObjId)) clearSelQueue();

        if (cpInd < m_polygon->getPoints(0)->size()) {
            m_polygon->getPoints(0)->at(cpInd).selected    = true;
            m_polygon->getPoints(0)->at(cpInd).refPosition = m_polygon->getPoints(0)->at(cpInd).position;
            m_cpSelectQueue[cpInd]                         = &m_polygon->getPoints(0)->at(cpInd);
        }
}

bool UIPolygon::checkCtrlPointInSel(uint32_t _id) {
        bool found = false;
        for (auto &kv : m_cpSelectQueue) {
            if (m_polygon->getPoints(0)->at(kv.first).id == _id) {
                found = true;
                break;
            }
        }
        return found;
}

void UIPolygon::clearSelQueue() {
        m_cpSelectQueue.clear();

        for (auto &it : *m_polygon->getPoints(0)) it.selected = false;
}

void UIPolygon::createCopyForUndo() {

}

void UIPolygon::moveCtrlPoints(vec2 _offset, cpEditMode cp_editM) {
        // movement with linear interpolation
        // check level 1
        for (auto &kv : m_cpSelectQueue) {
            kv.second->position = kv.second->refPosition + _offset;
        }
}

void UIPolygon::addPoint(vec2 pos, map <winProcStep, ProcStep> *procSteps) {
        vec2 mousePos = vec2(inverse(*getWinRelMat()) * vec4(pos, 0.f, 1.f));

        // run through all lines of the polygon and calculate the distance of
        // the mouse position to them (orthogonal line orthogonal must intersect
        // polygon line)
        map<float, int> sortedDist;
        auto            ply = m_polygon->getPoints(0);
        for (int i = 0; i < ply->size(); i++) {
            bool  orthoIntersects = true;
            float dist            = distPointLine(mousePos, vec2(ply->at(i).position),
                                                  vec2(ply->at((i + 1) % ply->size()).position), &orthoIntersects);
            if (orthoIntersects) sortedDist[dist] = i;
        }

        if (sortedDist.begin()->second + 1 < m_polygon->getPoints(0)->size())
            m_polygon->insertBeforePoint(0, sortedDist.begin()->second + 1, mousePos.x, mousePos.y);
        else
            m_polygon->addPoint(0, mousePos.x, mousePos.y);

        procSteps->at(winProcStep::Tesselate).active = true;
}

}  // namespace ara
