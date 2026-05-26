//
// Created by user on 5/5/25.
//

#pragma once

#include <UICommon.h>
#include <DataModel/Node.h>

namespace ara {

NLOHMANN_JSON_SERIALIZE_ENUM(pivotX, {
    {pivotX::center, "center"},
    {pivotX::left, "left"},
    {pivotX::right, "right"}
});

NLOHMANN_JSON_SERIALIZE_ENUM(pivotY, {
    {pivotY::center, "center"},
    {pivotY::bottom, "bottom"},
    {pivotY::top, "top"}
});

class UISharedRes;
class UINode;

class UINodeGeom  : public Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_excludeFromPaddingAndBorder, m_excludeFromParentContentTrans, m_skipBoundCheck, m_excludeFromParentScissoring, m_scissorChildren, m_limitContentTrans, m_posXInt, m_posYInt, m_posXFloat, m_posYFloat, m_widthInt, m_widthFloat, m_heightInt, m_heightFloat, m_widthType, m_heightType, m_zPos, m_alignX, m_alignY, m_pivX, m_pivY, m_fixAspect, m_borderRadius, m_borderWidth, m_padding, m_viewPort)

    UINodeGeom();
    ~UINodeGeom() override = default;

    virtual void setStyleInitVal(const std::string& name, const std::string& val, state st = state::m_state) = 0;
    virtual void updateMatrix() = 0;
    virtual void setChanged(bool val) = 0;
    virtual UINode* getParent() = 0;

    template <typename T>
    requires std::integral<T> || std::floating_point<T>
    void setCoord(T coord, const coordComp comp, const state st) {
        if (st == state::m_state || st == m_state) {
            if (comp == coordComp::x) {
                if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
                    m_posXInt = coord;
                    m_posXType = unitType::Pixels;
                } else if constexpr (std::is_same_v<T, float>) {
                    m_posXFloat = coord;
                    m_posXType = unitType::Percent;
                }
            } else {
                if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
                    m_posYInt = coord;
                    m_posYType = unitType::Pixels;
                } else if constexpr (std::is_same_v<T, float>) {
                    m_posYFloat = coord;
                    m_posYType = unitType::Percent;
                }
            }
            setChanged(true);
        }

        std::string valueStr;
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
            valueStr = std::to_string(coord) + "px";
        } else if constexpr (std::is_same_v<T, float>) {
            valueStr = std::to_string(coord * 100) + "%";
        }

        setStyleInitVal(comp == coordComp::x ? "x" : "y", valueStr, st);
    }

    template <typename T>
    requires std::integral<T> || std::floating_point<T>
    void setSizeComp(T val, const coordComp comp, const state st) {
        if (st == state::m_state || st == m_state) {
            if (comp == coordComp::x) {
                if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
                    m_widthInt = val;
                    m_widthType = unitType::Pixels;
                } else if constexpr (std::is_same_v<T, float>) {
                    m_widthFloat = val;
                    m_widthType = unitType::Percent;
                }
            } else {
                if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
                    m_heightInt = val;
                    m_heightType = unitType::Pixels;
                } else if constexpr (std::is_same_v<T, float>) {
                    m_heightFloat = val;
                    m_heightType = unitType::Percent;
                }
            }
            setChanged(true);
        }

        std::string valueStr;
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>) {
            valueStr = std::to_string(val) + "px";
        } else if constexpr (std::is_same_v<T, float>) {
            valueStr = std::to_string(val * 100) + "%";
        }

        setStyleInitVal(comp == coordComp::x ? "width" : "height", valueStr, st);
    }

    /** set x-position in pixels (int) (Values can be negative this means extreme right minus nr of pixels)
     or percentage (float) 0 - 1, 0 = left, 1 = right **/
    template<typename T>
    void setX(T x, state st = state::m_state) {
        setCoord(x, coordComp::x, st);
    }

    /** set y-position in absolute pixels (int). +y means towards the bottom
     or in percentage (float) 0 - 1, 0 = top, 1 = bottom **/
    template<typename T>
    void setY(T y, state st = state::m_state) {
        setCoord(y, coordComp::y, st);
    }
    /** set position in absolute pixels (int) or percentage (float). -y means towards the top Values can be negative
     * this means extreme right/top minus nr of pixels. **/
    template<typename T, typename U>
    void setPos(T posX, U posY, state st = state::m_state) {
        setCoord(posX, coordComp::x, st);
        setCoord(posY, coordComp::y, st);
    }

    template<typename T>
    requires std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::vec2>
    void setPos(T pos, state st = state::m_state) {
        setCoord(pos.x, coordComp::x, st);
        setCoord(pos.y, coordComp::y, st);
    }

    virtual void       setZPos(const float val) { m_zPos = val; }

    /** set width in absolute pixels (int), in relative percentage (float) 0-1 **/
    template<typename T>
    void setWidth(T width, state st = state::m_state) {
        setSizeComp(width, coordComp::x, st);
    }

    /** set height in absolute pixels (int), or in relative percentage (float) 0-1**/
    template<typename T>
    void setHeight(T height, state st = state::m_state) {
        setSizeComp(height, coordComp::y, st);
    }

    /** set width and height in absolute pixels (int) or relative percentage (float). Values can be negative this
     * means full width/height minus nr of pixels **/
    template<typename T, typename U>
    void setSize(T width, U height, state st = state::m_state) {
        setSizeComp(width, coordComp::x, st);
        setSizeComp(height, coordComp::y, st);
    }

    template<typename T>
    requires std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::vec2>
    void setSize(T size, state st = state::m_state) {
        setSizeComp(size.x, coordComp::x, st);
        setSizeComp(size.y, coordComp::y, st);
    }

    /** implicitly sets pivot to the same value  */
    void setAlignX(align type, state st = state::m_state);
    /** implicitly sets pivot to the same value  */
    void setAlignY(valign type, state st = state::m_state);
    void setAlign(align alignX, valign alignY, state st = state::m_state);

    /** explicitly set pivot x. Must be set AFTER setAlign */
    void setPivotX(pivotX pX);
    /** explicitly set pivot y. Must be set AFTER setAlign */
    void setPivotY(pivotY pY);
    /** explicitly set the pivot point. Must be set AFTER setAlign */
    void setPivot(pivotX pX, pivotY pY);

    /** padding is in the parent, the children read it from the parent's value when calculating their matrices */
    template<typename T>
    requires std::is_same_v<T, glm::vec4> || std::is_same_v<T, float>
    void setPadding(const T& val, const state st = state::m_state) {
        if (st == state::m_state || st == m_state) {
            if constexpr (std::is_same_v<T, float>) {
                m_padding = glm::vec4{val, val, val, val};
            } else {
                m_padding = val;
            }
            m_geoChanged = true;
        }

        std::string styleInitStr;
        if constexpr (std::is_same_v<T, float>) {
            styleInitStr = std::to_string(val);
        } else {
            styleInitStr = std::to_string(val[0]) + "," + std::to_string(val[1]) + "," + std::to_string(val[2]) + "," + std::to_string(val[3]);
        }

        setStyleInitVal("padding", styleInitStr, st);
    }

    virtual void setPadding(const float left, const float top, const float right, const float bottom, state st = state::m_state) {
        setPadding(glm::vec4{left, top, right, bottom});
    }

    void setBorderWidth(uint32_t val, state st = state::m_state);
    void setBorderRadius(uint32_t val, state st = state::m_state);

    /** in pixels. the origin is bottom, left (as opengl requests these coordinates) */
    virtual void setViewport(float x, float y, float width, float height);
    virtual void setViewport(glm::vec4* viewport) { m_viewPort = *viewport; }

    virtual void setZoomNormMat(float val);
    virtual void setZoomWithCenter(float val, glm::vec2& actMousePos);

    /** relative float values */
    virtual void setContentTransScale(float x, float y);
    /** in pixels */
    virtual void setContentTransTransl(float x, float y);
    virtual void setContentRotation(float angle, float ax, float ay, float az);
    void         setContentTransCentered(const bool val) { m_contTransMatCentered = val; }
    void         setSnapToHwPix(const bool val) { m_snapToHwPix = val; }
    virtual void setGeoChanged(const bool val) { m_geoChanged = val; }

    static bool contains(UINodeGeom* outer, UINodeGeom* node);

    /** return the x and y position, as it was set with setX/setY/setPos before,
     * that respecting alignment, returns either pixels or normalized percentages */
    glm::vec2 getOrigPos();
    /** return the node's x and y position relative to the upper left window corner in pixels */
    virtual glm::vec2& getWinPos();
    /** return the x and y position relative to the parent's node's upper left corner in pixels */
    virtual glm::vec2& getParentNodeRelPos();
    /** return the x and y position relative to the parent's content's upper left corner in pixels */
    glm::vec2& getPos();
    /** return the x and y position in pixels, respecting the UINodes alignment */
    glm::vec2& getAlignedPos();

    [[nodiscard]] float     getZPos() const { return m_zPos; }
    /** return the node's content's x and y position relative to the window's upper left corner in pixels */
    virtual glm::vec2& getContWinPos();
    /** return the size of the node (including padding and border) */
    glm::vec2& getSize();
    /** return the size of the node, including padding, border and all parents contenttransformations */
    virtual glm::vec2& getWinRelSize();
    /** return the size of the node content area, (node size minus padding and border) */
    virtual glm::vec2& getContentSize();
    virtual glm::vec2& getContentOffset();

    virtual void calcNormMat();

    /** returns the size of the node's outer bounds in pixels (including padding and border) */
    glm::vec2& getNodeSize();
    /** returns the width of the outer bounds of the node in pixels (including padding and border) */
    float getNodeWidth();
    /** returns the height of the outer bounds of the node in pixels (including padding and border) */
    float getNodeHeight();
    /** returns the size of the node's outer bounds (including padding and border), relative to its parent (0-1) */
    glm::vec2 getNodeRelSize();
    /** returns the viewport of the node. meant to be used with glScissor() or
     * glViewport(). THIS IS BOTTOM LEFT ORIGIN, AS OPENGL DEFINES IT */
    glm::vec4 getNodeViewportGL();
    glm::vec4 getContentViewport();
    glm::vec4 getNodeViewport();

    void setFixAspect(float val);

    [[nodiscard]] float getAspect() const { return m_fixAspect > -1.f ? m_fixAspect : m_aspect; }
    [[nodiscard]] bool  getScissorChildren() const { return m_scissorChildren; }

    [[nodiscard]] align  getAlignX() const { return m_alignX; }
    [[nodiscard]] valign getAlignY() const { return m_alignY; }
    [[nodiscard]] pivotX getPivotX() const { return m_pivX; }
    [[nodiscard]] pivotY getPivotY() const { return m_pivY; }

    [[nodiscard]] bool isViewportValid() const {
        return ((m_viewPort.x + m_viewPort.y + m_viewPort.z + m_viewPort.w) != 0);
    }

    glm::vec4*  getViewport() { return &m_viewPort; }
    float*      getMVPMatPtr();
    glm::mat4*  getMvp();
    glm::mat4*  getHwMvp();
    float*      getWinRelMatPtr();
    glm::mat4*  getWinRelMat();
    glm::mat4*  getNormMat();
    float*      getNormMatPtr();

    [[nodiscard]] uint32_t getBorderWidth() const { return m_borderWidth; }
    /** meant for use in shaders to draw the border with node relative normalized coordinates */
    glm::vec2&             getBorderWidthRel();
    [[nodiscard]] uint32_t getBorderRadius() const { return m_borderRadius; }
    glm::vec2&             getBorderRadiusRel();
    glm::vec2&             getBorderAliasRel();
    glm::vec4&             getPadding() { return m_padding; }

    // contentMat is in relative coords
    glm::vec3&          getContentTransTransl() { return m_contentTransMatTransl; }
    glm::mat4&          getContentTransMat() { return m_contentTransMat; }
    glm::vec2           getContentTransScale2D() { return {m_contentTransScale}; }
    glm::vec3&          getContentTransScale() { return m_contentTransScale; }
    [[nodiscard]] float getZoom() const { return m_contentTransScale.x; }
    glm::vec2&          getParentContentScale();
    [[nodiscard]] float getFixAspect() const { return m_fixAspect; }
    [[nodiscard]] float getPixRatio() const;

    virtual glm::mat4*  getContentMat(bool excludedFromParentContentTrans = false, bool excludedFromPaddingAndBorder = false);
    virtual glm::mat4*  getFlatContentMat(bool excludedFromParentContentTrans = false, bool excludedFromPaddingAndBorder = false);

    UISharedRes*        getSharedRes() const { return m_sharedRes; }
    virtual void        setSharedRes(UISharedRes* shared);

    void                updateContentTransMat();
    void                calcContentTransMat(glm::mat4& mat, const glm::vec3& trans) const;

    void                excludeFromParentPaddingAndBorder(const bool val) { m_excludeFromPaddingAndBorder = val; }
    [[nodiscard]] bool  isExcludedFromPaddingAndBorder() const { return m_excludeFromPaddingAndBorder; }
    void                excludeFromParentContentTrans(const bool val) { m_excludeFromParentContentTrans = val; }
    [[nodiscard]] bool  isExcludedFromParentContentTrans() const { return m_excludeFromParentContentTrans; }
    void                excludeFromOutOfBorderCheck(const bool val) { m_skipBoundCheck = val; }
    void                excludeFromScissoring(const bool val) { m_excludeFromParentScissoring = val; }
    void                excludeSizeFromParentContentTrans(const bool val) { m_excludeSizeFromParentContentTrans = val; }
    void                setStaticBorderSize(const bool val) { m_staticBorderSize = val; }
    [[nodiscard]] bool  isExcludedFromScissoring() const { return m_excludeFromParentScissoring; }
    void                limitContentTrans(const bool val) { m_limitContentTrans = val; }
    [[nodiscard]] bool  changed() const { return m_geoChanged; }

    // Bounding Box
    bool setScissorChildren(const bool on_off) { return (m_scissorChildren = on_off); }

    /** get the bounding box around the children in parent relative coordinates
     * (without the parent's transformation matrix). [0]: left/top x, [1]: left/top y, [2]: right/bottom x, [3]: right/bottom  */
    glm::vec4& getChildrenBoundBox() { return m_childBoundBox; }

    /** get the bounding box around the children in parent relative coordinates
     * (without the parent's transformation matrix) plus padding  */
    glm::vec2 getChildrenBBSizeWithPadding() {
        return {m_childBoundBox.z - m_childBoundBox.x + getPadding().x + getPadding().z,
                m_childBoundBox.w - m_childBoundBox.y + getPadding().y + getPadding().w};
    }

    void load(const std::filesystem::path& filePath, const bool skipNonClass = false) override {
        Node::load(filePath, skipNonClass);
        setChanged(true);
    }

    bool isOutOfParentBounds();
    virtual bool isInBounds(glm::vec2& pos);

