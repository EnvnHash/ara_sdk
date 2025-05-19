//
//  SPGridFloor.cpp
//
//  Created by Sven Hahne on 23.10.19.
//

#include "Shaders/ShaderPrototype/SPGridFloor.h"
#include "Shaders/ShaderUtils/ShaderBuffer.h"
#include "CameraSets/CameraSet.h"
#include <GLBase.h>


using namespace glm;
using namespace std;

namespace ara {

SPGridFloor::SPGridFloor(sceneData* sd) : ShaderProto(sd) {
    s_name                = getTypeName<SPGridFloor>();
    s_usesNodeMaterialPar = true;
    rebuildShader(1);
}

void SPGridFloor::rebuildShader(uint32_t nrCameras) {
    if (!nrCameras) {
        return;
    }

    if (s_shader) s_shCol->deleteShader(s_name);

    string vert = ShaderCollector::getShaderHeader() + "// SNGridFloor \n";
    vert +=
        "layout(location = 0) in vec4 position; \n"
        "layout(location = 1) in vec4 normal; \n"
        "layout(location = 2) in vec2 texCoord; \n"
        "uniform mat4 " +
        getStdMatrixNames()[toType(stdMatNameInd::ModelMat)] +
        "; \n"
        "uniform mat3 " +
        getStdMatrixNames()[toType(stdMatNameInd::NormalMat)] +
        "; \n"
        "\n"
        "out VS_GS { \n"
        "\tvec4 rawPos; \n"
        "\tvec3 normal; \n"
        "\tvec2 tex_coord; \n"
        "} vertex_out; \n"
        "\n"
        "void main() { \n"
        "\tvec4 wPos = " +
        getStdMatrixNames()[toType(stdMatNameInd::ModelMat)] +
        " * position; \n"
        "\tvertex_out.rawPos = wPos; \n"
        "\tvertex_out.normal = normalize(" +
        getStdMatrixNames()[toType(stdMatNameInd::NormalMat)] +
        " * normal.xyz); \n"
        "\tvertex_out.tex_coord = texCoord; \n"
        "\tgl_Position = position; \n"
        "}\n";

    //------------------------------------------------------------------------

    std::string geom = ShaderCollector::getShaderHeader() +
                       "layout(triangles, invocations=" + to_string(nrCameras) +
                       ") in;\n"
                       "layout(triangle_strip, max_vertices=3) out;\n"
                       "in VS_GS { \n"
                       "\tvec4 rawPos; \n"
                       "\tvec3 normal; \n"
                       "\tvec2 tex_coord; \n"
                       "} vertex_in[]; \n"
                       "out GS_FS { \n"
                       "\tvec4 rawPos; \n"
                       "\tvec3 normal; \n"
                       "\tvec2 tex_coord; \n"
                       "\tfloat camDist;\n"  // marcog.g : Added for floor render
                       "\tfloat floorSwitch;\n"
                       "\tvec4 ndc;\n"
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
                       "\t\t\tvertex_out.normal = vertex_in[i].normal; \n"
                       "\t\t\tvertex_out.tex_coord = vertex_in[i].tex_coord;\n"
                       "\t\t\tvertex_out.rawPos = vertex_in[i].rawPos; \n"
                       "\t\t\tvertex_out.camDist = -wPos.z;\n"
                       "\t\t\tvertex_out.ndc = p / p.w;\n"
                       "\t\t\tvertex_out.floorSwitch = floorSwitch[gl_InvocationID];\n"
                       "\t\t\tEmitVertex(); \n"
                       "\t\t}\n "
                       "\t\tEndPrimitive(); \n"
                       "\t} \n"
                       "}";

    //------------------------------------------------------------------

    string frag = ShaderCollector::getShaderHeader();

    frag += STRINGIFY(  uniform vec2 floorGridSize;\n

        uniform vec4 ambient; \n		    // material parameter, ambient amount
        uniform vec4 diffuse; \n			// material parameter
        uniform vec4 emissive; \n		    // material parameter
        uniform vec4 specular; \n			// material parameter
        uniform float shininess; \n		// exponent for sharping highlights
        uniform float strength; \n		// extra factor to adjust shininess
        uniform float ambientAmt; \n		// multiplier to material ambient amount

        in GS_FS {\n
            vec4 rawPos; \n
            vec3 normal; \n
            vec2 tex_coord; \n
            float camDist;\n
            float floorSwitch;\n
            vec4 ndc;
        } vertex_in;\n

        layout (location = 0) out vec4 fragColor; \n

        // hard coded standard directional light
        vec4 dirLight() { \n
            // compute cosine of the directions, using dot products, to see how much light would be reflected calculate normal in both directions
            vec3 lightSrc = vec3(-0.35, 0.4, 0.6); \n
            vec3 eye = vec3(0.0, 0.0, 1.0); \n
            // in case of flat shapes, the back and front would be calculated with the same normal. correct this here manually
            vec3 twoSideNormal = gl_FrontFacing ? vertex_in.normal : -vertex_in.normal;
            float diffuseAmt = max(0.0, dot(twoSideNormal, lightSrc)); \n
            float specularAmt = max(0.0, dot(twoSideNormal, eye - lightSrc)); \n

            if (diffuseAmt == 0.0)\n
                specularAmt = 0.0; \n
            else \n
                specularAmt = pow(specularAmt, shininess); \n // sharpen the highlight\n

            vec4 baseCol = diffuse; \n
            vec3 ambientLight = ambient.rgb; \n
            vec3 scatteredLight = ambientLight + baseCol.rgb * diffuseAmt; \n
            vec3 reflectedLight = specularAmt * specular.rgb * strength; \n
            return vec4((scatteredLight + reflectedLight + emissive.rgb) * ambientAmt, 1.0); \n
        }\n

        vec4 gridLevel(vec2 puv, vec4 c, float camDist) \n
        { \n
              vec2 wide_line_thin = vec2(0.01, 0.01); \n
              vec4 gradient = mix( \n
                  c, \n
                  vec4(0.0, 0.0, 0.0, 0.5), \n
                  clamp(1.0 - (2.0 / (1.0 + camDist)), 0.0, 1.0)); \n

              vec2 grid = smoothstep(vec2(1.0) - wide_line_thin * 2.0, vec2(1.0) - wide_line_thin, puv) \n
                  + smoothstep(vec2(1.0) - wide_line_thin * 2.0, vec2(1.0) - wide_line_thin, vec2(1.0) - puv); \n

              return vec4(min(grid.x + grid.y, 1.0)) * gradient; \n
        } \n

        // grid with perspective correct line width
        vec4 floorGrid(vec2 texC) \n
        { \n
              vec2 puv = mod(vec2(texC.x * floorGridSize.x, texC.y * floorGridSize.y), vec2(1.0)); \n
              return (gridLevel(puv, vec4(0.8, 0.8, 0.8, 1.0), vertex_in.camDist) \n
                    + gridLevel(mod(clamp(puv * 5.0, vec2(0.9), vec2(4.9)), vec2(1.0)), vec4(0.5, 0.5, 0.5, 1.0), vertex_in.camDist * vertex_in.camDist)); \n
        } \n

        void main() {\n
            if (!bool(vertex_in.floorSwitch)) discard;

            vec4 procCol = dirLight() + floorGrid(vertex_in.tex_coord); \n
            float on = max(1.0 - (vertex_in.camDist / floorGridSize.y), 0.0); // depth fragment [0-1], 1 => far
            fragColor = vec4(procCol.rgb *2.0 + 0.2, on * on * 0.4); \n
        });

    s_shader = s_shCol->add(s_name, vert, geom, frag);
}

void SPGridFloor::clear(renderPass pass) {}

void SPGridFloor::sendPar(CameraSet* cs, double time, SceneNode* node, SceneNode* parent, renderPass pass, uint loopNr) {
    ShaderProto::sendPar(cs, time, node, parent, pass);

    if (pass != renderPass::shadowMap && s_shader) {
        s_shader->setUniform1fv("floorSwitch", cs->getSetFloorSwitches(), cs->getNrCameras());
    }
}

bool SPGridFloor::begin(CameraSet* cs, renderPass pass, uint loopNr) {
    if (pass == renderPass::scene  && s_shader) {
        s_shader->begin();
        return true;
    }
    return false;
}

bool SPGridFloor::end(renderPass pass, uint loopNr) {
    if (pass == renderPass::scene && s_shader) {
        Shaders::end();
    }
    return false;
}

void SPGridFloor::postRender(renderPass pass) {}

Shaders* SPGridFloor::getShader(renderPass pass, uint loopNr) {
    return pass == renderPass::scene && s_shader ? s_shader : nullptr;
}

void SPGridFloor::setScreenSize(uint width, uint height) {
    s_scrWidth = width;
    s_scrHeight = height;
}

void SPGridFloor::setNrCams(int num) {
    rebuildShader(num);
}

}  // namespace ara
