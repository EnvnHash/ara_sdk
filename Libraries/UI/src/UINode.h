#pragma once

#include <GLBase.h>
#include <Property.h>
#include <ui_common.h>
#include <Utils/UniformBlock.h>

#include "ListProperty.h"
#include "ObjectMapInteraction.h"
#include "UISharedRes.h"
#include "glsg_common/glsg_common.h"

namespace ara {

class ResNode;
class UISharedRes;
class UIWindow;
class UIApplication;
class FloatingMenuDialog;

class UINode {
public:
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

    UINode();
    virtual ~UINode();

    virtual void init() {}

    /** draw the scenegraph from this node onwards */
    virtual void drawAsRoot(uint32_t* objId);

    /** the scenegraph drawing iteration loop */
    virtual void updtMatrIt(scissorStack* ss);

    /** the scenegraph drawing iteration loop */
    virtual void drawIt(scissorStack& ss, uint32_t* objId, bool treeChanged, bool* skipFirst);
    virtual bool draw(uint32_t* objId) { return true; }
    virtual bool drawIndirect(uint32_t* objId) { return true; }
    virtual void updateDrawData() {}

    // Mouse interaction
    static void hidIt(hidData* data, hidEvent evt, std::list<UINode*>::iterator it, std::list<UINode*>& tree);

    virtual void keyDownIt(hidData* data);
    virtual void onCharIt(hidData* data);
    // virtual void clearFocusIt();

    // mouse HID methods which are called during local tree iteration (root -> this node)
    virtual void mouseMove(hidData* data) {}
    virtual void mouseDrag(hidData* data) {}
    virtual void mouseDown(hidData* data) {}
    virtual void mouseDownRight(hidData* data) {}
    virtual void mouseUp(hidData* data) {}
    virtual void mouseUpRight(hidData* data) {}
    virtual void mouseWheel(hidData* data) {}

    // mouseIn and mouseOut are called directly without tree iteration
    virtual void mouseIn(hidData* data);
    virtual void mouseOut(hidData* data);

    // keyboard HID methods which are called during local tree iteration (root -> this node)
    virtual void keyDown(hidData* data) {}
    virtual void keyUp(hidData* data) {}
    virtual void onChar(hidData* data) {}
    virtual void onResize() {}

    virtual UINode* addChild(std::unique_ptr<UINode>&& child);
    virtual UINode* insertChild(int32_t position, std::unique_ptr<UINode>&& child);
    virtual UINode* insertChild(const std::string& name, std::unique_ptr<UINode>&& child);
    virtual UINode* insertAfter(const std::string& name, std::unique_ptr<UINode>&& child);
    virtual void    moveChildTo(int32_t position, UINode*);

    template <typename T>
    T* addChild() {
        return static_cast<T*>(UINode::addChild(static_cast<std::unique_ptr<UINode>>(std::make_unique<T>())));
    }

    template <typename T>
    T* insertChild(int32_t position) {
        return static_cast<T*>(UINode::insertChild(position, std::make_unique<T>()));
    }

    template <typename T>
    T* insertChild(const std::string& name) {
        return static_cast<T*>(UINode::insertChild(name, std::make_unique<T>()));
    }

    template <typename T>
    T* insertAfter(const std::string& name) {
        return static_cast<T*>(UINode::insertAfter(name, std::make_unique<T>()));
    }

    template <typename T>
    T* addChild(std::string* styleClass) {
        T* nc = UINode::addChild<T>();
        nc->addStyleClass(std::move(*styleClass));
        return nc;
    }

    template <typename T>
    T* addChild(std::string&& styleClass) {
        T* nc = UINode::addChild<T>();
        nc->addStyleClass(std::move(styleClass));
        return nc;
    }

    template <typename T>
    T* insertChild(int32_t position, std::string* styleClass) {
        T* nc = UINode::insertChild<T>(position);
        nc->addStyleClass(std::move(*styleClass));
        return nc;
    }

    template <typename T>
    T* insertChild(int32_t position, std::string&& styleClass) {
        T* nc = UINode::insertChild<T>(position);
        nc->addStyleClass(std::move(styleClass));
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h) {
        T* nc = UINode::addChild<T>();
        nc->setPos(x, y);
        nc->setSize(w, h);
        return nc;
    }

