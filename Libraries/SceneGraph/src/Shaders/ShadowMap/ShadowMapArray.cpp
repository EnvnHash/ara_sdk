/**
 *	Standard ShadowMap Generator
 *
 *  Created by Sven Hahne on 12.07.14.
 *
 */

#include "ShadowMapArray.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

ShadowMapArray::ShadowMapArray(CameraSet* _cs, int _scrWidth, int _scrHeight, uint _initNrLights)
    : ShadowMap(_cs->getGLBase()), shCol(&_cs->getGLBase()->shaderCollector()), nrLights(_initNrLights) {
    s_cs        = _cs;
    s_scrWidth  = _scrWidth;
    s_scrHeight = _scrHeight;

#ifndef ARA_USE_GLES31
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &max_shader_invoc);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxNumberLayers);
#endif

    rebuildFbo(_initNrLights);
    rebuildShader(_initNrLights);
}

void ShadowMapArray::rebuildShader(uint _nrLights) {
    // since the number of lights used in the scene will vary during runtime
    // we will use ShaderBufferObjects to not have to recompile the shader every
    // time
    // ...and they are faster anyway
    uint limitNrLights = std::max<uint>(std::min<uint>(_nrLights, (uint)max_shader_invoc), 1);

    string vert = shCol->getShaderHeader() + "// ShadowMapArray vertex Shader\n";

    vert += STRINGIFY(layout(location = 0) in vec4 position; \n void main(void) {
        \n gl_Position = position;
        \n
    });

    std::string geom = shCol->getShaderHeader() + "// ShadowMapArray geom Shader\n";
    geom += "layout(triangles, invocations= " + std::to_string(limitNrLights) +
            ") in;\n"
            "layout(triangle_strip, max_vertices = 3) out;\n"
            "uniform mat4 m_pv[" +
            std::to_string(limitNrLights) +
            "];\n"
            "uniform mat4 " +
            getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
            "; \n"
            "uniform int lightIndIsActMesh;\n";

    geom += STRINGIFY(void main() {
        if (lightIndIsActMesh != gl_InvocationID) {
            for (int i = 0; i < gl_in.length(); i++) {
                gl_Layer = gl_InvocationID;
                                gl_Position = m_pv[gl_InvocationID] * ) + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] + STRINGIFY( * gl_in[i].gl_Position;
				EmitVertex();
            }
            EndPrimitive();
        }
    });

    std::string frag = shCol->getShaderHeader() + "// ShadowMapArray fragment Shader\n";

    frag += STRINGIFY(layout(location = 0) out vec4 color; \n void main(void) { color = vec4(1.0); });

    if (s_shadowShader) shCol->deleteShader("ShadowMapArray");

    s_shadowShader = shCol->add("ShadowMapArray", vert.c_str(), geom.c_str(), frag.c_str());
}

void ShadowMapArray::rebuildFbo(uint _nrLights) {
    if (s_fbo) {
        s_fbo.release();

        // delete textures views
        for (uint i = 0; i < std::min(nrLights, (uint)depthTexViews.size()); i++) {
            glDeleteTextures(1, &depthTexViews[i]);
        }
    }

    // s_fbo for saving the depth information
    s_fbo = make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth * 2, s_scrHeight * 2, std::max<int>(_nrLights, 1), GL_RGBA8,
                             GL_TEXTURE_2D_ARRAY, true, 1, 1, 1, GL_CLAMP_TO_BORDER, true});

    // set the necessary texture parameters for the depth textures
    glBindTexture(GL_TEXTURE_2D_ARRAY, s_fbo->getDepthImg());
    setShadowTexPar(GL_TEXTURE_2D_ARRAY);

    // build textures views
    depthTexViews.resize(_nrLights);
    for (uint i = 0; i < _nrLights; i++) {
        // if (i >= nrLights)
        glGenTextures(1, &depthTexViews[i]);

#ifndef ARA_USE_GLES31
        // Now, create a view of the depth textures
        glTextureView(depthTexViews[i],      // New texture view
                      GL_TEXTURE_2D,         // Target for the new view
                      s_fbo->getDepthImg(),  // Original texture
                      GL_DEPTH24_STENCIL8,   // depth component m_format
                      0, 1,                  // All mipmaps, but it's only one
                      i, 1);                 // the specific layer, only one layer
#endif
        glBindTexture(GL_TEXTURE_2D, depthTexViews[i]);
        setShadowTexPar(GL_TEXTURE_2D);
    }

    nrLights = _nrLights;
}

void ShadowMapArray::setShadowTexPar(GLenum type) {
    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // setup depth comparison
    glTexParameteri(type, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(type, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // setup wrapping
    glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(type, 0);
}

void ShadowMapArray::begin() {
    s_fbo->bind();
    s_shadowShader->begin();

    // by default render all scenes, this can be set to a light id, to avoid
    // rendering the lights scene m_mesh (self-shadowing)
    s_shadowShader->setUniform1i("lightIndIsActMesh", -1);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.7f, 4.f);
}

void ShadowMapArray::end() {
    glDisable(GL_POLYGON_OFFSET_FILL);
    s_shadowShader->end();
    s_fbo->unbind();
}

void ShadowMapArray::setNrLights(uint _nrLights) {
    if (nrLights != _nrLights) {
        rebuildFbo(_nrLights);
        rebuildShader(_nrLights);
    }

    nrLights = _nrLights;
}

void ShadowMapArray::setScreenSize(uint _width, uint _height) {
    s_scrWidth  = _width;
    s_scrHeight = _height;
    rebuildFbo(nrLights);
}

void ShadowMapArray::bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs) {
    for (uint i = 0; i < nrTexs; i++) {
        glActiveTexture(GL_TEXTURE0 + baseTexUnit + i);
        glBindTexture(GL_TEXTURE_2D, depthTexViews[texOffs + i]);
    }
}

void ShadowMapArray::clear() {
    if (s_fbo) {
        s_fbo->bind();

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepthf(1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        s_fbo->unbind();
    }
}
}  // namespace ara
