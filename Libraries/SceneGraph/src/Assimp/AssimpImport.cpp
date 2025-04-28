//
//  AssimpImport.cpp
//
//  Created by Sven Hahne on 14/8/15.
//

#ifdef ARA_USE_ASSIMP

#include "AssimpImport.h"
#include <GLBase.h>
#include "Scene3DBase.h"
#include <Utils/BoundingBoxer.h>
#include <Utils/Texture.h>

#define aisgl_min(x, y) (x < y ? x : y)
#define aisgl_max(x, y) (y > x ? y : x)

using namespace glm;
using namespace std;

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(ara);
#else
namespace fs = std::filesystem;
#endif

namespace ara {

AssimpImport::AssimpImport(GLBase* glbase) : m_glbase(glbase), m_shCol(&glbase->shaderCollector()) {}

SceneNode* AssimpImport::load(Scene3DBase* scene3D, std::string& filePath, const std::function<void(SceneNode*)>& cb,
                              bool singleObjId, bool forceOSLoad) {
    if (!scene3D) {
        LOGE << "AssimpImport::load Error: no scene3D set.";
        return nullptr;
    }

    m_scene3D = scene3D;
    s_sd      = scene3D->getSceneData();

    auto modelContainer = scene3D->getSceneTree()->addChild(true);

    if (!forceOSLoad) {
        load(filePath, modelContainer, cb, singleObjId);
    } else {
        loadFromOsFileSys(filePath, modelContainer, cb, singleObjId);  // unfortunately must be done for ANDROID compat reasons
    }

    return modelContainer;
}

void AssimpImport::load(string& filePath, SceneNode* root, function<void(SceneNode*)> cb, bool singleObjId) {
    if (!root) {
        LOGE << "AssimpImport::load Error: need a valid root pointer.";
        return;
    }

#ifdef ARA_USE_CMRC
    auto fs = cmrc::ara::get_filesystem();
    if (fs.exists(filePath)) {
        m_filePath = filePath;

        // load from memory - not working with fbx files ....
        auto ext = filesystem::path(filePath).extension().string();
        setFileBufferExtension(ext);

        SNIdGroup* idGroup = nullptr;
        if (singleObjId) idGroup = root->createIdGroup();

        loadThread(root, cb, idGroup, false);
    } else {
        LOGE << "AssimpImport::load Error: " << filePath << " does not exist.";
    }
#else
    loadFromOsFileSys(filePath, root, std::move(cb), singleObjId);
#endif
}

void AssimpImport::loadFromOsFileSys(string& filePath, SceneNode* root, function<void(SceneNode*)> cb, bool singleObjId) {
    if (std::filesystem::exists(filePath)) {
        m_filePath = filePath;
        auto p     = std::filesystem::path(m_filePath);
        if (p.extension().empty()) {
            return;  // in case a pure directory path was passed -> abort
        }

        m_fileExtension    = p.extension().string();
        SNIdGroup* idGroup = nullptr;
        if (singleObjId) {
            idGroup = root->createIdGroup();
        }

        // be sure the root has glbase
        root->setGLBase(m_glbase);

        loadThread(root, std::move(cb), idGroup, false);

        // be sure the opengl callback is set - if a updateWindow is called after calling this method, it for sure will
        // be called m_threadRunning.wait();
    }
}

void AssimpImport::loadFromBuffer(std::string* fileBuffer, SceneNode* root, std::function<void(SceneNode*)> cb,
                                  bool singleObjId) {
    if (!root) {
        LOGE << "AssimpImport::load Error: need a valid root pointer.";
        return;
    }

    m_fileBuffer       = fileBuffer;
    SNIdGroup* idGroup = nullptr;
    if (singleObjId) idGroup = root->createIdGroup();

    loadThread(root, cb, idGroup, false);
}

void AssimpImport::loadThread(SceneNode* root, function<void(SceneNode*)> cb, SNIdGroup* idGroup, bool optimize) {
    // load model in a separate thread
    if (s_sd && s_sd->debug) {
        LOG << this << " AssimpImport starting loading model " << m_filePath.c_str();
        m_sd.startTime = chrono::system_clock::now();
    }

    m_sd.sceneRoot = root;
    m_sd.loadEndCb = std::move(cb);
    m_sd.idGroup   = idGroup;

    // get a handle to the predefined STDOUT log stream and attach it to the
    // logging system. It remains active for all further calls to
    // aiImportFile(Ex) and aiApplyPostProcessing.
    if (s_sd && s_sd->debug && !s_sd->aiLogStreamSet) {
        aiEnableVerboseLogging(false);
        m_sd.stream = {nullptr};
        m_sd.stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, nullptr);
        aiAttachLogStream(&m_sd.stream);
        s_sd->aiLogStreamSet = true;
    }

#ifdef ARA_USE_CMRC
    // this can be also a request to load from the harddrive...
    auto fs               = cmrc::ara::get_filesystem();
    bool onVirtualFileSys = fs.exists(m_filePath);
    bool onHD             = exists(std::filesystem::path(m_filePath));

