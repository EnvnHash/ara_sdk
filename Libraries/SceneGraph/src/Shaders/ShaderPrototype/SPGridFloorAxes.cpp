//
//  SPGridFloorAxes.cpp
//
//  Created by Sven Hahne on 23.10.19.
//

#include "Shaders/ShaderPrototype/SPGridFloorAxes.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SPGridFloorAxes::SPGridFloorAxes(sceneData* sd) : ShaderProto(sd) {
    s_name = getTypeName<SPGridFloorAxes>();
    rebuildShader(1);
}

void SPGridFloorAxes::rebuildShader(uint32_t nrCameras) {
    if (!nrCameras) {
        return;
    }

    if (s_shader) {
        s_shCol->deleteShader(s_name);
    }

    string vert = ShaderCollector::getShaderHeader() + "// SPGridFloorAxes \n";
    vert +=
        "layout(location = 0) in vec4 position; \n"
        "layout(location = 3) in vec4 color; \n"
        "uniform mat4 " +
        getStdMatrixNames()[toType(stdMatNameInd::ModelMat)] +
        "; \n"
        "uniform mat3 " +
        getStdMatrixNames()[toType(stdMatNameInd::NormalMat)] +
        "; \n"
        "uniform vec4 lineCol[2]; \n"
        "\n"
        "out VS_GS { \n"
        "\tvec4 rawPos; \n"
        "\tvec4 color; \n"
        "} vertex_out; \n"
        "\n"
        "void main() { \n"
        "\tvec4 wPos = " +
        getStdMatrixNames()[toType(stdMatNameInd::ModelMat)] +
        " * position; \n"
        "\tvertex_out.rawPos = wPos; \n"
        "\tvertex_out.color = lineCol[int(gl_VertexID >= 2)]; \n"
        "\tgl_Position = position; \n"
        "}\n";

    std::string geom = ShaderCollector::getShaderHeader() +
                       "layout(lines, invocations=" + to_string(nrCameras) +
                       ") in;\n"
                       "layout(line_strip, max_vertices=2) out;\n"
                       "in VS_GS { \n"
                       "\tvec4 rawPos; \n"
                       "\tvec4 color; \n"
                       "} vertex_in[]; \n"
                       "out GS_FS { \n"
                       "\tvec4 color; \n"
                       "\tfloat camDist;\n"  // marcog.g : Added for floor render
                       "\tfloat floorSwitch;\n"
                       "} vertex_out; \n"
                       "uniform mat4 " +
                       getStdMatrixNames()[toType(stdMatNameInd::CamModelMat)] + "[" + to_string(nrCameras) +
                       "];\n"
                       "uniform mat4 " +
                       getStdMatrixNames()[toType(stdMatNameInd::ProjectionMat)] + "[" + to_string(nrCameras) +
                       "];\n"
                       "uniform float floorSwitch[" +
                       to_string(nrCameras) +
                       "];\n"
                       "uniform int skipForInd; \n"
                       "uniform int camLimit; \n"
                       "uniform int fishEye[" +
                       to_string(nrCameras) + "]; \n" + "uniform vec4 fishEyeAdjust[" + to_string(nrCameras) + "]; \n" +
                       ShaderCollector::getFisheyeVertSnippet(nrCameras) +
                       "void main() {\n"
                       "\tif(skipForInd != gl_InvocationID && gl_InvocationID <= camLimit) {\n"
                       "\t\tfor (int i = 0; i < gl_in.length(); i++) { \n"
                       "\t\t\tgl_Layer = gl_InvocationID; \n"
                       "\t\t\tvec4 wPos = " +
                       getStdMatrixNames()[toType(stdMatNameInd::CamModelMat)] +
                       "[gl_InvocationID] * vertex_in[i].rawPos; \n"
                       "\t\t\tvec4 p = " +
                       getStdMatrixNames()[toType(stdMatNameInd::ProjectionMat)] +
                       "[gl_InvocationID] * wPos; \n"
                       "\t\t\tgl_Position = bool(fishEye[gl_InvocationID]) ? "
                       "fishEyePos(wPos): p; \n"
                       "\t\t\tvertex_out.color = vertex_in[i].color; \n"
                       "\t\t\tvertex_out.camDist = -wPos.z;\n"
                       "\t\t\tvertex_out.floorSwitch = floorSwitch[gl_InvocationID];\n"
                       "\t\t\tEmitVertex(); \n"
                       "\t\t}\n "
                       "\t\tEndPrimitive(); \n"
                       "\t} \n"
                       "}";

    //------------------------------------------------------------------

    string frag = ShaderCollector::getShaderHeader();

    frag += STRINGIFY(uniform vec2 floorGridSize;\n
        in GS_FS {\n
            vec4 color; \n
            float camDist;\n
            float floorSwitch;\n
        } vertex_in;\n

        layout (location = 0) out vec4 fragColor; \n

        void main() {\n
            if (!bool(vertex_in.floorSwitch)) {
                discard;
            }

            vec4 procCol = vertex_in.color; \n
            float on = max(1.0 - (vertex_in.camDist / floorGridSize.y), 0.0); // depth fragment [0-1], 1 => far
            fragColor = vec4(procCol.rgb, on * on * procCol.a); \n
        });

    s_shader = s_shCol->add(s_name, vert, geom, frag);
}

void SPGridFloorAxes::clear(renderPass pass) {}

void SPGridFloorAxes::sendPar(CameraSet* cs, double time, SceneNode* node, SceneNode* parent, renderPass pass, uint loopNr) {
    ShaderProto::sendPar(cs, time, node, parent, pass);

    if (pass == renderPass::scene && s_shader) {
        s_shader->setUniform1fv("floorSwitch", cs->getSetFloorSwitches(), cs->getNrCameras());
    }
}

bool SPGridFloorAxes::begin(CameraSet* cs, renderPass pass, uint loopNr) {
    if(pass == renderPass::scene && s_shader) {
        s_shader->begin();
        return true;
    }
    return false;
}

bool SPGridFloorAxes::end(renderPass pass, uint loopNr) {
    if (pass == renderPass::scene && s_shader) {
        Shaders::end();
    }
    return false;
}

void SPGridFloorAxes::postRender(renderPass pass) {}

Shaders* SPGridFloorAxes::getShader(renderPass pass, uint loopNr) {
    return pass == renderPass::scene && s_shader ? s_shader : nullptr;
}

void SPGridFloorAxes::setScreenSize(uint width, uint height) {
    s_scrWidth = width, s_scrHeight = height;
}

void SPGridFloorAxes::setNrCams(int num) {
    rebuildShader(num);
}

}  // namespace ara