    template <typename T>
    T* addChild(float x, float y, float w, float h) {
        T* nc = UINode::addChild<T>();
        nc->setPos(x, y);
        nc->setSize(w, h);
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h, const float* fgcolor, const float* bkcolor) {
        T*    nc = UINode::addChild<T>();
        auto* n  = static_cast<UINode*>(nc);
        n->setPos(x, y);
        n->setSize(w, h);
        if (fgcolor != nullptr) n->setColor(fgcolor[0], fgcolor[1], fgcolor[2], fgcolor[3]);
        if (bkcolor != nullptr) n->setBackgroundColor(bkcolor[0], bkcolor[1], bkcolor[2], bkcolor[3]);
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h, glm::vec4 fgcolor, glm::vec4 bkcolor) {
        T*    nc = UINode::addChild<T>();
        auto* n  = static_cast<UINode*>(nc);
        n->setPos(x, y);
        n->setSize(w, h);
        n->setColor(fgcolor);
        n->setBackgroundColor(bkcolor);
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h, std::filesystem::path path) {
        T*    nc = UINode::addChild<T>();
        auto* n  = static_cast<UINode*>(nc);
        n->setPos(x, y);
        n->setSize(w, h);
        n->setPath(std::move(path));
        return nc;
    }

    virtual void remove_child(UINode* node);
    virtual void init_child(UINode* _child, UINode* _parent);
    virtual bool removeFocus();
    virtual void clearChildren();

    // utility function for connecting to properties. stores a local callback
    // function add passes it as to the property which will store a reference to
    // it as a weak pointer on dtor the referring pointer is freed and by
    // weak_ptr.lock() checking its weak_ptr reference inside the Property is
    // deleted implicitly
    template <typename T>
    void onChanged(Property<T>* prop, std::function<void(std::any)> f) {
        if (prop) {
            m_onValChangedCb[prop] = std::make_shared<std::function<void(std::any)>>(f);
            prop->onPreChange(m_onValChangedCb[prop]);
        }
    }

    template <typename T>
    void onChanged(PropertyPtr<T>& prop, std::function<void(std::any)> f) {
        m_onValChangedCb[prop.ptr] = std::make_shared<std::function<void(std::any)>>(f);
        if (prop.ptr) prop.onPreChange(m_onValChangedCb[prop.ptr]);
    }

    // utility method to also immediately execute the function
    template <typename T>
    void execAndBind(Property<T>* prop, std::function<void(std::any)> f) {
        onChanged<T>(prop, f);
        f((*prop)());
    }

    template <typename T>
    void onChanged(ListProperty<T>* prop, std::function<void(std::any)> f) {
        if (prop) {
            m_onValChangedCb[prop] = std::make_shared<std::function<void(std::any)>>(f);
            prop->onChanged(m_onValChangedCb[prop]);
        }
    }

    // utility method to also immediately execute the function
    template <typename T>
    void execAndBind(ListProperty<T>* prop, std::function<void(std::any)> f) {
        onChanged<T>(prop, f);
        f((*prop)());
    }

    template <typename T>
    void removeOnChanged(Property<T>* prop) {
        m_onValChangedCb[prop] = nullptr;
    }

    virtual void addGlCb(const std::string&, const std::function<bool()>& f);
    virtual void eraseCb(const std::string&);
    virtual bool hasCb(const std::string&);

    uint32_t                                getMinChildId(uint32_t minId = UINT32_MAX);
    uint32_t                                getMaxChildId(uint32_t maxId = 0);
    std::vector<std::unique_ptr<UINode>>&   getChildren() { return m_children; }
    [[nodiscard]] uint32_t                  getId() const { return m_objIdMin; }
    [[nodiscard]] uint32_t                  getMinId() const { return m_objIdMin; }
    [[nodiscard]] uint32_t                  getMaxId() const { return m_objIdMax; }
    void                                    setId(uint32_t val) { m_objIdMin = val; m_objIdMax = val; }
    void               setMinId(uint32_t val) { m_objIdMin = val; }
    void               setMaxId(uint32_t val) { m_objIdMax = val; }
    [[nodiscard]] bool containsObjectId(uint32_t id) const { return (id >= m_objIdMin && id <= m_objIdMax); }
    void               setHasDepth(bool val) { m_hasDepth = val; }
    virtual void       setZPos(float val) { m_zPos = val; }

    UINode*                 getNode(const std::string& name);
    UINode*                 getNodeById(uint32_t searchID);
    UINode*                 getRoot();
    virtual UINode*         getParent() { return m_parent; }
    virtual float           getValue() { return 0.f; }
    virtual float           getAlpha() { return m_absoluteAlpha; }
    virtual std::string&    getName() { return m_name; }
    [[nodiscard]] float     getZPos() const { return m_zPos; }
    [[nodiscard]] bool      canReceiveDrag() const { return m_canReceiveDrag; }
    [[nodiscard]] bool      getScissorChildren() const { return m_ScissorChildren; }
    std::filesystem::path   dataPath() { return m_sharedRes ? m_sharedRes->dataPath : std::filesystem::current_path(); }
    UISharedRes*            getSharedRes() { return m_sharedRes; }
    UIWindow*               getWindow();
    UIApplication*          getApp();
    float                   getPixRatio();