    if (onVirtualFileSys || onHD)
#else
    filesystem::path filePathFS = filesystem::path(m_filePath);
    if (filesystem::exists(filePathFS) || m_fileBuffer)
#endif
    {
        m_sd.fileAbsolutePath = GetDirectory(m_filePath);

        if (s_sd && s_sd->debug)
            LOG << " AssimpImport::loadThread: loadModel " << m_filePath.c_str() << " \n fileAbsolutePath "
                << m_sd.fileAbsolutePath.c_str();

        // aiProcess_FlipUVs is for VAR code. Not needed otherwise.
        unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
                             aiProcess_CalcTangentSpace | aiProcess_GenUVCoords | aiProcess_GenNormals | 0;

        if (m_preTransformVertices && !m_suppressRootTransform) flags |= aiProcess_PreTransformVertices;

        if (m_sd.scene) m_sd.import.FreeScene();

        // if there is a filebuffer set -> load from memory, otherwise from file
        if (m_fileBuffer) {
            m_sd.scene = m_sd.import.ReadFileFromMemory((void*)&m_fileBuffer[0], m_fileBuffer->size(), flags,
                                                        m_fileBufferExtension.c_str());
        } else {
#ifdef ARA_USE_CMRC
            auto p = filesystem::path(m_filePath);

            if (onVirtualFileSys) {
                auto file = fs.open(m_filePath);
                if (!file.size()) return;

                size_t          size = file.size();
                vector<uint8_t> vp;
                if (size > 0) {
                    vp.resize(size);
                    std::copy(file.begin(), file.end(), vp.begin());
                }

                m_sd.scene = m_sd.import.ReadFileFromMemory(&vp[0], vp.size(), flags, p.extension().string().c_str());
            } else {
                m_sd.scene = m_sd.import.ReadFile(m_filePath, flags);
            }
#else
            m_sd.scene = m_sd.import.ReadFile(m_filePath, flags);
#endif
        }

        if (m_sd.scene) {
            get_bounding_box();

            if (s_sd && s_sd->debug) {
                LOG << "AssimpImport::loadThread: scene min: " << m_sd.scene_min.x << ", " << m_sd.scene_min.y << ", "
                    << m_sd.scene_min.z;
                LOG << "AssimpImport::loadThread: scene max: " << m_sd.scene_max.x << ", " << m_sd.scene_max.y << ", "
                    << m_sd.scene_max.z;
            }

            m_sd.scene_center.x = (m_sd.scene_min.x + m_sd.scene_max.x) / 2.0f;
            m_sd.scene_center.y = (m_sd.scene_min.y + m_sd.scene_max.y) / 2.0f;
            m_sd.scene_center.z = (m_sd.scene_min.z + m_sd.scene_max.z) / 2.0f;

            vec3 dim    = m_sd.getDimensions();
            vec3 center = m_sd.getCenter();  // convert scene_center to glm::vec3

            if (s_sd && s_sd->debug) {
                LOG << "AssimpImport::loadThread: dimensions: " << to_string(dim).c_str();
                LOG << "AssimpImport::loadThread: center: " << to_string(center).c_str();
            }

            m_sd.loadSuccess = true;

            if (s_sd && s_sd->debug) {
                m_sd.endTime = chrono::system_clock::now();
                double dur   = chrono::duration<double, milli>(m_sd.endTime - m_sd.startTime).count();
                LOG << this << "AssimpImport loading model done, took: " << dur << " ms";
            }

            // add a call the sceneData openGlCbs vector with the local
            // uploadToGl() method
            if (m_scene)
                m_scene->addGlCb(this, m_filePath, bind(&AssimpImport::uploadToGL, this));
            else if (m_scene3D)
                m_scene3D->addGlCb(m_filePath, [this] {
                    uploadToGL();
                    return true;
                });
        } else {
            LOGE << this << "ERROR::ASSIMP " << m_sd.import.GetErrorString();
        }
    } else {
        LOGE << this << "AssimpImport::loadModel(): model does not exist: " << m_filePath.c_str();
    }
}

