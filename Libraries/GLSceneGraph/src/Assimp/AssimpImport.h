#pragma once

#ifdef ARA_USE_ASSIMP

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef __ANDROID__
#include <android_native_app_glue.h>
// #include <AndroidJNIIOSystem.h>
#endif

#include <glsg_common/glsg_common.h>

#include <utility>

#include "Assimp/AssimpData.h"
#include "Conditional.h"

namespace ara {

class GLBase;
class sceneData;
class Scene3DBase;
class SceneNode;
class SNIdGroup;
class WindowBase;

class AssimpImport {
public:
    enum drawMode { MESH_WIREFRAME = 0, MESH_FILL, MESH_POINTS };

    typedef std::function<void(AssimpImport*)> uploadGLCb;

    explicit AssimpImport(GLBase* glbase);
    virtual ~AssimpImport();

    /** singleObjId: if set to true, all imported nodes will have the same
     * object ID  */
    SceneNode* load(Scene3DBase* scene, std::string& filePath, const std::function<void(SceneNode*)>& cb,
                    bool singleObjId = false, bool forceOSLoad = false);
    void load(std::string& filePath, SceneNode* root, std::function<void(SceneNode*)> cb, bool singleObjId = false);
    void loadFromOsFileSys(std::string& filePath, SceneNode* root, std::function<void(SceneNode*)> cb, bool singleObjId = false);
    void loadFromBuffer(std::string* fileBuffer, SceneNode* root, std::function<void(SceneNode*)> cb, bool singleObjId = false);
    virtual void loadThread(SceneNode* root, std::function<void(SceneNode*)> cb, SNIdGroup* idGroup = nullptr, bool optimize = true);

    void        createLightsFromAiModel();
    std::string GetDirectory(std::string& _path);

    bool uploadToGL();
    void initNormShader();

    void setSceneData(sceneData* sd) { s_sd = sd; }
    void setUseGL32(bool val) { m_useGL32 = true; }
    void setScene(WindowBase* scene) { m_scene = scene; }
    void suppressRootTrans(bool val) { m_suppressRootTransform = val; }
    void preTransformVert(bool val) { m_preTransformVertices = val; }
    void setRootScaleFact(glm::vec3& fct) { m_rootScaleFact = fct; }
    void setRootScaleFact(float x, float y, float z) {
        m_rootScaleFact.x = x;
        m_rootScaleFact.y = y;
        m_rootScaleFact.z = z;
    }
    /// euler angles yaw, pitch, roll
    void        setRootRotation(glm::vec3& fct) { m_rootRotateYawPitchRoll = fct; }
    void        setRootBasePoint(glm::vec3& p) { m_rootBasePoint = p; }
    void        setFileBufferExtension(std::string ext) { m_fileBufferExtension = std::move(ext); }
    std::string getFileExtension() { return m_fileExtension; }

    // row-major to column major
    static glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
        glm::mat4 to;
        to[0][0] = (GLfloat)from->a1;
        to[0][1] = (GLfloat)from->b1;
        to[0][2] = (GLfloat)from->c1;
        to[0][3] = (GLfloat)from->d1;
        to[1][0] = (GLfloat)from->a2;
        to[1][1] = (GLfloat)from->b2;
        to[1][2] = (GLfloat)from->c2;
        to[1][3] = (GLfloat)from->d2;
        to[2][0] = (GLfloat)from->a3;
        to[2][1] = (GLfloat)from->b3;
        to[2][2] = (GLfloat)from->c3;
        to[2][3] = (GLfloat)from->d3;
        to[3][0] = (GLfloat)from->a4;
        to[3][1] = (GLfloat)from->b4;
        to[3][2] = (GLfloat)from->c4;
        to[3][3] = (GLfloat)from->d4;
        return to;
    }

    aiMatrix4x4 glmToAiMatrix4x4(const glm::mat4* from) {
        aiMatrix4x4 to;
        to.a1 = (*from)[0][0];
        to.b1 = (*from)[0][1];
        to.c1 = (*from)[0][2];
        to.d1 = (*from)[0][3];
        to.a2 = (*from)[1][0];
        to.b2 = (*from)[1][1];
        to.c2 = (*from)[1][2];
        to.d2 = (*from)[1][3];
        to.a3 = (*from)[2][0];
        to.b3 = (*from)[2][1];
        to.c3 = (*from)[2][2];
        to.d3 = (*from)[2][3];
        to.a4 = (*from)[3][0];
        to.b4 = (*from)[3][1];
        to.c4 = (*from)[3][2];
        to.d4 = (*from)[3][3];
        return to;
    }

    AssimpData m_sd;

protected:
    // Initial VBO creation, etc
    bool loadGLResources();
    void loadSubMeshes(aiScene const* scene, aiNode* node, SceneNode* parent);
    void copyMeshes(aiScene const* scene, aiNode* node, SceneNode* meshSN);
    void get_bounding_box();
    void get_bounding_box_for_node(const aiNode* nd, aiVector3D* min, aiVector3D* max, aiMatrix4x4* trafo);

    bool debugGL                 = false;
    bool m_debug                 = false;
    bool m_useGL32               = false;
    bool m_suppressRootTransform = false;
    bool m_preTransformVertices  = false;

    Scene3DBase*     m_scene3D         = nullptr;
    ShaderCollector* m_shCol           = nullptr;
    GLBase*          m_glbase          = nullptr;
    Shaders*         normShader        = nullptr;
    WindowBase*      m_scene           = nullptr;
    GLuint           m_programPipeline = 0;
    std::thread      m_loadThread;
    std::string      m_filePath;
    std::string      m_fileExtension;
    std::string      m_fileBufferExtension;
    std::string*     m_fileBuffer             = nullptr;
    glm::vec3        m_rootScaleFact          = glm::vec3{1.f};
    glm::vec3        m_rootRotateYawPitchRoll = glm::vec3{0.f};
    glm::vec3        m_rootBasePoint          = glm::vec3{0.f};
    glm::vec3        dcScale{0.f};
    glm::vec3        dcTrans{0.f};
    glm::vec3        dcSkew{0.f};
    glm::vec4        dcPersp{0.f};
    glm::quat        dcOri = glm::quat(0.f, 0.f, 0.f, 0.f);

    Conditional m_threadRunning;
    sceneData*  s_sd = nullptr;
#ifdef __ANDROID__
    // Assimp::AndroidJNIIOSystem*         m_ioSystem=nullptr;
#endif
};

}  // namespace ara

#endif