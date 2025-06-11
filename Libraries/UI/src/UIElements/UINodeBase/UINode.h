//
// Created by user on 5/5/25.
//

#pragma once

#include <UIElements/UINodeBase/UINodeHID.h>
#include <Utils/UniformBlock.h>
#include <Property.h>
#include <ListProperty.h>
#include <UICommon.h>

// avoid including too many headers when using this class
#include <ObjectMapInteraction.h>
#include <DrawManagers/DrawManager.h>


namespace ara {

class ObjectMapInteraction;
class ShaderCollector;
class Shaders;
class Quad;
class UIApplication;
class UIWindow;
class WindowManager;

class UINode : public UINodeHID {
public:
    UINode();
    ~UINode() override;

    virtual void init() {}

    template <typename T, typename... Args>
    T* addChild(Args&& ... args) {
        return static_cast<T*>(UINode::addChild(std::make_unique<T>(args...)));
    }

    template <typename T, typename U>
    requires std::integral<U> || std::floating_point<U>
    T* addChild(U x, U y, U w, U h) {
        T* nc = addChild<T>();
        nc->setPos(x, y);
        nc->setSize(w, h);
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h, const float* fgcolor, const float* bkcolor) {
        T*    nc = addChild<T>();
        auto* n  = static_cast<UINode*>(nc);
        n->setPos(x, y);
        n->setSize(w, h);
        if (fgcolor != nullptr) {
            n->setColor(fgcolor[0], fgcolor[1], fgcolor[2], fgcolor[3]);
        }
        if (bkcolor != nullptr) {
            n->setBackgroundColor(bkcolor[0], bkcolor[1], bkcolor[2], bkcolor[3]);
        }
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h, glm::vec4 fgcolor, glm::vec4 bkcolor) {
        T*    nc = addChild<T>();
        auto* n  = static_cast<UINode*>(nc);
        n->setPos(x, y);
        n->setSize(w, h);
        n->setColor(fgcolor);
        n->setBackgroundColor(bkcolor);
        return nc;
    }

