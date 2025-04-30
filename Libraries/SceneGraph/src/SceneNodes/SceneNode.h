//
//  SceneNode.h
//

#pragma once

#include <Shaders/ShaderUtils/MaterialProperties.h>
#include <Shaders/Shaders.h>
#include <Utils/TFO.h>
#include <Utils/Texture.h>
#include <WindowManagement/WindowBase.h>

#include "GLUtils/sceneData.h"

namespace ara {
class SceneNode;
class CameraSet;
class ShaderProto;
class sceneData;

class SNIdGroup {
public:
    SNIdGroup() = default;
    SNIdGroup(SceneNode* o) { owner = o; }
    SceneNode*                          owner = nullptr;
    std::unordered_map<SceneNode*, int> ids;
};

class SceneNode {
public:
    typedef std::function<bool(SceneNode*)>             itNodeCbFunc;
    typedef std::function<bool(SceneNode*, SceneNode*)> itNodeParentCbFunc;

    explicit SceneNode(sceneData* sd = nullptr);
    virtual ~SceneNode() = default;

    virtual void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);
    virtual void postDraw(double time, double dt, CameraSet* cs, Shaders* shader, TFO* tfo = nullptr) {}
    virtual void update(double time, double dt, CameraSet* cs);
    virtual void assignTexUnits(Shaders* shader);
    virtual void useTextureUnitInd(int unit, int ind, Shaders* shader = nullptr, TFO* tfo = nullptr);

    virtual void onKey(int key, int scancode, int action, int mods) {}

    virtual void       setParentNode(SceneNode* _parent);
    virtual SceneNode* addChild(bool updtNodeIds = true);
    virtual SceneNode* addChild(std::unique_ptr<SceneNode>&& _newScene, bool updtNodeIds = false);

    template <class T>
    T* addChild(bool updtNodeIds = false) {
        return static_cast<T *>(addChild(std::make_unique<T>(s_sd), updtNodeIds));
    }

    virtual void addChildRef(SceneNode* newNode,
                             bool       updtNodeIds = true);  ///< ownership stays outside of this Node and
                                                        ///< has to be delete outside
    virtual SceneNode* insertChild(uint ind, bool updtNodeIds = true);
    virtual SceneNode* insertChild(uint ind, std::unique_ptr<SceneNode> newScene, bool updtNodeIds = true);

    template <class T>
    T* insertChild(uint ind, bool updtNodeIds = false) {
        return static_cast<T *>(insertChild(ind, std::make_unique<T>(s_sd), updtNodeIds));
    }

    virtual void       clearChildren();
    virtual void       removeChild(SceneNode* node);
    virtual void       removeChild(const std::string& name);
    virtual void       removeChildDontKill(SceneNode* node);
    bool               hasParentNode() const { return !m_parents.empty(); }
    virtual SceneNode* getParentNode(int objId);
    SceneNode*         getFirstParentNode() const { return !m_parents.empty() ? (*m_parents.begin()) : nullptr; }

    std::list<SceneNode*>*                   getParents() { return &m_parents; }
    virtual SceneNode*                       getRootNode();
    std::vector<SceneNode*>*                 getChildren() { return &m_children; }
    std::vector<std::unique_ptr<SceneNode>>* getIntChildren() { return &m_int_children; }
    std::string&                             getName() { return m_name; }
    virtual SceneNode*                       getNode(const std::string& searchName);
    virtual glm::vec3&                       getCenter();

    virtual bool findChild(SceneNode* node);

    virtual bool iterateNode(SceneNode* node, const itNodeCbFunc& cbFunc);
    virtual bool iterateNodeParent(SceneNode* node, SceneNode* parent, const itNodeParentCbFunc& cbFunc);
    virtual uint regenNodeIds(uint idOffs);

    virtual void translate(float x, float y, float z);
    virtual void translate(const glm::vec3& transVec);
    virtual void rotate(float angle, float x, float y, float z);  ///< Angles in Radians, Axis must be normalized
    virtual void rotate(float angle, const glm::vec3& rotVec);  ///< Angles in Radians, Axis must be normalized
    virtual void rotate(glm::mat4& rot);
    virtual void scale(float x, float y, float z);
    virtual void scale(const glm::vec3& scaleVec);

