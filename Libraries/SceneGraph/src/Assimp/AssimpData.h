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

    void drawMeshNoShdr(uint meshNr, GLenum renderType) const;
    void sendMeshMatToShdr(uint meshNr, Shaders* shdr);

    void setTextures(const AssimpMeshHelper& mesh, Shaders *shdr) const;
    static void setCullFace(const AssimpMeshHelper& mesh);
    static void setBlendFunc(const AssimpMeshHelper& mesh);
    void setMaterials(AssimpMeshHelper& mesh, Shaders* shdr) const ;
    void drawByRenderType(const AssimpMeshHelper& mesh, GLenum renderType, uint nrInst = 0) const;
    void draw(AssimpMeshHelper& mesh, Shaders* shdr, GLenum renderType, bool useTextures, uint nrInst = 0) const;

    void drawMesh(uint meshNr, GLenum renderType, Shaders* shdr);
    void draw(GLenum renderType, Shaders* shdr = nullptr);
    void drawInstanced(GLenum renderType, uint nrInst, Shaders* shdr = nullptr);
    void drawNoText(uint meshNr, GLenum renderType, Shaders* shdr);

    void update(double time);
    void updateAnimations(double time);
    void updateMeshes(const aiNode* node, const glm::mat4 &parentMatrix);
    void updateBones();
    void updateGLResources();          ///< updates the *actual GL resources* for the
                                       ///< current animation

    bool             hasAnimations() { return animations.size() > 0; }
    uint             getAnimationCount() { return static_cast<uint>(animations.size()); }
    AssimpAnimation& getAnimation(int animationIndex);

    void playAllAnimations() {
        for (auto & animation : animations) {
            animation.play();
        }
    }
    void stopAllAnimations() {
        for (auto & animation : animations) {
            animation.stop();
        }
    }
    void resetAllAnimations() {
        for (auto & animation : animations) {
            animation.reset();
        }
    }
    void setPausedForAllAnimations(bool pause) {
        for (auto & animation : animations) {
            animation.setPaused(pause);
        }
    }
    void setLoopStateForAllAnimations(enum AssimpAnimation::loopType state) {
        for (auto & animation : animations) {
            animation.setLoopState(state);
        }
    }
    void setPositionForAllAnimations(float position) {
        for (auto & animation : animations) {
            animation.setPosition(position);
        }
    }

    void enableTextures() { bUsingTextures = true; }
    void enableNormals() { bUsingNormals = true; }
    void enableColors() { bUsingColors = true; }
    void enableMaterials() { bUsingMaterials = true; }
    void disableTextures() { bUsingTextures = false; }
    void disableNormals() { bUsingNormals = false; }
    void disableColors() { bUsingColors = false; }
    void disableMaterials() { bUsingMaterials = false; }

    [[nodiscard]] bool              hasMeshes() const { return !modelMeshes.empty(); }
    [[nodiscard]] uint              getMeshCount() const { return static_cast<uint>(modelMeshes.size()); }
    [[nodiscard]] int               getNumMeshes() const { return scene->mNumMeshes; }
    [[nodiscard]] glm::vec3         getPosition() const { return pos; }
    [[nodiscard]] glm::vec3         getSceneCenter() const { return aiVecToOfVec(scene_center); }
    [[nodiscard]] glm::vec3         getScale() const { return scale; }
    [[nodiscard]] glm::vec3         getDimensions() const;
    [[nodiscard]] glm::vec3         getCenter() const { return {scene_center.x, scene_center.y, scene_center.z}; }
    [[nodiscard]] glm::vec3         getSceneMin(bool bScaled = false) const;
    [[nodiscard]] glm::vec3         getSceneMax(bool bScaled = false) const;
    [[nodiscard]] int               getNumRotations() const { return static_cast<int>(rotAngle.size()); }  ///< returns the no. of applied rotations
    [[nodiscard]] glm::vec3         getRotationAxis(int which) const;
    [[nodiscard]] float             getRotationAngle(int which) const;
    [[nodiscard]] const aiScene*    getAssimpScene() const { return scene; }

    [[nodiscard]] std::vector<std::string> getMeshNames() const;
    [[maybe_unused]] [[nodiscard]] Mesh*   getMesh(const std::string& name) const;
    [[maybe_unused]] [[nodiscard]] Mesh*   getMesh(int num) const;
    [[nodiscard]] VAO*                     getVao(int num) const;
    [[maybe_unused]] Mesh*                 getCurrentAnimatedMesh(const std::string& name);
    [[maybe_unused]] [[nodiscard]] Mesh*   getCurrentAnimatedMesh(int num) const;

    [[nodiscard]] const MaterialProperties *getMaterialForMesh(const std::string &name) const;
    MaterialProperties*                     getMaterialForMesh(int num);
    [[nodiscard]] Texture*                  getTextureForMesh(const std::string& name, int ind) const;
    [[nodiscard]] Texture*                  getTextureForMesh(int num, int ind) const;

    void clear() {}

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

    std::vector<AssimpTexture>      textures;
    std::vector<AssimpMeshHelper>   modelMeshes;
    std::vector<AssimpAnimation>    animations;

    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
};
}  // namespace ara
#endif