    // functional
    virtual void setSharedRes(UISharedRes* sharedRes);
    virtual void setCanReceiveDrag(bool val) { m_canReceiveDrag = val; }
    virtual void setParent(UINode* parent) { m_parent = parent; }
    virtual void setRefDraw(bool v) { m_referenceDrawing = true; }
    virtual void setVisibility(bool val, state st = state::m_state);
    /** in pixels. origin is bottom, left (as opengl requests this coordinates)
     */
    virtual void setViewport(float x, float y, float width, float height);
    virtual void setViewport(glm::vec4* viewport) { m_viewPort = *viewport; }

    virtual void setZoomNormMat(float val);
    virtual void setZoomWithCenter(float val, glm::vec2& center);
    void         setName(std::string name) { m_name = std::move(name); }
    void         setMouseIcon(MouseIcon icon) { ui_MouseIcon = icon; }
    void         setFontType(std::string fontType) { m_fontType = std::string(std::move(fontType)); }
    std::string& getFontType() { return m_fontType; }
    virtual void setValue(float val) {}
    virtual void setAbsValue(float val) {}
    virtual void setPath(std::filesystem::path file) { m_filepath = std::move(file); }
    void setKeyDownCb(std::function<void(hidData*)> func) { m_keyDownCb = std::move(func); }
    void setKeyUpCb(std::function<void(hidData*)> func) { m_keyUpCb = std::move(func); }

    void addMouseHidCb(hidEvent evt, const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseClickCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseClickRightCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseUpCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseUpRightCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseDragCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseMoveCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void addMouseWheelCb(const std::function<void(hidData*)>& func, bool onHit = true);
    void clearMouseCb(hidEvent evt);
    void addMouseInCb(std::function<void(hidData*)> func, state st = state::m_state);
    void addMouseOutCb(std::function<void(hidData*)> func, state st = state::none);

    std::list<mouseCb>&             getMouseHidCb(hidEvent evt) { return m_mouseHidCb[evt]; }
    std::list<mouseCb>&             getMouseDownCb() { return m_mouseHidCb[hidEvent::MouseDownLeft]; }
    std::list<mouseCb>&             getMouseUpCb() { return m_mouseHidCb[hidEvent::MouseUpLeft]; }
    std::list<mouseCb>&             getMouseDownRightCb() { return m_mouseHidCb[hidEvent::MouseDownRight]; }
    std::list<mouseCb>&             getMouseUpRightCb() { return m_mouseHidCb[hidEvent::MouseUpRight]; }
    std::list<mouseCb>&             getMouseDragCb() { return m_mouseHidCb[hidEvent::MouseDrag]; }
    std::list<mouseCb>&             getMouseWheelCb() { return m_mouseHidCb[hidEvent::MouseWheel]; }
    std::function<void(hidData*)>*  getKeyDownCb() { return m_keyDownCb ? &m_keyDownCb : nullptr; }
    std::function<void(hidData*)>*  getKeyUpCb() { return m_keyUpCb ? &m_keyUpCb : nullptr; }

    [[nodiscard]] bool      changed() const { return m_geoChanged; }
    [[nodiscard]] bool      isInited() const { return m_inited; }
    void                    setInited(bool val) { m_inited = val; }
    void                    setForceInit(bool val) { m_forceInit = val; }
    [[nodiscard]] bool      isVisible() const { return m_visible; }
    void                    excludeFromObjMap(bool val) { m_excludeFromObjMap = val; }
    [[nodiscard]] bool      isExcludedFromObjMap() const { return m_excludeFromObjMap; }
    void                    excludeFromScissoring(bool val) { m_excludeFromParentScissoring = val; }
    [[nodiscard]] bool      isExcludedFromScissoring() const { return m_excludeFromParentScissoring; }
    [[nodiscard]] bool      isHIDBlocked() const { return m_blockHID; }
    void                    setHIDBlocked(bool val);
    virtual void            onLostFocus();
    virtual void            onGotFocus();
    [[nodiscard]] bool      hasInputFocus() const { return m_hasInputFocus; }
    void                    setInputFocus(bool val) { m_hasInputFocus = val; }
    [[nodiscard]] bool      getFocusAllowed() const { return m_focusAllowed; }
    void                    setFocusAllowed(bool val) { m_focusAllowed = val; }
    void                    setOnFocusedCb(std::function<void()> f) { m_onFocusedCb = std::move(f); }
    std::function<void()>*  getOnFocusedCb() { return m_onFocusedCb ? &m_onFocusedCb : nullptr; }
    void                    setOnLostFocusCb(std::function<void()> f) { m_onLostFocusCb = std::move(f); }
    std::function<void()>*  getOnLostFocusCb() { return m_onLostFocusCb ? &m_onLostFocusCb : nullptr; }
    void                    setReturnFocusCb(std::function<void()> func) { m_returnFocusCb = std::move(func); }
    std::function<void()>   getReturnFocusCb() { return m_returnFocusCb; }

    virtual void     dump();
    virtual uint32_t getSubNodeCount();
    virtual void     getSubNodeCountIt(UINode* node, uint32_t* count);

    // matrix operations - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    /** set x-position in pixels (Values can be negative this means extreme
     * right minus nr of pixels) **/
    void setX(int32_t x, state st = state::m_state);
    /** set x-position in percentage 0 - 1, 0 = left, 1 = right **/
    void setX(float x, state st = state::m_state);
    /** set y-position in absolute pixels. +y means towards the bottom **/
    void setY(int32_t y, state st = state::m_state);
    /** set y-position in percentage 0 - 1, 0 = top, 1 = bottom **/
    void setY(float y, state st = state::m_state);

