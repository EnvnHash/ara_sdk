//
// Created by sven on 21-04-25.
//

#pragma once

#include <GlbCommon/GlbCommon.h>

namespace ara {

enum class state : int32_t {
    none = 0,
    selected,
    highlighted,
    disabled,
    disabledSelected,
    disabledHighlighted,
    m_state,
    count
};

enum class styleInit : int32_t {
    none = 0,
    x,
    y,
    width,
    height,
    align,
    valign,
    color,
    bkcolor,
    brdColor,
    brdWidth,
    brdRadius,
    padding,
    image,
    imagePadding,
    imageOnState,
    imageStates,
    imageOnStateBack,
    imgFlag,
    imgAlign,
    imgScale,
    textColor,
    text,
    textAlign,
    textValign,
    fontFontSize,
    fontFontFamily,
    labelOptions,
    caretColor,
    rowHeight,
    visible
};

enum class pivotX : int32_t { left = 0, right, center };
enum class pivotY : int32_t { bottom = 0, top, center };
enum class sliderScale : int32_t { slideLinear = 0, slidSqrt, slidSquared };

class UINode;

class ObjPosIt {
public:
    std::vector<std::unique_ptr<UINode>>::iterator              it;
    std::list<std::vector<std::unique_ptr<UINode>>::iterator>   parents;
    std::vector<std::unique_ptr<UINode>>*                       list           = nullptr;

    UINode*             foundNode      = nullptr;
    uint32_t            foundId        = 0;
    int32_t             foundTreeLevel = -1;
    int32_t             treeLevel      = 0;
    std::list<UINode*>  localTree;
    glm::vec2           pos{};
    hidEvent            event{};
};

class DivVaoData {
public:
    glm::vec4 pos{};
    glm::vec2 texCoord{};
    glm::vec4 color{};
    glm::vec4 aux0{};
    glm::vec4 aux1{};   /// aux1 (Div: x:borderWidth, y:borderHeight,
                        /// z:borderRadiusX, w:borderRadiusY
    glm::vec4 aux2{};
    glm::vec4 aux3{};
};

class DrawSet;

class IndDrawBlock {
public:
    void stdInit() {
        vaoData.resize(4);
        indices.resize(6);
    }
    std::pair<std::vector<DivVaoData>*, uint32_t> getUpdtPair() { return {&vaoData, vaoOffset}; }

    std::vector<DivVaoData> vaoData;
    std::vector<GLuint>     indices;
    uint32_t                vaoOffset = 0;
    DrawSet*                drawSet   = nullptr;
};

}