bool AssimpImport::uploadToGL() {
    if (m_sd.loadSuccess && m_sd.scene)
        return loadGLResources();
    else {
        LOGE << "AssimpImport::loadModel(): " << aiGetErrorString();
        return false;
    }
}

bool AssimpImport::loadGLResources() {
    // copy the scenes bounding box, the imported scene as a node will have transVec(0)
    m_sd.sceneRoot->setBoundingBox(m_sd.scene_min.x * m_rootScaleFact.x, m_sd.scene_max.x * m_rootScaleFact.x,
                                   m_sd.scene_min.y * m_rootScaleFact.y, m_sd.scene_max.y * m_rootScaleFact.y,
                                   m_sd.scene_min.z * m_rootScaleFact.z, m_sd.scene_max.z * m_rootScaleFact.z);

    if (!m_sd.scene->mRootNode || !m_sd.sceneRoot) return false;
    m_sd.sceneRoot->setVisibility(true);

    // convert the rootTransMat to aiMatrix4x4 and start loading nodes
    loadSubMeshes(m_sd.scene, m_sd.scene->mRootNode, m_sd.sceneRoot);  // recursive call

    // Object Map and LightShadow Maps have to be regenerated
    if (s_sd) {
        s_sd->reqRenderPasses->at(GLSG_OBJECT_MAP_PASS) = true;
        s_sd->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
        s_sd->reqRenderPasses->at(GLSG_OBJECT_ID_PASS)  = true;
    }
    m_sd.loadEndCb(m_sd.sceneRoot);  // call gl load end callback, pass by ref

    if (m_scene3D) {
        m_scene3D->getSceneData()->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
        m_scene3D->getSceneData()->reqRenderPasses->at(GLSG_OBJECT_MAP_PASS) = true;
    }

    // the importer destructor will clean up everything

    return true;
}