    /** set position in absolute pixels. -y means towards the top Values can be
     * negative this means extreme right/top minus nr of pixels **/
    void setPos(int32_t posX, int32_t posY, state st = state::m_state);
    /** set x-position in absolute pixels (Values can be negative this means
     * extreme right minus nr of pixels) and y-position relative from -0.5 to
     * 0.5. +y means towards the bottom **/
    void setPos(int32_t posX, float posY, state st = state::m_state);
    /** set x-position relative from 0 to 1 (if PIVOT and ALIGN is top left) and
     * y-position in absolute pixels (Values can be negative this means extreme
     * top minus nr of pixels) **/
    void setPos(float posX, int32_t posY, state st = state::m_state);
    /** set position relative from 0 to 1 (if PIVOT and ALIGN is top left). +y
     * means towards the bottom **/
    void setPos(float posX, float posY, state st = state::m_state);

    /** set width in absolute pixels **/
    void setWidth(int32_t width, state st = state::m_state);
    /** set width in relative percentage 0-1  **/
    void setWidth(float width, state st = state::m_state);
    /** set height in absolute pixels **/
    void setHeight(int32_t height, state st = state::m_state);
    /** set height in relative percentage 0-1  **/
    void setHeight(float height, state st = state::m_state);

    /** set width and height in absolute pixels. Values can be negative this
     * means full width/height minus nr of pixels **/
    void setSize(int32_t width, int32_t height, state st = state::m_state);
    /** set width in absolute pixels (Values can be negative this means full
     * width minus nr of pixels) and height in percentage 0-1 **/
    void setSize(int32_t width, float height, state st = state::m_state);
    /** set width relative percentage 0-1 and height in absolute pixels (Values
     * can be negative this means full height minus nr of pixels ) **/
    void setSize(float width, int32_t height, state st = state::m_state);
    /** set width and height relative percentage 0-1 **/
    void setSize(float width, float height, state st = state::m_state);

    /** implicitly sets pivot to same value  */
    void setAlignX(align type, state st = state::m_state);
    /** implicitly sets pivot to same value  */
    void setAlignY(valign type, state st = state::m_state);
    void setAlign(align alignX, valign alignY, state st = state::m_state);

    /** explicitly set pivot x. Must be set AFTER setAlign */
    void setPivotX(pivotX pX);
    /** explicitly set pivot y. Must be set AFTER setAlign */
    void setPivotY(pivotY pY);
    /** explicitly set pivot point. Must be set AFTER setAlign */
    void setPivot(pivotX pX, pivotY pY);

    /** padding is the in the parent, the children read it from the parent's value when calculating their matrices */
    void setPadding(float val, state st = state::m_state);

    virtual void setPadding(float left, float top, float right, float bot, state st = state::m_state);
    virtual void setPadding(glm::vec4& val, state st = state::m_state);

    void setBorderWidth(uint32_t val, state st = state::m_state);
    void setBorderRadius(uint32_t val, state st = state::m_state);

    virtual void setBorderColor(float r, float g, float b, float a, state st = state::m_state);
    virtual void setBorderColor(glm::vec4& col, state st = state::m_state);
    virtual void setColor(float r, float g, float b, float a, state st = state::m_state);
    virtual void setColor(glm::vec4& col, state st = state::m_state);
    virtual void setBackgroundColor(float r, float g, float b, float a, state st = state::m_state);
    virtual void setBackgroundColor(glm::vec4& col, state st = state::m_state);

    glm::vec4& getColor() { return m_color; }
    glm::vec4& getBackgroundColor() { return m_bgColor; }

    void setChanged(bool val);
    void setFixAspect(float val);
    void setChangeCb(std::function<void()> f) { m_changeCb = std::move(f); }
    void reqTreeChanged(bool val) { m_reqTreeChanged = true; }
    void reqUpdtTree();