    template <typename T>
    T* addChild(int32_t x, int32_t y, int32_t w, int32_t h, std::filesystem::path path) {
        T*    nc = addChild<T>();
        auto* n  = static_cast<UINode*>(nc);
        n->setPos(x, y);
        n->setSize(w, h);
        n->setPath(std::move(path));
        return nc;
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
    T* insertChild(int32_t position, const std::string& styleClass) {
        T* nc = UINode::insertChild<T>(position);
        nc->addStyleClass(styleClass);
        return nc;
    }

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
        if (prop.ptr) {
            prop.onPreChange(m_onValChangedCb[prop.ptr]);
        }
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

    virtual UINode* addChild(std::unique_ptr<UINode>&& child);
    virtual UINode* insertChild(int32_t position, std::unique_ptr<UINode>&& child);
    virtual UINode* insertChild(const std::string& name, std::unique_ptr<UINode>&& child);
    virtual UINode* insertAfter(const std::string& name, std::unique_ptr<UINode>&& child);
    virtual void moveChildTo(int32_t position, UINode*);
    virtual void remove_child(UINode* node);
    virtual void clearChildren();
    virtual void removeGLResources() {}

    void initChild(UINode* child, UINode* parent);

    /** draw the scenegraph from this node onwards */
    virtual void drawAsRoot(uint32_t& objId);

    /** the scenegraph drawing iteration loop */
    virtual void drawIt(scissorStack& ss, uint32_t& objId, bool treeChanged, bool& skipFirst);
    virtual bool draw(uint32_t& objId) { return true; }
    virtual bool drawIndirect(uint32_t& objId) { return true; }
    virtual void updateDrawData() {}
    virtual void updtMatrIt(scissorStack* ss);

    void updateMatrix() override;
    void checkInit();
    void checkStyles();
    void immediateScissoring(scissorStack* ss);
    void checkScissoring(scissorStack* ss);
    void getParentViewport();
    void setPositionAndSize();
    void setAlignment();

    virtual void        pushVaoUpdtOffsets() {}
    void                applyStyle() override;

    void                setChanged(bool val) override;
    void                setChangeCb(std::function<void()> f) { m_changeCb = std::move(f); }
    void                setHIDBlocked(bool val) override;
    bool                removeFocus() override;
    void                setId(uint32_t val) { m_objIdMin = val; m_objIdMax = val; }
    void                setMinId(uint32_t val) { m_objIdMin = val; }
    void                setMaxId(uint32_t val) { m_objIdMax = val; }
    void                reqTreeChanged(bool val) { m_reqTreeChanged = true; }
    virtual void        setParent(UINode* parent) { m_parent = parent; }
    virtual void        setRefDraw(bool v) { m_referenceDrawing = true; }
    void                setName(std::string name) { m_name = std::move(name); }
    virtual void        setValue(float val) {}
    virtual void        setAbsValue(float val) {}
    virtual void        setPath(std::filesystem::path file) { m_filepath = std::move(file); }
    [[nodiscard]] bool  isVisible() const { return m_visible; }
    [[nodiscard]] bool  isInited() const { return m_inited; }
    void                setInited(bool val) { m_inited = val; }
    void                setForceInit(bool val) { m_forceInit = val; }
    void                excludeFromObjMap(bool val) { m_excludeFromObjMap = val; }
    [[nodiscard]] bool  isExcludedFromObjMap() const { return m_excludeFromObjMap; }
    void                setViewport(float x, float y, float width, float height) override;
    void                setViewport(glm::vec4* viewport) override { UINodeGeom::setViewport(viewport); }
    void                setVisibility(bool val, state st = state::m_state) override;
    void                setDrawFlag() const;
    void                setHasDepth(bool val) { m_hasDepth = val; }
    void                setDrawInmediate(bool val) { m_drawImmediate = val; }
    void                setShaderCollector(ShaderCollector* shCol) { m_shCol = shCol; }
    void                setSharedRes(UISharedRes* shared) override;
    void                reqUpdtTree() const;
    void                setVaoOffset(GLuint v) { m_indDrawBlock.vaoOffset = v; }
    virtual void        clearDs() { m_indDrawBlock.drawSet = nullptr; }

    auto                    getVaoOffset() const { return m_indDrawBlock.vaoOffset; }
    auto&                   getDivData() { return m_indDrawBlock.vaoData; }
    auto&                   getIndDrawBlock() { return m_indDrawBlock; }
    uint32_t                getMinChildId(uint32_t minId = UINT32_MAX);
    uint32_t                getMaxChildId(uint32_t maxId = 0);
    auto&                   getChildren() { return m_children; }
    [[nodiscard]] auto      getId() const { return m_objIdMin; }
    [[nodiscard]] auto      getMinId() const { return m_objIdMin; }
    [[nodiscard]] auto      getMaxId() const { return m_objIdMax; }
    UINode*                 getRoot();
    UINode*                 getParent() override { return m_parent; }
    UINode*                 getNode(const std::string& name);
    UINode*                 getNodeById(uint32_t searchID);
    virtual float           getValue() { return 0.f; }
    virtual std::string&    getName() { return m_name; }

    [[nodiscard]] bool containsObjectId(uint32_t id) const { return (id >= m_objIdMin && id <= m_objIdMax); }

    virtual uint32_t getSubNodeCount();
    virtual void     getSubNodeCountIt(UINode* node, uint32_t* count);

    static bool objPosIt(ObjPosIt& opi);

    virtual void dump();

    virtual void addGlCb(const std::string&, const std::function<bool()>& f);
    virtual void eraseCb(const std::string&);
    virtual bool hasCb(const std::string&);

    void keyDownIt(hidData& data) override;
    void onCharIt(hidData& data) override;

    std::filesystem::path   dataPath();
    UIApplication*          getApp() const;

    bool isInBounds(glm::vec2& pos) override;
    bool recChildrenBoundBox(glm::vec4& ref);

    void            runOnMainThread(const std::function<bool()>& func, bool forcePush = false) const;
    WindowManager*  getWinMan();
    void            addGlCbSync(const std::function<bool()>& func) const;

    static void limitDrawVaoToBounds(const std::vector<DivVaoData>::iterator& dIt, glm::vec2& size, glm::vec2& uvDiff,
                                     glm::vec4& scIndDraw, glm::vec4& vp);
    static void limitTexCoordsToBounds(float* tc, int32_t stdQuadVertInd, const glm::vec2& tvSize, const glm::vec2& uvSize);

    // generic iteration function for calculations on parts of the node-tree
    static void itrNodes(const std::unique_ptr<UINode>& node, void* result,
                         const std::function<void*(const std::unique_ptr<UINode>&, void*)>& f) {
        result = f(node, result);
        for (const auto& it : node->getChildren()) {
            UINode::itrNodes(it, result, f);
        }
    }

    static void itrNodes(const std::unique_ptr<UINode>& node, const std::function<void(const std::unique_ptr<UINode>&)>& f) {
        f(node);
        for (const auto& it : node->getChildren()) {
            UINode::itrNodes(it, f);
        }
    }

    static void itrNodes(UINode* node, const std::function<void(UINode*)>& f)  {
        f(node);
        for (auto& it : node->getChildren()) {
            UINode::itrNodes(it.get(), f);
        }
    }

    void util_FillRect(glm::ivec2 pos, glm::ivec2 size, glm::vec4 col = {1.f, 1.f, 1.f, 1.f}, Shaders* shdr=nullptr, Quad* quad=nullptr);
    void util_FillRect(glm::ivec2 pos, glm::ivec2 size, float* color, Shaders* shdr = nullptr, Quad* quad = nullptr);
    void util_FillRect(glm::ivec4& r, float* color, Shaders* shdr = nullptr, Quad* quad = nullptr);

protected:
    bool getNodeIt(UINode* node, UINode** fn, const std::string& name);

    std::shared_ptr<DrawManager>            m_drawMan;
    ObjectMapInteraction*                   m_objSel = nullptr;
    std::vector<std::unique_ptr<UINode>>    m_children;

    uint32_t m_objIdMin   = 0;
    uint32_t m_objIdMax   = 0;

    UINode* m_parent = nullptr;

    bool m_inited               = false;
    bool m_forceInit            = false;
    bool m_excludeFromObjMap    = false;
    bool m_treeChanged          = false;  /// a child was added or removed
    bool m_reqTreeChanged       = false;
    bool m_referenceDrawing     = false;

    std::function<void()>     m_changeCb;
    std::string               m_name;

    std::unordered_map<void*, std::shared_ptr<std::function<void(std::any)>>> m_onValChangedCb;

    std::filesystem::path m_filepath;
    virtual void dumpIt(UINode* node, int32_t* depth, bool dumpLocalTree);

    bool                m_drawImmediate    = true;
    IndDrawBlock        m_indDrawBlock;
    UniformBlock        m_uniBlock;
    Shaders*            m_shdr   = nullptr;
    Quad*               m_quad   = nullptr;
    ShaderCollector*    m_shCol  = nullptr;

    bool m_hasDepth = false;

};

}