    void rebuildModelMat() {
        if (!m_parents.empty()) rebuildModelMat(*m_parents.begin());
    }
    virtual void rebuildModelMat(SceneNode* parent);
    virtual void setModelMat(glm::mat4& mm);

    virtual void setSingleId(bool val);  ///< if true, add all children to an idGroup and treat them
                                         ///< as a single object during picking
    SNIdGroup* createIdGroup() {
        if (m_idGroup) m_idGroup.reset();
        m_idGroup = std::make_unique<SNIdGroup>(this);
        return m_idGroup.get();
    }
    SNIdGroup*  getIdGroup() const { return m_idGroup ? m_idGroup.get() : nullptr; }
    void        setIdGroup(SNIdGroup* gr) { m_extIdGroup = gr; }
    SNIdGroup*  getExtIdGroup() const { return m_extIdGroup; }
    void        addPar(const std::string& cmd, float* ptr) { m_par[cmd] = ptr; }
    void        setPar(const std::string* cmd, float val) {
        if (m_par.contains(*cmd)) {
            *m_par[*cmd] = val;
        }
    }

    virtual bool setSelected(bool val, SceneNode* parent = nullptr, bool procCb = true);
    virtual void setParentModelMat(SceneNode* parent, glm::mat4& inMat);
    virtual void setBoundingBox(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);
    virtual void setBoundingBox(glm::vec3* boundMin, glm::vec3* boundMax);

    void addRemoveCb(void* ptr, const std::string& name, std::function<void()> cb) {
        s_removeCb[ptr][name] = std::move(cb);
    }
    void addTexture(std::unique_ptr<Texture> newTex) { m_textures.emplace_back(std::move(newTex)); }
    void setVisibility(bool val) { m_visible = val; }
    void setBlendMode(GLenum src, GLenum dst) {
        m_blendSrc = src;
        m_blendDst = dst;
    }
    void setMaterial(const MaterialProperties* material) { m_material = *material; }
    void setHasModelSN(bool val) { m_hasModelSN = val; }
    bool getHasModelSN() const { return m_hasModelSN; }
    void setGridNrSteps(int x, int y) {
        m_gridNrSteps.x = static_cast<float>(x);
        m_gridNrSteps.y = static_cast<float>(y);
    }
    glm::vec2& getGridNrSteps() { return m_gridNrSteps; }
    void       setGridBgAlpha(float val) { m_gridBgAlpha = val; }
    void       setGridLineThick(float val) { m_gridLineThick = val; }
    void       setExcludeFromObjMap(bool val) { m_excludeFromObjMap = val; }
    void       setPolyFill(bool val) { m_polyFill = val; }
    void       setWinding(GLenum val) { m_winding = val; }
    void       setUseNormalizedTexCoord(GLenum val) { m_useNormalizedTexCoord = val; }
    void       setFlipTc(const glm::ivec2& val) { m_flipTc = val; }
    void       setFlipTc(int idx, int val) { m_flipTc[idx] = val; }
    void       setInvertTc(const glm::ivec2& val) { m_invertTc = val; }
    void       setInvertTc(int idx, int val) { m_invertTc[idx] = val; }
    void       setName(const char* name) { m_name = std::string(name); }
    void       setName(std::string name) { m_name = std::move(name); }
    void       setRootNode(SceneNode* rootNode) { m_rootNode = rootNode; }
    void       setScene(WindowBase* scene) { m_scene = scene; }
    void       setFixGizmoRot(bool val) { m_FixGizmoRot = val; }
    void       setExtTex(GLuint texId) { m_extTexId = texId; }
    void       setExtTex1(GLuint texId) { m_extTexId1 = texId; }
    void       setExtTex2(GLuint texId) { m_extTexId2 = texId; }
    void       setDrawCb(std::function<void(double)> f) { m_drawEndCb = std::move(f); }
    void       setIsYuv(bool val) { m_isYuv = val; }
    void       setIsNv12(bool val) { m_isNv12 = val; }
    void       setLumaKey(bool val) { m_lumaKey = val; }
    void       setLumaThres(float smthstpLow, float smthstpHigh) {
        m_lumaKeySSLow  = smthstpLow;
        m_lumaKeySSHigh = smthstpHigh;
    }
    void setAllowDragSelect(bool val) { m_dragSelectAllowed = val; }
    void setSceneData(sceneData* sd) {
        s_sd     = sd;
        m_glbase = sd->glbase;
    }
    void setGLBase(GLBase* glbase) { m_glbase = glbase; }