    virtual void setSelected(bool val, bool forceStyleUpdt = false);
    virtual void setDisabled(bool val, bool forceStyleUpdt = false);
    virtual void setHighlighted(bool val, bool forceStyleUpdt = false);
    virtual void setDisabledHighlighted(bool val, bool forceStyleUpdt = false);
    virtual void setDisabledSelected(bool val, bool forceStyleUpdt = false);

    /** return the x and y position, as it was set with setX/setY/setPos before,
     * that respecting alignment, returns either pixels or normalized percentages */
    glm::vec2 getOrigPos();
    /** return the node's x and y position relative to the upper left window corner in pixels */
    virtual glm::vec2& getWinPos();
    /** return the x and y position relative to the parent's node's upper left corner in pixels */
    virtual glm::vec2& getParentNodeRelPos();
    /** return the x and y position relative to the parent's content's upper left corner in pixels */
    glm::vec2& getPos();
    /** return the node's content's x and y position relative to the window's upper left corner in pixels */
    virtual glm::vec2& getContWinPos();
    /** return the size of the node, (including padding and border) */
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

    [[nodiscard]] float getAspect() const { return m_fixAspect > -1.f ? m_fixAspect : m_aspect; }
    [[nodiscard]] uint32_t getBorderWidth() const { return m_borderWidth; }
    /** meant for use in shaders to draw the border with node relative normalized coordinates */
    glm::vec2&             getBorderWidthRel();
    [[nodiscard]] uint32_t getBorderRadius() const { return m_borderRadius; }
    glm::vec2&             getBorderRadiusRel();
    glm::vec2&             getBorderAliasRel();
    glm::vec4&             getPadding() { return m_padding; }
    align  getAlignX() { return m_alignX; }
    valign getAlignY() { return m_alignY; }
    pivotX getPivotX() { return m_pivX; }
    pivotY getPivotY() { return m_pivY; }

    [[nodiscard]] bool isViewportValid() const {
        return ((m_viewPort.x + m_viewPort.y + m_viewPort.z + m_viewPort.w) != 0);
    }
    glm::vec4* getViewport() { return &m_viewPort; }

    float*      getMVPMatPtr();
    glm::mat4*  getMvp();
    glm::mat4*  getHwMvp();
    float*      getWinRelMatPtr();
    glm::mat4*  getWinRelMat();
    glm::mat4*  getNormMat();
    float*      getNormMatPtr();

    // contentMat is in relative coords
    glm::vec3&          getContentTransTransl() { return m_contentTransMatTransl; }
    glm::mat4&          getContentTransMat() { return m_contentTransMat; }
    glm::vec2           getContentTransScale2D() { return {m_contentTransScale}; }
    glm::vec3&          getContentTransScale() { return m_contentTransScale; }
    [[nodiscard]] float getZoom() const { return m_contentTransScale.x; }
    glm::vec2&          getParentContentScale();
    float               getFixAspect() const { return m_fixAspect; }

    void                     setVaoOffset(GLuint v) { m_indDrawBlock.vaoOffset = v; }
    GLuint                   getVaoOffset() const { return m_indDrawBlock.vaoOffset; }
    std::vector<DivVaoData>& getDivData() { return m_indDrawBlock.vaoData; }
    virtual void             pushVaoUpdtOffsets() {}
    virtual void             clearDs() { m_indDrawBlock.drawSet = nullptr; }

    static void limitDrawVaoToBounds(std::vector<DivVaoData>::iterator& dIt, glm::vec2& size, glm::vec2& uvDiff,
                                     glm::vec4& scIndDraw, glm::vec4& vp);
    static void limitTexCoordsToBounds(float* tc, int32_t stdQuadVertInd, const glm::vec2& tvSize, const glm::vec2& uvSize);

    virtual glm::mat4* getContentMat(bool excludedFromParentContentTrans = false, bool excludedFromPadding = false);
    virtual glm::mat4* getFlatContentMat(bool excludedFromParentContentTrans = false, bool excludedFromPadding = false);

    /** relative float values */
    virtual void setContentTransScale(float x, float y);
    /** in pixels */
    virtual void setContentTransTransl(float x, float y);
    virtual void setContentRotation(float angle, float ax, float ay, float az);
    void         setContentTransCentered(bool val) { m_contTransMatCentered = val; }

    virtual void updateMatrix();
    virtual void calcContentTransMat();

    void               excludeFromPadding(bool val) { m_excludeFromPadding = val; }
    [[nodiscard]] bool isExcludedFromPadding() const { return m_excludeFromPadding; }
    void               excludeFromParentViewTrans(bool val) { m_excludeFromParentContentTrans = val; }
    [[nodiscard]] bool isExcludedFromParentContentTrans() const { return m_excludeFromParentContentTrans; }
    void               excludeFromStyles(bool val) { m_excludeFromStyles = val; }
    void               excludeFromOutOfBorderCheck(bool val) { m_skipBoundCheck = val; }

