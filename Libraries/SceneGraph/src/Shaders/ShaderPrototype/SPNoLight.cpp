//
//  SPSpotLightShadowVsm.cpp
//
//  Created by Sven Hahne on 23.10.19.
//

#include "Shaders/ShaderPrototype/SPNoLight.h"
#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SPNoLight::SPNoLight(sceneData *sd) : ShaderProto(sd) {
    s_name                = getTypeName<SPNoLight>();
    s_maxSceneLightDens   = 30.f;
    s_usesNodeMaterialPar = true;

#ifdef STAGE3D_USE_UB
    s_useUniformBlock = false;
#endif

    // init shader
    rebuildShader(1);
}

void SPNoLight::rebuildShader(uint32_t nrCameras) {
    if (!nrCameras) return;


    string vert = s_shCol->getShaderHeader() + "// SPNoLight Light Prototype\n";
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 1) in vec4 normal; \n
        layout(location = 2) in vec2 texCoord; \n
        layout(location = 3) in vec4 color; \n);

    if (!s_useUniformBlock) {
        vert += "uniform mat4 " + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +"; \n"
                "uniform mat3 " + getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] + "; \n";
        if (!s_lights.empty()) vert += "uniform mat4 shadow_matrix[" + std::to_string(s_lights.size()) + "]; \n";
    } else {
        vert += getNodeDataUb(nrCameras);
    }

    vert += "\n"
        "out VS_GS { \n"
        "vec4 rawPos; \n"
        "vec3 normal; \n"
        "vec2 tex_coord; \n"
        "vec4 color; \n"
        "} vertex_out; \n"
        "\n"
        "void main() { \n"
        "vec4 wPos = " + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] + " * position; \n"
        "vertex_out.rawPos = wPos; \n"
        "vertex_out.normal = normalize(" + getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] + " * normal.xyz); \n"
        "vertex_out.tex_coord = texCoord; \n"
        "vertex_out.color = color; \n"
        "gl_Position = position; \n"
        "}";

    //------------------------------------------------------------------------

    std::string geom = s_glbase->shaderCollector().getShaderHeader() +
       "layout(triangles, invocations=" + to_string(nrCameras) + ") in;\n"
       "layout(triangle_strip, max_vertices=3) out;\n"

        "in VS_GS { \n"
        "\tvec4 rawPos; \n"
        "\tvec3 normal; \n"
        "\tvec2 tex_coord; \n"
        "\tvec4 color; \n"
        "} vertex_in[]; \n"

        "out GS_FS { \n"
        "\tvec4 rawPos; \n"
        "\tvec3 normal; \n"
        "\tvec2 tex_coord; \n"
        "\tvec4 color; \n"
        "\tfloat camDist;\n"
        "} vertex_out; \n";

    if (!s_useUniformBlock) {
        geom += "uniform mat4 " + getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] + "[" + to_string(nrCameras) + "];\n"
                "uniform mat4 " + getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[" + to_string(nrCameras) + "];\n"
                "uniform int skipForInd; \n"
                "uniform int camLimit; \n"
                "uniform vec2 texMin;\n"
                "uniform vec2 texMax;\n"
                "uniform int normTexCoord; \n"
                "uniform ivec2 flipTc; \n"
                "uniform ivec2 invertTc; \n";
    } else {
        geom += getNodeDataUb(nrCameras);
    }

    geom +=
        "void main() {\n"
        "\tif(skipForInd != gl_InvocationID && gl_InvocationID <= camLimit) {\n"
        "\t\tbool emitPrimitive = true; \n "
        "\t\tfor (int i = 0; i < gl_in.length(); i++) { \n"
        "\t\t\tgl_Layer = gl_InvocationID; \n"
#ifndef ARA_USE_GLES31  // GLES does not support multiple viewports
        "\t\t\tgl_ViewportIndex = gl_InvocationID; \n"