void AssimpImport::loadSubMeshes(aiScene const* scene, aiNode* node, SceneNode* parent) {
    if (!parent || !scene || !node) {
        return;
    }

    // filter cameras and light
    if (!strcmp(node->mName.C_Str(), "Camera") || !strcmp(node->mName.C_Str(), "Light")) {
        return;
    }

    SceneNode* newNode = parent->addChild(true);
    newNode->setVisibility(true);
    newNode->setName(node->mName.C_Str());
    if (m_sd.idGroup) {
        newNode->setIdGroup(m_sd.idGroup);
    }

    // if this is the scene root
    if (scene->mRootNode == node) {
        // initial root transformation matrix for optional transformations coming from outside the class
        glm::mat4 rootTransMat(1.f);

        // optional rotation
        if (abs(m_rootRotateYawPitchRoll.x) > 0.0001f || abs(m_rootRotateYawPitchRoll.y) > 0.0001f ||
            abs(m_rootRotateYawPitchRoll.z) > 0.0001f) {
            rootTransMat = glm::yawPitchRoll(m_rootRotateYawPitchRoll.x, m_rootRotateYawPitchRoll.y, m_rootRotateYawPitchRoll.z);
        }

        for (int i = 0; i < 3; i++) {
            rootTransMat[3][i] = m_rootBasePoint[i];  // optional translation offset
        }

        for (int i = 0; i < 3; i++) {
            rootTransMat[i][i] = m_rootScaleFact[i];  // optional scaling offset
        }

        // start off with the rootTransMat as is, or multiply it with the scene root Matrix coming from the file to load
        glm::mat4 transMat;

        if (!m_suppressRootTransform && !m_preTransformVertices) {
            transMat = rootTransMat * aiMatrix4x4ToGlm(&m_sd.scene->mRootNode->mTransformation);
        } else if (m_suppressRootTransform && m_preTransformVertices) {
            // in case the root Transformation should be suppressed and the
            // vertices should be pretransformed, apply the scene's root matrix
            // inverted
            transMat = rootTransMat *
                glm::inverse(aiMatrix4x4ToGlm(&m_sd.scene->mRootNode->mTransformation));  // eigentlich quatsch ... wenn
                                                                                          // preTransform an ist, ist
                                                                                          // mRootNode->mTransformation
                                                                                          // identity
        } else {
            transMat = rootTransMat;
        }

        newNode->m_usefixedModelMat = true;
        newNode->setModelMat(transMat);
    } else {
        glm::mat4 nodeMat = aiMatrix4x4ToGlm(&node->mTransformation);
        if (glm::decompose(aiMatrix4x4ToGlm(&node->mTransformation), dcScale, dcOri, dcTrans, dcSkew, dcPersp)) {
            newNode->scale(&dcScale);
            auto rotMat = glm::mat4(dcOri);
            newNode->rotate(rotMat);
            newNode->translate(&dcTrans);
        } else {
            newNode->m_usefixedModelMat = true;
            newNode->setModelMat(nodeMat);
        }
    }

    // if node has meshes, create a new scene object for it
    if (node->mNumMeshes > 0) {
        copyMeshes(scene, node, newNode);
    }

    // continue for all child nodes
    if (node->mNumChildren) {
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            loadSubMeshes(scene, node->mChildren[i], newNode);
        }
    }
}