    void setDrawFlag();
    void setSelectedCb(std::function<void(bool)> f) { m_selectedCb = std::move(f); }
    void setDrawInmediate(bool val) { m_drawImmediate = val; }
    void setAlpha(float val) { m_alpha = val; setChanged(true); }
    void setShaderCollector(ShaderCollector* shCol) { m_shCol = shCol; }
    void setGlBase(GLBase* glbase) { m_glbase = glbase; }

    // state - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool  isSelected() { return m_state == state::selected; }
    state getState() { return m_state; }
    state getLastState() { return m_lastState; }
    void  setState(state st);

    // glbase - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void            runOnMainThread(const std::function<bool()>& func, bool forcePush = false);
    WindowManager*  getWinMan() { return m_glbase->getWinMan(); }
    void            addGlCbSync(const std::function<bool()>& func) const;

    // styles - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void setStyleInitVal(const std::string& name, std::string val, state st = state::m_state) {
        m_styleCustDefs[st == state::m_state ? m_state : st][name] = std::move(val);
    }
    virtual void loadStyleDefaults();
    virtual void rebuildCustomStyle();
    virtual void updateStyleIt(ResNode* node, state st, std::string& styleClass);
    virtual void updateStyle();
    void         addStyleClass(std::string* styleClass) { addStyleClass(std::move(*styleClass)); }
    virtual void addStyleClass(std::string&& styleClass);
    virtual void clearStyles();
    virtual void applyStyle();
    std::string& getStyleClass() { return m_baseStyleClass; }
    bool         getStyleChanged() const { return m_styleChanged; }
    bool         getStyleClassInited() const { return m_styleClassInited; }

    // Resources ---------------------------------------------------------------
    ResNode* getStyleResNode();

    // Bounding Box
    bool setScissorChildren(bool on_off) { return (m_ScissorChildren = on_off); }

    /** get the bounding box around the children in parent relative coordinates
     * (without the parent's transformation matrix) */
    glm::vec4& getChildrenBoundBox() { return m_childBoundBox; }
    /** get the bounding box around the children in parent relative coordinates
     * (without the parent's transformation matrix) plus padding  */
    glm::vec2 getChildrenBBSizeWithPadd() {
        return {m_childBoundBox.z - m_childBoundBox.x + getPadding().x + getPadding().z,
                m_childBoundBox.w - m_childBoundBox.y + getPadding().y + getPadding().w};
    }

    virtual bool rec_ChildrenBoundBox(glm::vec4& ref);
    bool         isOutOfParentBounds() {
        if (!m_parent || m_skipBoundCheck) {
            return false;
        }
        return glm::any(glm::greaterThan(m_parentNodeRelPos, m_parent->m_size)) ||
               glm::any(glm::lessThan(m_parentNodeRelPos + m_size, m_zeroVec));
    }

    // util ------------------------------------------------------------------

    void util_FillRect(int32_t x, int32_t y, int32_t cx, int32_t cy, float cr = 1.f, float cg = 1.f, float cb = 1.f, float ca = 1.f,
                       Shaders* shdr = nullptr, Quad* quad = nullptr);
    void util_FillRect(int32_t x, int32_t y, int32_t cx, int32_t cy, float* color, Shaders* shdr = nullptr, Quad* quad = nullptr);
    void util_FillRect(glm::ivec4& r, float* color, Shaders* shdr = nullptr, Quad* quad = nullptr);

    static std::string& getCustomDefName() { return m_customDefName; }

    MouseIcon ui_MouseIcon = MouseIcon::arrow;
    /** the node's position relative to the window in pixels */
    glm::vec2              m_winRelPos  = glm::vec2{0.f};
    glm::vec2              m_winRelSize = glm::vec2{0.f};
    std::list<std::string> m_styleTree;

    std::unordered_map<state, std::unordered_map<styleInit, std::function<void()>>> m_setStyleFunc;

    // generic iteration function for calculations on parts of the node-tree
    static void itrNodes(const std::unique_ptr<UINode>& node, void* result,
                         const std::function<void*(const std::unique_ptr<UINode>&, void*)>& f) {
        result = f(node, result);
        for (auto& it : node->getChildren()) {
            UINode::itrNodes(it, result, f);
        }
    }

    static void itrNodes(const std::unique_ptr<UINode>& node, const std::function<void(const std::unique_ptr<UINode>&)>& f) {
        f(node);
        for (auto& it : node->getChildren()) {
            UINode::itrNodes(it, f);
        }
    }

    static void itrNodes(UINode* node, const std::function<void(UINode*)>& f) {
        f(node);
        for (auto& it : node->getChildren()) {
            UINode::itrNodes(it.get(), f);
        }
    }