protected:
    void checkUpdateMatrix();
    glm::vec2 getContentTransOverflow(float x, float y);

    UISharedRes* m_sharedRes = nullptr;
    scissorStack m_scissorStack;
    std::function<bool(ObjPosIt&)> m_outOfTreeObjId;

    /** if false, offset and scaling is done relative to 0|0 top left origin, if
    * true relative to the center of the UINode **/
    bool m_contTransMatCentered                 = false;
    bool m_excludeFromPaddingAndBorder          = false;
    bool m_scissorChildren                      = false;
    bool m_hasContRot                           = false;
    bool m_geoChanged                           = true;
    bool m_excludeFromParentScissoring          = false;
    bool m_excludeFromParentContentTrans        = false;
    bool m_excludeSizeFromParentContentTrans    = false;
    bool m_staticBorderSize                     = true; /// when set, border stays same on zooming (parentContentTrans)
    bool m_isOutOfParentBounds                  = false;
    bool m_skipBoundCheck                       = false;
    bool m_oob                                  = false;
    bool m_updating                             = false;
    bool m_limitContentTrans                    = false;
    bool m_snapToHwPix                          = false;

    int32_t m_posXInt   = 0;
    int32_t m_posYInt   = 0;
    float   m_posXFloat = 0;
    float   m_posYFloat = 0;
    float   m_zPos      = 1.f;

    unitType m_posXType   = unitType::Pixels;
    unitType m_posYType   = unitType::Pixels;
    unitType m_widthType  = unitType::Percent;
    unitType m_heightType = unitType::Percent;

    /** the node's position relative to the window in pixels */
    glm::vec2 m_winRelPos{};
    glm::vec2 m_winRelSize{};

    /** all parent's content * contentTransformation matrices -> the flattened
    * matrixStack at this position in the scene graph. */
    glm::mat4* m_parentMat       = nullptr;
    glm::mat4  m_parentMatLocCpy = glm::mat4(1.f);
    glm::mat4  m_identMat        = glm::mat4(1.f);

    int32_t m_widthInt    = 0;
    int32_t m_heightInt   = 1;
    float   m_widthFloat  = 1.f;
    float   m_heightFloat = 1.f;

    float m_fixAspect       = -1.f;
    float m_aspect          = 1.f;

    align  m_alignX = align::left;
    valign m_alignY = valign::top;

    pivotX m_pivX = pivotX::left;
    pivotY m_pivY = pivotY::top;

    glm::vec4 m_padding{};
    glm::vec3 m_init_size{1.f};
    glm::vec3 m_work_size{1.f};

    /** the node's size relative to its parent in normalized coordinates (to be
     * passed to a shader to render the node) */
    glm::vec2 m_relSize{0};
    /** the node's size in pixels (including padding and border) */
    glm::vec2 m_size{0};
    /** the node's content size in pixels (without padding and border) */
    glm::vec2 m_contentSize{0};
    /** the node's content offset in pixels (padding + border) */
    glm::vec2 m_contentOffset{0};

    /** bounding box around all children in format left/top x,y and right/bottom x.y parentRelative */
    glm::vec4 m_childBoundBox{0};

    /** the position relative to the parent's (transformed) content's top left corner in pixels */
    glm::vec2 m_pos{};
    /** the position as set initially before alignment calculations */
    glm::vec2 m_preAlignPos{};
    /** the position relative to the parent's node's top left corner in pixels
     * (ignoring border and padding) */
    glm::vec2 m_parentNodeRelPos{};
    /** the content's position relative to window's top left corner in pixels */
    glm::vec2 m_contWinPos{};
    /** the node's position in parent contentTransMat space */
    glm::vec2 m_parentTransPos{};

    /** the node's window relative matrix. */
    glm::mat4 m_winRelMat = glm::mat4(1.f);
    /** to be passed down to the children if the node is excluded from its
     * parents contentTransOffset. Represents the node's content relative to
     * it's parent => excludes the border and padding areas. in pixels */
    glm::mat4 m_contentMat   = glm::mat4(1.f);
    glm::mat4 m_nodeMat      = glm::mat4(1.f);
    glm::mat4 m_nodeTransMat = glm::mat4(1.f);
    /** the node's matrix multiplied with the UIWindow's orthographic matrix.
     * this is meant to be passed to a shader for rendering the node */
    glm::mat4 m_mvp = glm::mat4(1.f);
    /** the node's matrix multiplied with the UIWindow's orthographic matrix in
     * hardware pixels. this is meant to be passed to a shader for rendering the node */
    glm::mat4 m_mvpHw = glm::mat4(1.f);
    /** the node's matrix multiplied with the UIWindow's matrix in normalized
     * opengl coordinate system. this is meant to be passed to a shader for
     * rendering the node */
    glm::mat4 m_normMat = glm::mat4(1.f);

    /** to be passed down to the children matrix for transforming children
     * inside the nodes content matrix (e.g. ScrollView, zooming of images etc.)
     */
    glm::vec3 m_contentTransScaleFixAspect{1.f};
    glm::vec3 m_contentTransScale{1.f};
    glm::vec3 m_contentTransMatTransl{};
    glm::vec4 m_contentTransRotate{1.f, 0.f, 0.f, 0.f};
    glm::mat4 m_contentTransMat    = glm::mat4(1.f);
    glm::mat4 m_contentTransMatRel = glm::mat4(1.f);
    glm::mat4 m_contRot            = glm::mat4(1.f);
    glm::mat4 m_nodePosMat         = glm::mat4(1.f);
    glm::mat4 m_nodePosMatHw       = glm::mat4(1.f);  // in hw pixels
    glm::vec2 m_contentCenterOffs{};               // in hw pixels

    /** the nodes transformed content matrix, multiplied with the matrix stack */
    glm::mat4 m_flatContentTransMat = glm::mat4(1.f);

    /** the node's viewport (x,y,width,height) in window relative pixels */
    glm::vec4 m_parentContVp{1.f};
    glm::vec4 m_viewPort        = glm::vec4(0.f);
    glm::vec4 m_scissVp{};

    // temp value for scissoring
    glm::vec4 m_sc{};
    glm::vec4 m_scIndDraw{};
    glm::vec4 m_bb{};

    glm::vec2  m_parentContScale = glm::vec2{1.f, 1.f};  // contentTransMat scaling part
    glm::mat4* m_orthoMat        = nullptr;

    glm::vec2 m_borderWidthRel{0.001f, 0.001f};
    uint32_t  m_borderWidth  = 0;
    uint32_t  m_borderRadius = 0;
    uint32_t  m_borderAlias  = 1;
    glm::vec2 m_borderRadiusRel{0};
    glm::vec2 m_borderAliasRel{0};

    state m_state     = state::none;
    state m_lastState = state::none;
};

}