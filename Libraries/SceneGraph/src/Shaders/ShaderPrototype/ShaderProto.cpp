//
//  ShaderProto.cpp
//
//  Created by Sven Hahne on 11.08.17
//

#include "ShaderProto.h"
#include "CameraSets/CameraSet.h"
#include "Lights/Light.h"
#include "SceneNodes/SceneNode.h"
#include "Shaders/Shaders.h"
#include "Utils/UniformBlock.h"

using namespace glm;
using namespace std;

namespace ara {
ShaderProto::ShaderProto(sceneData* sd)
    : s_shCol(&sd->glbase->shaderCollector()),
    s_glbase(sd->glbase),
    s_scrWidth(static_cast<int>(sd->winViewport.z)),
    s_scrHeight(static_cast<int>(sd->winViewport.w)),
    s_sd(sd) {}

/**
 * Standard shader setup: model, view, projection matrices, object highlighting, node Materials
 */
void ShaderProto::sendPar(CameraSet* cs, double time, SceneNode* node, SceneNode* parent, renderPass pass, uint loopNr) {
    if (pass != GLSG_SHADOW_MAP_PASS && s_shader) {
        // update values
        s_camLimit   = pass == GLSG_GIZMO_PASS ? 0 : 100000;
        s_skipForInd = !node->m_skipForCamInd.empty() ? node->m_skipForCamInd[parent] : 100000;

        if (pass != GLSG_OBJECT_MAP_PASS) {
            s_highLight = static_cast<float>(node->isSelected(parent));  // marco.g : this fixes the projector model
        }                                                     // selection, "parent" had to be passed so it could
                                                            // evaluate if it is selected too, it solves the
                                                            // problem to at least 1 level

        if (s_useUniformBlock) {
            if (!s_ub.isInited() || s_shader->getProgram() != s_ub.getProgram()) {
                s_ub.init(s_shader->getProgram(), "nodeData");

                s_ub.addVarName(getStdMatrixNames()[toType(StdMatNameInd::ModelMat)], value_ptr(node->getModelMat(parent)), 0);
                s_ub.addVarName(getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)], cs->getSetModelMatrPtr(), 0);
                s_ub.addVarName(getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)], cs->getSetProjectionMatrPtr(), 0);
                s_ub.addVarName(getStdMatrixNames()[toType(StdMatNameInd::NormalMat)], value_ptr(node->getNormalMat(parent)), 0);

                // check if the actual node is a camera and part of the actual
                // camera set, in this case, tell the geometry shader to skip it
                s_ub.addVarName("skipForInd", &s_skipForInd, GL_INT);
                s_ub.addVarName("camLimit", &s_camLimit, GL_INT);
                s_ub.addVarName("fishEye", cs->getSetFishEyeSwitches(), 0);
                s_ub.addVarName("fishEyeAdjust", cs->getSetFishEyeParams(), 0);
                s_ub.addVarName("highLight", &s_highLight, GL_FLOAT);
            }

        } else {
            // the Shader Class automatically detects if the uniform exists or not this redundancy is worth the pain
            // since it is done only once while internally model/view/projection multiplication would be done for each
            // vertex which will be way slower for model with a lot of vertexes
            s_shader->setUniformMatrix4fv(getStdMatrixNames()[toType(StdMatNameInd::ModelMat)], value_ptr(node->getModelMat(parent)));
            s_shader->setUniformMatrix4fv(getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)], cs->getSetModelMatrPtr(), cs->getNrCameras());
            s_shader->setUniformMatrix4fv(getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)], cs->getSetProjectionMatrPtr(), cs->getNrCameras());

            // check if the actual node is a camera and part of the actual
            // camera set, in this case, tell the geometryshader to skip it
            s_shader->setUniform1i("skipForInd", s_skipForInd);
            s_shader->setUniform1i("camLimit", s_camLimit);
            s_shader->setUniform1iv("fishEye", cs->getSetFishEyeSwitches(), cs->getNrCameras());
            s_shader->setUniform4fv("fishEyeAdjust", cs->getSetFishEyeParams(), cs->getNrCameras());

            if (pass != GLSG_OBJECT_MAP_PASS) {
                s_shader->setUniformMatrix3fv(getStdMatrixNames()[toType(StdMatNameInd::NormalMat)], value_ptr(node->getNormalMat(parent)));
                s_shader->setUniform1f("highLight", s_highLight);
            }
        }

        // send SceneNode Material, uses a separate uniform block
        if (pass != GLSG_OBJECT_MAP_PASS) {
            node->getMaterial()->sendToShader(s_shader->getProgram());
        }
    }
}

std::string ShaderProto::getNodeDataUb(uint32_t nrCameras) {
    return "uniform nodeData {\n" + getUbPar(nrCameras) + "};\n";
}

std::string ShaderProto::getUbPar(uint32_t nrCameras) {
    return "\t mat4 " + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
           ";\n"
           "\t mat4 " +
           getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] + "[" + std::to_string(nrCameras) +
           "];\n"
           //"\t mat4 " + getStdMatrixNames()[toType(StdMatNameInd::ViewMat)] +
           //"[" + std::to_string(nrCameras) + "];\n"
           "\t mat4 " +
           getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[" + std::to_string(nrCameras) +
           "];\n"
           "\t mat3 " +
           getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] +
           ";\n"
           "\t int skipForInd; \n"
           "\t int camLimit; \n"
           "\t int fishEye[" +
           to_string(nrCameras) +
           "];\n"
           "\t vec4 fishEyeAdjust[" +
           to_string(nrCameras) +
           "];\n"
           "\t float highLight;\n";
}

void ShaderProto::addLight(Light* light) {
    light->setup(false);
    s_lights.emplace_back(light);
    s_reqCalcLights = true;
}

void ShaderProto::removeLight(Light* light) {
    auto it = ranges::find_if(s_lights, isThisLight(light));

    if (it != s_lights.end()) {
        eraseShadowMap(static_cast<uint>(it - s_lights.begin()));
        s_lights.erase(it);
    } else {
        LOGE << " ShaderProto::removeLight Error!!! Couldn't find Light to remove";
    }

    s_reqCalcLights = true;
}

}  // namespace ara