    class ObjPosIt {
    public:
        std::vector<std::unique_ptr<UINode>>::iterator              it;
        std::list<std::vector<std::unique_ptr<UINode>>::iterator>   parents;
        std::vector<std::unique_ptr<UINode>>*                       list           = nullptr;
        UINode*                                                     foundNode      = nullptr;
        uint32_t                                                    foundId        = 0;
        int32_t                                                     foundTreeLevel = -1;
        int32_t                                                     treeLevel      = 0;
        std::list<UINode*>                                          localTree;
        glm::vec2                                                   pos{};
        hidEvent                                                    event{};
    };

    virtual bool isInBounds(glm::vec2& pos);
    static bool contains(UINode* outer, UINode* node);
    static bool objPosIt(ObjPosIt& opi);

    std::function<bool(ObjPosIt&)> m_outOfTreeObjId;
    IndDrawBlock                   m_indDrawBlock;

protected:
    bool getNodeIt(UINode* node, UINode** fn, const std::string& name) {
        if (node->m_name == name) {
            *fn = node;
            return true;
        } else {
            for (auto& it : node->m_children) {
                if (getNodeIt(it.get(), fn, name)) {
                    return true;
                }
            }
            return false;
        }
    }

    virtual void dumpIt(UINode* node, int32_t* depth, bool dumpLocalTree);

    Quad*                                   m_quad   = nullptr;
    GLBase*                                 m_glbase = nullptr;
    Shaders*                                m_shdr   = nullptr;
    ShaderCollector*                        m_shCol  = nullptr;
    ObjectMapInteraction*                   m_objSel = nullptr;
    std::vector<std::unique_ptr<UINode>>    m_children;
    std::function<void()>                   m_returnFocusCb;
    scissorStack                            m_scissorStack;

    uint32_t m_objIdMin   = 0;
    uint32_t m_objIdMax   = 0;

    UINode* m_parent             = nullptr;
    glm::vec4  m_viewPort        = glm::vec4(0.f);
    glm::vec2  m_parentContScale = glm::vec2(1.f, 1.f);  // contentTransMat scaling part
    glm::mat4* m_orthoMat        = nullptr;

    state m_state     = state::none;
    state m_lastState = state::none;

    bool m_hasInputFocus                 = false;
    bool m_hasDepth                      = false;
    bool m_blockHID                      = false;
    bool m_geoChanged                    = true;
    bool m_inited                        = false;
    bool m_forceInit                     = false;
    bool m_focusAllowed                  = true;
    bool m_visible                       = true;
    bool m_canReceiveDrag                = false;
    bool m_excludeFromObjMap             = false;
    bool m_excludeFromPadding            = false;
    bool m_excludeFromParentScissoring   = false;
    bool m_excludeFromParentContentTrans = false;
    bool m_excludeFromStyles             = false;
    bool m_styleChanged                  = false;
    bool m_styleClassInited              = true;
    bool m_drawParamChanged              = false;
    bool m_treeChanged                   = false;  /// a child was added or removed
    bool m_reqTreeChanged                = false;
    bool m_drawImmediate                 = true;
    bool m_isOutOfParentBounds           = false;
    bool m_oob                           = false;
    bool m_skipBoundCheck                = false;
    bool m_hasContRot                    = false;
    bool m_referenceDrawing              = false;

    uint32_t  m_borderWidth  = 0;
    uint32_t  m_borderRadius = 0;
    uint32_t  m_borderAlias  = 1;
    glm::vec2 m_borderRadiusRel{0};
    glm::vec2 m_borderAliasRel{0};

    /** all parent's content * contentTransformation matrices -> the flattened
     * matrixStack at this position in the scene graph. */
    glm::mat4* m_parentMat       = nullptr;
    glm::mat4  m_parentMatLocCpy = glm::mat4(1.f);
    glm::mat4  m_identMat        = glm::mat4(1.f);

    int32_t m_widthInt    = 0;
    int32_t m_heightInt   = 1;
    float   m_widthFloat  = 1.f;
    float   m_heightFloat = 1.f;

    bool  m_updating        = false;
    float m_fixAspect       = -1.f;
    float m_aspect          = 1.f;
    float m_alpha           = 1.f; /// relative alpha of this node
    float m_absoluteAlpha   = 1.f; /// "flat" absolute alpha, top-down multiplied values

    int32_t m_posXInt   = 0;
    int32_t m_posYInt   = 0;
    float   m_posXFloat = 0;
    float   m_posYFloat = 0;
    float   m_zPos      = 1.f;

    unitType m_posXType   = unitType::Pixels;
    unitType m_posYType   = unitType::Pixels;
    unitType m_widthType  = unitType::Percent;
    unitType m_heightType = unitType::Percent;

    align  m_alignX = align::left;
    valign m_alignY = valign::top;

    pivotX m_pivX = pivotX::left;
    pivotY m_pivY = pivotY::top;

