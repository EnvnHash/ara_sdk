//
// Created by user on 02.07.2020.
//

#pragma once

#include <GeoPrimitives/Polygon.h>
#include <Utils/FBO.h>

#include "UINodeBase/UINode.h"

namespace ara {
class UIPolygon : public UINode {
public:
    UIPolygon();
    ~UIPolygon() override = default;

    void init() override;
    bool draw(uint32_t* objId) override;
    void drawCtrlPoints(uint32_t* id);
    void initCtrlPointShdr();
    void tesselate() const;
    bool objIdInPolygon(int id) const;
    void mouseDrag(hidData* data) override;
    void mouseDown(hidData* data) override;
    void onResize() override;
    void addPointToSelection(uint32_t clickedObjId, uint32_t level, std::map<winProcStep, ProcStep>* procSteps);
    bool checkCtrlPointInSel(uint32_t _id) const;
    void clearSelQueue();
    void createCopyForUndo();
    void moveCtrlPoints(glm::vec2 _offset, cpEditMode cp_editM);
    void addPoint(glm::vec2 pos, std::map<winProcStep, ProcStep>* procSteps);

    void setDrawInvert(bool val) { m_drawInv = val; }

private:
    std::unique_ptr<Polygon> m_polygon;
    Shaders*                 m_ctrPointShdr = nullptr;
    Shaders*                 m_uiTexShdr    = nullptr;
    std::unique_ptr<FBO>     m_fbo;
    std::vector<glm::vec4>   m_lColor;
    int                      m_ctrlPointSizePix;
    bool                     m_drawInv = true;

    std::function<void(Shaders*, uint32_t*)> m_objIdDrawFunc;
    std::map<uint32_t, CtrlPoint*>           m_cpSelectQueue;

    glm::vec2 m_mc_mousePos{};
    glm::vec2 m_relMouseOffs{};
    glm::mat4 m_ident{};
};
}  // namespace ara