void AssimpImport::copyMeshes(aiScene const* scene, aiNode* node, SceneNode* parent) {
    if (!s_sd) return;
    aiColor4D dcolor, scolor, acolor, ecolor;
    auto bbox = static_cast<BoundingBoxer*>(s_sd->boundBoxer);

    // create OpenGL buffers and populate them based on each mesh pertinent info.
    for (uint i = 0; i < node->mNumMeshes; ++i) {
        // current mesh we are introspecting
        auto idx = node->mMeshes[i];
        if (idx >= scene->mNumMeshes) continue;

        auto mesh = scene->mMeshes[idx];

        auto meshSN = parent->addChild(false);
        meshSN->setName(mesh->mName.C_Str());
        meshSN->m_depthTest   = true;
        meshSN->m_drawIndexed = true;
        meshSN->m_minTexC     = glm::vec2{std::numeric_limits<float>::max()};
        meshSN->m_maxTexC     = glm::vec2{std::numeric_limits<float>::min()};
        meshSN->setVisibility(true);

        // Handle material info
        bool loadedTextures = false;
        if (mesh->mMaterialIndex != 0) {
            aiMaterial* mtl = scene->mMaterials[mesh->mMaterialIndex];

            // get name
            C_STRUCT aiString mat_name;
            aiGetMaterialString(mtl, AI_MATKEY_NAME, &mat_name);

            // load materials
            if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &dcolor))
                meshSN->getMaterial()->setDiffuse(dcolor.r, dcolor.g, dcolor.b, dcolor.a);

            if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &scolor))
                meshSN->getMaterial()->setSpecular(scolor.r, scolor.g, scolor.b, scolor.a);

            if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &acolor))
                meshSN->getMaterial()->setAmbient(acolor.r, acolor.g, acolor.b, acolor.a);

            if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &ecolor))
                meshSN->getMaterial()->setEmissive(ecolor.r, ecolor.g, ecolor.b, ecolor.a);

            float shininess;
            if (AI_SUCCESS == aiGetMaterialFloat(mtl, AI_MATKEY_SHININESS, &shininess))
                meshSN->getMaterial()->setShininess(shininess);

            int blendMode;
            if (AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_BLEND_FUNC, &blendMode)) {
                if (blendMode == aiBlendMode_Default) {
                    meshSN->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                } else {
                    meshSN->setBlendMode(GL_SRC_ALPHA, GL_ONE);
                }
            }

            // Culling
            uint max = 1;
            int  two_sided;
            if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) {
                meshSN->m_cullFace = two_sided;
            }

            // Load Textures
            int      texIndex      = 0;
            bool     wrongfilePath = false;
            aiString texPath;
            string   realfilePath;

            if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath)) {
                string texPathStr = string(texPath.data);
                string relPath    = GetDirectory(texPathStr);

                // check if the filePath is absolute or relative
                size_t foundPos = relPath.find_first_of("/\\");
                if (foundPos != 2) {
                    foundPos = relPath.find_last_of("/\\");
                    if (foundPos != string::npos) {
                        relPath = relPath.substr(0, relPath.find_last_of("/\\"));
                    } else {
                        relPath = "";
                    }

                    string texFile = texPath.data;
                    realfilePath   = m_sd.fileAbsolutePath;
                    realfilePath += relPath + PATH_SEPARATOR + texFile;
                } else {
                    realfilePath = (string)texPath.data;
                }

                if (filesystem::exists(filesystem::path(realfilePath))) {
                    wrongfilePath = true;
                }

                bool bTextureAlreadyExists = false;
                for (const auto & j : *meshSN->getTextures()) {
                    if (j->getFileName() == realfilePath) {
                        bTextureAlreadyExists = true;
                        break;
                    }
                }

                if (!bTextureAlreadyExists) {
                    meshSN->addTexture(make_unique<Texture>(m_glbase));
                    auto texture = meshSN->getTextures()->back().get();
                    if (!texture->loadTexture2D(realfilePath.c_str())) {
                        LOGE << "AssimpImport::loadGLResource(): couldn't load texture: " << realfilePath.c_str();
                    }
                }

                loadedTextures = true;
            }
        }

        int usage = GL_STREAM_DRAW;

        if (m_sd.getAnimationCount())
            usage = GL_STREAM_DRAW;
        else
            usage = GL_STATIC_DRAW;

        // now upload all data to a vao.
        // init vao, check format
        string format = "position:3f";
        if (mesh->HasNormals()) format += ",normal:3f";
        if (mesh->HasTextureCoords(0)) format += ",texCoord:2f";
        format += ",color:4f";  // also if there are no colors, assign a static
                                // color for debugging reasons

        meshSN->m_vao = new VAO(format.c_str(), GL_DYNAMIC_DRAW);
        meshSN->m_vao->upload(CoordType::Position, &mesh->mVertices[0].x, mesh->mNumVertices);

        if (mesh->HasNormals()) {
            meshSN->m_vao->upload(CoordType::Normal, &mesh->mNormals[0].x, mesh->mNumVertices);
        }

        if (mesh->HasTextureCoords(0)) {
            vector<vec2> vec(mesh->mNumVertices);
            for (uint tc = 0; tc < mesh->mNumVertices; tc++) {
                // A vertex can contain up to 8 different texture coordinates.
                // We thus make the assumption that we won't use models where a
                // vertex can have multiple texture coordinates so we always
                // take the first set [0].
                if (mesh->mTextureCoords[0] && mesh->mTextureCoords[0][tc].x <= 1.0) {
                    vec[tc].x = mesh->mTextureCoords[0][tc].x;
                    vec[tc].y = mesh->mTextureCoords[0][tc].y;

                    // min/max calculation
                    if (meshSN->m_minTexC.x > vec[tc].x) meshSN->m_minTexC.x = vec[tc].x;
                    if (meshSN->m_minTexC.y > vec[tc].y) meshSN->m_minTexC.y = vec[tc].y;
                    if (meshSN->m_maxTexC.x < vec[tc].x) meshSN->m_maxTexC.x = vec[tc].x;
                    if (meshSN->m_maxTexC.y < vec[tc].y) meshSN->m_maxTexC.y = vec[tc].y;
                }
            }

            meshSN->m_vao->upload(CoordType::TexCoord, &vec[0][0], mesh->mNumVertices);
        }

        if (!mesh->HasVertexColors(0)) {
            if (mesh->mTextureCoords[0] && loadedTextures) {
                meshSN->m_vao->setStaticColor(0.f, 0.f, 0.f, 1.f);
            } else {
                meshSN->m_vao->setStaticColor(1.f, 1.f, 1.f, 1.f);
            }
        } else {
            meshSN->m_vao->upload(CoordType::Color, &mesh->mColors[0][0].r, mesh->mNumVertices);
        }

        if (mesh->mNumFaces != 0) {
            for (uint k = 0; k < mesh->mNumFaces; k++) {
                for (uint j = 0; j < mesh->mFaces[k].mNumIndices; j++) {
                    meshSN->m_indices.emplace_back(mesh->mFaces[k].mIndices[j]);
                }
            }
        } else {
            continue;
        }

        meshSN->m_vao->setElemIndices((int)meshSN->m_indices.size(), (GLuint*)&meshSN->m_indices[0]);

        // Assimp only calculates the bounding box of the whole scene, but not of the separate meshs do this here per
        // Mesh via the BoundingBox on the GPU
        bbox->begin();
        meshSN->m_vao->draw(GL_TRIANGLES);
        bbox->end();

        //-------------------------------------------------------------------------

        meshSN->setBoundingBox(&bbox->getBoundMin(), &bbox->getBoundMax());
    }

}