    virtual uint       getNrSubNodes();
    virtual bool       isSelected(SceneNode* parent = nullptr);
    virtual glm::mat4* getModelMat();
    virtual glm::mat3* getNormalMat();
    virtual int        getObjId(SceneNode* parent = nullptr);
    virtual SceneNode* getNodeWithID(int id);

    bool                                   isVisible() const { return m_visible; }
    bool                                   isFixGizmoRot() const { return m_FixGizmoRot; }
    bool                                   dragSelectedAllowed() const { return m_dragSelectAllowed; }
    glm::vec3&                             getDimension() { return m_dimension; }
    float                                  getRotAngle() const { return m_rotAngle; }
    glm::vec3&                             getRotAxis() { return m_rotAxis; }
    glm::vec3&                             getTransVec() { return m_transVec; }
    glm::mat4&                             getRotMat() { return m_rotMat; }
    glm::vec3&                             getScalingVec() { return m_scaleVec; }
    MaterialProperties*                    getMaterial() { return &m_material; }
    glm::mat4&                             getModelMat(SceneNode* parent) { return m_absModelMat[parent]; }
    glm::mat3&                             getNormalMat(SceneNode* parent) { return m_absNormalMat[parent]; }
    glm::mat4&                             getRelModelMat() { return modelMat; }
    glm::vec3&                             getBoundingBoxMin() { return m_boundingBoxMin; }
    glm::vec3&                             getBoundingBoxMax() { return m_boundingBoxMax; }
    std::vector<std::unique_ptr<Texture>>* getTextures() { return &m_textures; }
    WindowBase*                            getScene() const { return m_scene; }
    sceneData*                             getSceneData() const { return s_sd; }
    bool                                   isPolyFilled() const { return m_polyFill; }
    bool                                   excludeFromObjMap() const { return m_excludeFromObjMap; }
    void pushClickCb(void* ptr, std::function<void()> func) { m_clickCb[ptr] = std::move(func); }
    void deleteClickCb(void* ptr) {
        m_clickCb.erase(ptr);
    }

    /** called when the model Matrix of this node changes */
    void pushModelMatChangedCb(void* ptr, const std::function<bool(SceneNode*)>& func) {
        m_modelMatChangedCb[ptr] = func;
    }
    void deleteModelMatChangedCb(void* ptr) {
        m_modelMatChangedCb.erase(ptr);
    }

    virtual void deleteGarbage();
    virtual void unregister();
    virtual void dumpTreeIt(SceneNode* tNode, uint level);
    virtual void dump();

public:
    std::shared_ptr<VAO> m_vao;

    uint64_t      m_nameFlag  = 0;
    uint64_t      m_transFlag = 0;
    sceneNodeType m_nodeType  = GLSG_SNT_STANDARD;

    bool m_drawGridTex       = false;
    bool m_visible           = false;
    bool m_selectable        = true;
    bool m_hasNewModelMat    = true;
    bool m_hasBoundingBox    = false;
    bool m_cullFace          = true;
    bool m_depthTest         = true;
    bool m_drawIndexed       = false;
    bool m_dragSelectAllowed = false;
    bool m_emptyDrawFunc     = false;
    bool m_usefixedModelMat  = false;

    float m_staticNDCSize = 0.f;
    float m_maxNDCSize    = 0.f;

    std::vector<GLuint>                              m_indices;
    glm::vec2                                        m_minTexC         = glm::vec2{0.f};
    glm::vec2                                        m_maxTexC         = glm::vec2{1.f};
    std::atomic<bool>                                m_calcMatrixStack = false;
    std::unordered_map<SceneNode*, int>              m_skipForCamInd;
    std::unordered_map<SceneNode*, int>              m_nodeObjId;
    std::unordered_map<SceneNode*, bool>             m_selected;
    std::array<std::string, GLSG_NUM_RENDER_PASSES>  m_protoName;
    std::array<ShaderProto*, GLSG_NUM_RENDER_PASSES> m_cachedCustShdr = {nullptr};

protected:
    WindowBase*           m_scene = nullptr;
    std::list<SceneNode*> m_parents;
    SceneNode*            m_rootNode = nullptr;
    GLBase*               m_glbase   = nullptr;
    sceneData*            s_sd       = nullptr;

    MaterialProperties m_material;