#endif
        "\t\t\tvec4 wPos = " + getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] + "[gl_InvocationID] * vertex_in[i].rawPos; \n"
        "\t\t\tvec4 p = " + getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[gl_InvocationID] * wPos; \n"
        "\t\t\tgl_Position = p; \n"
        "\t\t\tvertex_out.normal = vertex_in[i].normal; \n"
        "\t\t\tvec2 tc = bool(normTexCoord) ? (vertex_in[i].tex_coord - texMin) / (texMax - texMin) : vertex_in[i].tex_coord; \n"
        "\t\t\tvertex_out.tex_coord = vec2(bool(flipTc.x) ? 1.0 - tc.x : (bool(invertTc.x) ? (texMin.x + (texMax.x - tc.x)) : tc.x), \n"
        "\t\t\t\tbool(flipTc.y) ? 1.0 - tc.y : (bool(invertTc.y) ? (texMin.y + (texMax.y - tc.y)) : tc.y)); \n"
        "\t\t\tvertex_out.color = vertex_in[i].color; \n"
        "\t\t\tvertex_out.rawPos = vertex_in[i].rawPos; \n"
        "\t\t\tvertex_out.camDist=-wPos.z;\n"
        "\t\t\tEmitVertex(); \n"
        "\t\t}\n "
        "\t\tEndPrimitive(); \n"
        "\t} \n"
        "}";

    //------------------------------------------------------------------

    string frag = s_shCol->getShaderHeader();

    frag +=
        "uniform sampler2D tex0;\n"
        "uniform sampler2D tex2;\n"
        "uniform sampler2D tex1;\n";

    if (!s_useUniformBlock) {
        frag +=
            "uniform int hasTexture;\n"
            "uniform int isGizmo;\n"
            "uniform int polyFill;\n"
            "uniform int depthFromTex;\n"
            "uniform vec2 texMin;\n"
            "uniform vec2 texMax;\n"
            "uniform vec2 resolution;\n";
    } else {
        frag += getNodeDataUb(nrCameras);
    }

    frag += STRINGIFY(float pi=3.14159265358979323846;\n
        in GS_FS {\n
        \t vec4 rawPos; \n
	\t vec3 normal; \n
	\t vec2 tex_coord; \n
	\t vec4 color; \n
	\t float camDist;\n
        } vertex_in; \n\n
        layout(location = 0) out vec4 fragColor;\n);

    frag += STRINGIFY(
    void main() {\n
        vec4 diffuse = bool(hasTexture) ? texture(tex0, vertex_in.tex_coord) : vec4(vertex_in.color.rgb, 1.0); \n
        vec4 procCol = bool(isGizmo) ? clamp(diffuse, vec4(0.3), vec4(0.8)) : diffuse; \n
        float alpha = bool(isGizmo) ? 1.0 : procCol.a; \n
        fragColor = vec4(procCol.rgb, alpha); \n
        gl_FragDepth = alpha > 0.05 ? gl_FragCoord.z : 1.0; \n
    });

    s_shader = s_shCol->add("SPNoLight_" + std::to_string(nrCameras), vert, geom, frag);
}

void SPNoLight::clear(renderPass _pass) {
    if (_pass == GLSG_SHADOW_MAP_PASS && shadowGen) {
        shadowGen->clear();
    }
}

void SPNoLight::sendPar(CameraSet *cs, double time, SceneNode *node, SceneNode *parent, renderPass _pass, uint loopNr) {
    ShaderProto::sendPar(cs, time, node, parent, _pass, loopNr);

    if ((_pass == GLSG_SCENE_PASS || _pass == GLSG_GIZMO_PASS) && s_shader) {
        s_shader->setUniform1i("hasTexture", 0);
        s_shader->setUniform1i("isGizmo", (int)(parent->m_nodeType == GLSG_GIZMO));
    }
}

bool SPNoLight::begin(CameraSet *cs, renderPass pass, uint loopNr) {
    switch (pass) {
        case GLSG_SCENE_PASS:
            if (s_shader) {
                glEnable(GL_BLEND);  // something is disabeling blending before randomly...
                s_shader->begin();
            }
            return true;

        case GLSG_GIZMO_PASS:
            if (s_shader) {
                s_shader->begin();
            }
            return true;

        default: return false;
    }
}

bool SPNoLight::end(renderPass pass, uint loopNr) {
    if (pass == GLSG_SCENE_PASS || pass == GLSG_GIZMO_PASS) {
#ifndef __APPLE__
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
#endif
        if (s_shader) {
            Shaders::end();
        }
    }

    return loopNr < (s_nrPasses - 1);
}

Shaders *SPNoLight::getShader(renderPass pass, uint loopNr) {
    switch (pass) {
        case GLSG_SHADOW_MAP_PASS:
        case GLSG_SCENE_PASS:
            return s_shader ? s_shader : nullptr;
        case GLSG_GIZMO_PASS:
            return s_shader ? s_shader : nullptr;
        default:
            return nullptr;
    }
}

void SPNoLight::setScreenSize(uint width, uint height) {
    s_scrWidth = width, s_scrHeight = height;
    if (shadowGen) {
        shadowGen->setScreenSize(width, height);
        if (s_sd) {
            s_sd->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
        }
    }
}

void SPNoLight::setNrCams(int nrCams) {
    s_nrCams = nrCams;
    rebuildShader(s_nrCams);
}

std::string SPNoLight::getUbPar(uint32_t nrCameras) {
    return ShaderProto::getUbPar(nrCameras) +
           "\t vec2 texMin;\n"
           "\t vec2 texMax;\n"
           "\t int normTexCoord; \n"
           "\t ivec2 flipTc; \n"
           "\t ivec2 invertTc; \n"
           "\t int hasTexture;\n"
           "\t int lightMode;\n"
           "\t int shineThrough;\n"
           "\t float maxSceneLightDens;\n"
           "\t int showProjBright;\n"
           "\t int lightIndIsActMesh;\n"
           "\t int isGizmo;\n"
           "\t int drawGridTexture;\n"
           "\t int polyFill;\n"
           "\t vec2 gridNrSteps;\n"
           "\t float gridBgAlpha;\n"
           "\t float gridLineThickness;\n"
           "\t vec2 resolution;\n";
}

}