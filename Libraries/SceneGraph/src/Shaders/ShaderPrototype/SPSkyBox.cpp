//
//  SPSkyBox.cpp
//
//  Created by Sven Hahne on 23.10.19.
//

#include "Shaders/ShaderPrototype/SPSkyBox.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(ara);
#else
namespace fs = std::filesystem;
#endif

using namespace glm;
using namespace std;

namespace ara {

SPSkyBox::SPSkyBox(sceneData* sd) : ShaderProto(sd) {
    s_name    = getTypeName<SPSkyBox>();
    m_cubeTex = make_unique<Texture>(sd->glbase);

    rebuildShader(1);

#ifdef ARA_USE_CMRC
    auto fs = cmrc::ara::get_filesystem();

    if (fs.exists("resdata/skybox.jpg")) {
        auto file = fs.open("resdata/skybox.jpg");  // returns {const char*, const char*}
        if (!file.size()) return;
        m_cubeTex->loadFromMemPtr((void*)file.begin(), file.size(), GL_TEXTURE_CUBE_MAP, 1);
    } else
        LOGE << "ERROR skybox.jpg does not exist on path";
#else
    m_cubeTex->loadTextureCube(s_sd->dataPath + "skybox.jpg");
#endif
}

void SPSkyBox::rebuildShader(uint32_t nrCameras) {
    if (!nrCameras) {
        return;
    }

    string vert = ShaderCollector::getShaderHeader() + "// SPSkyBox \n";
    vert += STRINGIFY(layout(location = 0) in vec4 position; \n out vec4 pos; \n void main() { pos = position; }\n);

    std::string geom = s_glbase->shaderCollector().getShaderHeader() +
                       "layout(triangles, invocations=" + to_string(nrCameras) +
                       ") in;\n"
                       "layout(triangle_strip, max_vertices=3) out;\n"
                       "in vec4 pos[];\n"
                       "out GS_FS { \n"
                       "\tvec3 tex_coord; \n"
                       "} vertex_out; \n"
                       "uniform mat4 " +
                       getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] + "[" + to_string(nrCameras) +
                       "];\n"
                       "uniform mat4 " +
                       getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[" + to_string(nrCameras) +
                       "];\n"
                       "void main() {\n"
                       "\tmat4 normMat = mat4(mat3(transpose(inverse(" +
                       getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] +
                       "[gl_InvocationID]))));\n"
                       "\tfor (int i = 0; i < gl_in.length(); i++) { \n"
                       "\tgl_Layer = gl_InvocationID; \n"
#ifndef ARA_USE_GLES31  // GLES does not support multiple viewports
                       "\tgl_ViewportIndex = gl_InvocationID;\n"
#endif
                       "\t\tvertex_out.tex_coord = pos[i].xyz;\n"
                       "\t\tgl_Position = " +
                       getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] +
                       "[gl_InvocationID] * normMat * pos[i]; \n"
                       "\t\tEmitVertex(); \n"
                       "\t}\n "
                       "\tEndPrimitive(); \n"
                       "}";

    //---

    string frag = ShaderCollector::getShaderHeader();

    frag += STRINGIFY(
        in GS_FS {\n
            vec3 tex_coord; \n
        } vertex_in;\n
    );

#ifdef ARA_USE_GLES31
    frag += "highp uniform samplerCube tex;\n";
#else
    frag += "uniform samplerCube tex; \n";
#endif
    frag += STRINGIFY(layout(location = 0) out vec4 fragColor; \n void main() {
        \n fragColor = texture(tex, vertex_in.tex_coord);
        \n
    });

    s_shader = s_shCol->add(s_name + "_" + std::to_string(nrCameras), vert, geom, frag);
}

void SPSkyBox::clear(renderPass pass) {}

void SPSkyBox::sendPar(CameraSet* cs, double time, SceneNode* node, SceneNode* parent, renderPass pass, uint loopNr) {
    ShaderProto::sendPar(cs, time, node, parent, pass);

    if (pass == GLSG_SCENE_PASS && s_shader) {
        s_shader->setUniform1i("samplerCube", 0);
        m_cubeTex->bind(0);
    }
}

bool SPSkyBox::begin(CameraSet* cs, renderPass pass, uint loopNr) {
    if (pass == GLSG_SCENE_PASS && s_shader) {
        s_shader->begin();
        return true;
    }
    return false;
}

bool SPSkyBox::end(renderPass pass, uint loopNr) {
    if (pass == GLSG_SCENE_PASS && s_shader) {
        Shaders::end();
        return true;
    }
    return false;
}

Shaders* SPSkyBox::getShader(renderPass pass, uint loopNr) {
    return pass == GLSG_SCENE_PASS && s_shader ? s_shader : nullptr;
}

void SPSkyBox::setScreenSize(uint width, uint height) {
    s_scrWidth = width, s_scrHeight = height;
}

void SPSkyBox::setNrCams(int nrCams) {
    rebuildShader(nrCams);
}

}  // namespace ara