    std::map<int, SceneNode*>     m_sceneNodeMap;
    std::map<std::string, float*> m_par;
    std::map<renderPass, bool>    m_renderPassEnabled;

    std::vector<auxTexPar>                  m_auxTex;
    std::vector<std::unique_ptr<SceneNode>> m_int_children;  ///< ownerships stays inside the SceneNode instance and
                                                             ///< gets freed here
    std::vector<SceneNode*> m_children;                      ///< unified array of internal and external sceneNodes, the
                                                             ///< pointer inside this array will not be deleted by the
                                                             ///< sceneNodes destructor
    std::vector<SceneNode*>               m_sceneNodesToKill;
    std::vector<std::unique_ptr<Texture>> m_textures;

    std::map<void*, std::function<bool(SceneNode*)>> m_modelMatChangedCb;
    std::list<std::function<void()>>                 m_changedParentCb;
    std::map<void*, std::function<void()>>           m_clickCb;

    std::map<SceneNode*, glm::mat4> m_parentModelMat;  ///< Data Matrix of the parent node
    std::map<SceneNode*, glm::mat4> m_absModelMat;     ///< absolute Data Matrix, this is used for rendering
    std::map<SceneNode*, glm::mat3> m_absNormalMat;    ///< absolute Normal Matrix, this is used for rendering

    glm::mat4 modelMat    = glm::mat4(1.f);  ///< relative (to its parent) Data Matrix of this node
    glm::mat4 m_transMat  = glm::mat4(1.f);  ///< relative translation Matrix of this Node
    glm::mat4 m_rotMat    = glm::mat4(1.f);  ///< relative rotation Matrix of this Node
    glm::mat4 m_newRotMat = glm::mat4(1.f);  ///< relative rotation Matrix of this Node
    glm::mat4 m_scaleMat  = glm::mat4(1.f);  ///< relative scaling Matrix of this Node
    glm::mat4 m_pvm       = glm::mat4(1.f);
    glm::mat4 m_tempCamProjViewMat;

    glm::vec4 m_tempProj2DVecStart{0.f};
    glm::vec4 m_tempProj2DVecEnd{0.f};
    glm::vec4 m_tempDiffVec{0.f};
    glm::vec3 m_boundingBoxMin{0.f};
    glm::vec3 m_boundingBoxMax{1.f};
    glm::vec3 m_vaoDimension{0};  ///> the dimension of the Nodes Mesh as it was
                                  /// calculated by the BoundingBox Shader
    glm::vec3 m_dimension{0};     ///> the actual dimension of the Node, that means its Meshes
                                  /// dimension multiplied by the actual scaling
    glm::vec3 m_transVec{0};
    glm::vec3 m_scaleVec{1.f};
    glm::vec3 m_rotAxis{1.f, 0.f, 0.f};
    glm::vec3 m_center{0.f};
    glm::vec2 m_gridNrSteps{10.f};
    glm::ivec2 m_flipTc{0};
    glm::ivec2 m_invertTc{0};

    float m_gridBgAlpha   = 1.f;
    float m_gridLineThick = 2.f;
    float m_rotAngle      = 0.f;
    float m_lumaKeySSLow  = 0.25f;
    float m_lumaKeySSHigh = 0.3f;

    bool m_hasModelSN            = false;
    bool m_excludeFromObjMap     = false;
    bool m_polyFill              = true;
    bool m_useNormalizedTexCoord = false;
    bool m_FixGizmoRot           = false;
    bool m_recalcCenter          = true;
    bool m_isYuv                 = false;
    bool m_isNv12                = false;
    bool m_lumaKey               = false;

    GLenum m_blendSrc      = GL_SRC_ALPHA;
    GLenum m_blendSrcAlpha = GL_ONE;
    GLenum m_blendDst      = GL_ONE_MINUS_SRC_ALPHA;
    GLenum m_blendDstAlpha = GL_ONE;
    GLenum m_winding       = GL_CCW;

    GLuint m_extTexId  = 0;
    GLuint m_extTexId1 = 0;
    GLuint m_extTexId2 = 0;

    std::string m_name;

    std::unique_ptr<SNIdGroup> m_idGroup;
    SNIdGroup*                 m_extIdGroup = nullptr;

    std::map<void*, std::unordered_map<std::string, std::function<void()>>> s_removeCb;

    std::function<void(double)> m_drawEndCb;
};

}  // namespace ara