    glm::vec4 m_padding{0.f};
    glm::vec3 m_init_size{1.f};
    glm::vec3 m_work_size{1.f};

    /** the node's size relative to it's parent in normalized coordinates (to be
     * passed to a shader to render the node) */
    glm::vec2 m_relSize{0};
    /** the node's size in pixels (including padding and border) */
    glm::vec2 m_size{0};
    /** the node's content size in pixels (without padding and border) */
    glm::vec2 m_contentSize{0};
    /** the node's content offset in pixels (padding + border) */
    glm::vec2 m_contentOffset{0};

    /** bounding box around all children in format left/top x,y and right/bottom x.y */
    glm::vec4 m_childBoundBox{0};

    /** the position relative to the parent's (transformed) content's top left corner in pixels */
    glm::vec2 m_pos{0.f};
    /** the position relative to the parent's node's top left corner in pixels
     * (ignoring border and padding) */
    glm::vec2 m_parentNodeRelPos{0.f};
    /** the content's position relative to window's top left corner in pixels */
    glm::vec2 m_contWinPos{0.f};
    /** the node's position in parent contentTransMat space */
    glm::vec2 m_parentTransPos{0.f};

    /** the node's window relative matrix. */
    glm::mat4 m_winRelMat = glm::mat4(1.f);
    /** to be passed down to the children if the node is excluded from it's
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
    glm::vec3 m_contentTransMatTransl{0.f};
    glm::vec4 m_contentTransRotate{1.f, 0.f, 0.f, 0.f};
    glm::mat4 m_contentTransMat    = glm::mat4(1.f);
    glm::mat4 m_contentTransMatRel = glm::mat4(1.f);
    glm::mat4 m_contRot            = glm::mat4(1.f);
    glm::mat4 m_nodePosMat         = glm::mat4(1.f);
    glm::mat4 m_nodePosMatHw       = glm::mat4(1.f);  // in hw pixels
    glm::vec2 m_contentCenterOffs{0.f};               // in hw pixels
    /** if false, offset and scaling is done relative to 0|0 top left origin, if
     * true relative to the center of the UINode **/
    bool m_contTransMatCentered = false;

    /** the nodes transformed content matrix, multiplied with the matrix stack */
    glm::mat4 m_flatContentTransMat = glm::mat4(1.f);

    /** the node's viewport (x,y,width,height) in window relative pixels */
    glm::vec4 m_parentContVp{1.f};
    glm::vec2 m_borderWidthRel{0.001f, 0.001f};
    glm::vec4 m_color{0.0};
    glm::vec4 m_bgColor{0.0};
    glm::vec4 m_borderColor{0.0};
    glm::vec2 m_uvDiff{0.f};

    std::function<void()>     m_changeCb;
    std::function<void(bool)> m_selectedCb;
    std::string               m_name;

    std::string                                                             m_baseStyleClass;
    std::string                                                             m_custDefStyleSheet = {0};
    std::unordered_map<state, std::unordered_map<std::string, std::string>> m_styleCustDefs;
    std::unique_ptr<ResNode>                                                m_customStyleNode;

    UISharedRes* m_sharedRes = nullptr;
    UniformBlock m_uniBlock;

    StopWatch m_watch;
    StopWatch m_drawWatch;
    glm::vec4 p[2]{glm::vec4{}, glm::vec4{}};
    glm::vec2 pN[2]{glm::vec2{}, glm::vec2{}};
    glm::vec4 m_scissVp{};

    // temp value for scissoring
    glm::vec4 m_sc{};
    glm::vec4 m_scIndDraw{};
    glm::vec4 m_bb{};

    std::string m_fontType;

    // callbacks ->
    std::unordered_map<hidEvent, std::list<mouseCb>> m_mouseHidCb;

    std::unordered_map<state, std::function<void(hidData*)>>                  m_mouseInCb;
    std::unordered_map<state, std::function<void(hidData*)>>                  m_mouseOutCb;
    std::unordered_map<void*, std::shared_ptr<std::function<void(std::any)>>> m_onValChangedCb;
    std::function<void(hidData*)>                                             m_keyDownCb;
    std::function<void(hidData*)>                                             m_keyUpCb;

    std::function<void()> m_onFocusedCb;
    std::function<void()> m_onLostFocusCb;

    std::filesystem::path m_filepath;

    bool m_ScissorChildren = false;

    std::unordered_map<std::string, std::any> m_styleInitVals;

    static inline std::string m_customDefName = "__custom_Default";
    static inline glm::vec2   m_zeroVec{0.f};
    static inline glm::vec2   m_oneVec{1.f};
    static inline glm::vec2   m_objItLT{0.f};
    static inline glm::vec2   m_objItRB{0.f};
};

}  // namespace ara
