#pragma once

#ifdef ARA_USE_ASSIMP

#include <assimp/Importer.hpp>

#include "Assimp/AssimpAnimation.h"
#include "Assimp/AssimpMeshHelper.h"
#include "Assimp/AssimpTexture.h"
#include "Assimp/AssimpUtils.h"
#include "Utils/Texture.h"

namespace ara {

class MaterialProperties;
class Mesh;
class Shaders;
class SceneNode;
class SNIdGroup;
class Texture;
class VAO;

class AssimpData {
public:
    AssimpData() = default;
    virtual ~AssimpData() {}

    void drawMeshNoShdr(uint meshNr, GLenum renderType);
    void sendMeshMatToShdr(uint meshNr, Shaders* shdr);
    void drawMesh(uint meshNr, GLenum renderType, Shaders* shdr);
    void draw(GLenum renderType, Shaders* shdr = nullptr);
    void drawInstanced(GLenum renderType, uint nrInst, Shaders* shdr = nullptr);
    void drawNoText(uint meshNr, GLenum renderType, Shaders* shdr);
    void update(double time);
    void updateAnimations(double time);
    void updateMeshes(aiNode* node, glm::mat4 parentMatrix);
    void updateBones();
    void updateGLResources();          ///< updates the *actual GL resources* for the
                                       ///< current animation

    bool             hasAnimations() { return animations.size() > 0; }
    uint             getAnimationCount() { return static_cast<uint>(animations.size()); }
    AssimpAnimation& getAnimation(int animationIndex);

    void playAllAnimations() {
        for (uint i = 0; i < animations.size(); i++) animations[i].play();
    }
    void stopAllAnimations() {
        for (uint i = 0; i < animations.size(); i++) animations[i].stop();
    }
    void resetAllAnimations() {
        for (uint i = 0; i < (int)animations.size(); i++) animations[i].reset();
    }
    void setPausedForAllAnimations(bool pause) {
        for (uint i = 0; i < (int)animations.size(); i++) animations[i].setPaused(pause);
    }
    void setLoopStateForAllAnimations(enum AssimpAnimation::loopType state) {
        for (auto i = 0; i < (int)animations.size(); i++) animations[i].setLoopState(state);
    }
    void setPositionForAllAnimations(float position) {
        for (uint i = 0; i < (int)animations.size(); i++) animations[i].setPosition(position);
    }

    void      enableTextures() { bUsingTextures = true; }
    void      enableNormals() { bUsingNormals = true; }
    void      enableColors() { bUsingColors = true; }
    void      enableMaterials() { bUsingMaterials = true; }
    void      disableTextures() { bUsingTextures = false; }
    void      disableNormals() { bUsingNormals = false; }
    void      disableColors() { bUsingColors = false; }
    void      disableMaterials() { bUsingMaterials = false; }
    bool      hasMeshes() { return modelMeshes.size() > 0; }
    uint      getMeshCount() { return static_cast<uint>(modelMeshes.size()); }
    int       getNumMeshes() { return scene->mNumMeshes; }
    glm::vec3 getPosition() { return pos; }
    glm::vec3 getSceneCenter() { return aiVecToOfVec(scene_center); }
    glm::vec3 getScale() { return scale; }
    glm::vec3 getDimensions() const;
    glm::vec3 getCenter() { return glm::vec3(scene_center.x, scene_center.y, scene_center.z); }
    glm::vec3 getSceneMin(bool bScaled = false) const;
    glm::vec3 getSceneMax(bool bScaled = false) const;
    int       getNumRotations() { return static_cast<int>(rotAngle.size()); }  ///< returns the no. of applied rotations
    glm::vec3 getRotationAxis(int which);
    float     getRotationAngle(int which);
    const aiScene* getAssimpScene() { return scene; }

    std::vector<std::string> getMeshNames() const;
    [[maybe_unused]] Mesh*   getMesh(const std::string& name) const;
    [[maybe_unused]] Mesh*   getMesh(int num) const;
    VAO*                     getVao(int num);
    [[maybe_unused]] Mesh*   getCurrentAnimatedMesh(const std::string& name);
    [[maybe_unused]] Mesh*   getCurrentAnimatedMesh(int num) const;
    MaterialProperties*      getMaterialForMesh(const std::string& name);
    MaterialProperties*      getMaterialForMesh(int num);
    Texture*                 getTextureForMesh(const std::string& name, int ind);
    Texture*                 getTextureForMesh(int num, int ind);

    void clear() {
        /*
        // clear out everything.
        animations.clear();
        modelMeshes.clear();
        pos = vec3(0.f, 0.f, 0.f);
        scale = vec3(1.f, 1.f, 1.f);
        rotAngle.clear();
        rotAxis.clear();
        //lights.clear();

        scale = vec3(1, 1, 1);
        bUsingMaterials = true;
        bUsingNormals = true;
        bUsingTextures = true;
        bUsingColors = true;
        */
    }

    SceneNode*                      sceneRoot = nullptr;
    SNIdGroup*                      idGroup   = nullptr;
    std::function<void(SceneNode*)> loadEndCb;

    bool glOk            = false;
    bool drawIndexed     = false;
    bool loadSuccess     = false;
    bool bUsingTextures  = false;
    bool bUsingNormals   = false;
    bool bUsingColors    = false;
    bool bUsingMaterials = false;

    std::vector<float>     rotAngle;
    std::vector<glm::vec3> rotAxis;
    glm::vec3              scale       = glm::vec3{1.f};
    glm::vec3              pos         = glm::vec3{1.f};
    glm::mat4              modelMatrix = glm::mat4{1.f};

    std::string fileAbsolutePath;

    // the main Asset Import scene that does the magic.
    Assimp::Importer                 import;
    const aiScene*                   scene = nullptr;
    std::shared_ptr<aiPropertyStore> store;
    aiVector3D                       scene_min, scene_max, scene_center;
    aiLogStream                      stream;

    std::vector<AssimpTexture>     textures;
    std::vector<AssimpMeshHelper*> modelMeshes;
    std::vector<AssimpAnimation>   animations;

    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
};
}  // namespace ara
#endif