void AssimpImport::createLightsFromAiModel() {
}

string AssimpImport::GetDirectory(string& filePath) {
    size_t found = filePath.find_last_of("/\\");
    return (filePath.substr(0, found));
}

void AssimpImport::initNormShader() {
    // std140 pads vec3 to 4 components, to deal with vec3 we have to use a
    // proper struct
    string src =
        "#version 430\n#define WORK_GROUP_SIZE 128\n;struct Position { float "
        "x, y, z; };\n";

    src += STRINGIFY(
        layout(std430, binding = 1) buffer Vertices {
            Position pos[];
            \n
        }\n;
        layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;\n uniform uint numVertices;\n uniform vec3 center;\n uniform vec3 bBoxSize;\n void
            main() {
                uint i = gl_GlobalInvocationID.x;
                // thread block size may not be exact multiple of number of
                // particles
                if (i >= numVertices) return;
                vec3 p      = (vec3(pos[i].x, pos[i].y, pos[i].z) - center);
                pos[i].x    = p.x;
                \n pos[i].y = p.y;
                \n pos[i].z = p.z;
                \n
            });

    normShader = m_shCol->add("AssimpImportNorm", src.c_str());
}

void AssimpImport::get_bounding_box() {
    aiMatrix4x4 trafo;
    aiIdentityMatrix4(&trafo);

    m_sd.scene_min.x = m_sd.scene_min.y = m_sd.scene_min.z = 1e10f;
    m_sd.scene_max.x = m_sd.scene_max.y = m_sd.scene_max.z = -1e10f;

    get_bounding_box_for_node(m_sd.scene->mRootNode, &m_sd.scene_min, &m_sd.scene_max, &trafo);
}

void AssimpImport::get_bounding_box_for_node(const aiNode* nd, aiVector3D* min, aiVector3D* max, aiMatrix4x4* trafo) {
    aiMatrix4x4 prev = *trafo;
    aiMultiplyMatrix4(trafo, &nd->mTransformation);

    for (uint n = 0; n < nd->mNumMeshes; ++n) {
        const struct aiMesh* mesh = m_sd.scene->mMeshes[nd->mMeshes[n]];
        for (uint t = 0; t < mesh->mNumVertices; ++t) {
            aiVector3D tmp = mesh->mVertices[t];
            aiTransformVecByMatrix4(&tmp, trafo);

            min->x = aisgl_min(min->x, tmp.x);
            min->y = aisgl_min(min->y, tmp.y);
            min->z = aisgl_min(min->z, tmp.z);

            max->x = aisgl_max(max->x, tmp.x);
            max->y = aisgl_max(max->y, tmp.y);
            max->z = aisgl_max(max->z, tmp.z);
        }
    }

    for (uint n = 0; n < nd->mNumChildren; ++n) get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);

    *trafo = prev;
}

AssimpImport::~AssimpImport() {
#ifdef __ANDROID__
    // if (m_ioSystem)        delete m_ioSystem;
#endif
    if (m_scene) m_scene->eraseGlCb(this);
}

}  // namespace ara

#endif