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

ShadowMapArray::ShadowMapArray(CameraSet* cs, int scrWidth, int scrHeight, int32_t initNrLights)
    : ShadowMap(cs->getGLBase()), m_shCol(&cs->getGLBase()->shaderCollector()), m_nrLights(initNrLights) {
    s_cs        = cs;
    s_scrWidth  = scrWidth;
    s_scrHeight = scrHeight;

#ifndef ARA_USE_GLES31
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &m_maxShaderInvoc);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_maxNumberLayers);
#endif

    rebuildFbo(initNrLights);
    rebuildShader(initNrLights);
}

void ShadowMapArray::rebuildShader(uint _nrLights) {
    // since the number of lights used in the scene will vary during runtime we will use ShaderBufferObjects to not have
    // to recompile the shader every time ...and they are faster anyway
    uint limitNrLights = std::max<uint>(std::min<uint>(_nrLights, static_cast<uint>(m_maxShaderInvoc)), 1);

    string vert = ara::ShaderCollector::getShaderHeader() + "// ShadowMapArray vertex Shader\n";

    vert += STRINGIFY(layout(location = 0) in vec4 position; \n
        void main() {\n
        gl_Position = position; \n
    });

    std::string geom = ara::ShaderCollector::getShaderHeader() + "// ShadowMapArray geom Shader\n";
    geom += "layout(triangles, invocations= " + std::to_string(limitNrLights) +
            ") in;\n"
            "layout(triangle_strip, max_vertices = 3) out;\n"
            "uniform mat4 m_pv[" +
            std::to_string(limitNrLights) +
            "];\n"
            "uniform mat4 " + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] + "; \n"
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

    std::string frag = ara::ShaderCollector::getShaderHeader() + "// ShadowMapArray fragment Shader\n";

    frag += STRINGIFY(layout(location = 0) out vec4 color; \n void main() { color = vec4(1.0); });

    if (s_shadowShader) {
        m_shCol->deleteShader("ShadowMapArray");
    }

    s_shadowShader = m_shCol->add("ShadowMapArray", vert, geom, frag);
}

void ShadowMapArray::rebuildFbo(int32_t nrLights) {
    if (s_fbo) {
        s_fbo.release();

        // delete textures views
        for (auto i = 0; i < std::min(nrLights, static_cast<int32_t>(m_depthTexViews.size())); i++) {
            glDeleteTextures(1, &m_depthTexViews[i]);
        }
    }

    // s_fbo for saving the depth information
    s_fbo = make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth * 2, s_scrHeight * 2, std::max<int>(nrLights, 1), GL_RGBA8,
                             GL_TEXTURE_2D_ARRAY, true, 1, 1, 1, GL_CLAMP_TO_BORDER, true});

    // set the necessary texture parameters for the depth textures
    glBindTexture(GL_TEXTURE_2D_ARRAY, s_fbo->getDepthImg());
    setShadowTexPar(GL_TEXTURE_2D_ARRAY);

    // build textures views
    m_depthTexViews.resize(nrLights);
    for (auto i = 0; i < nrLights; i++) {
        glGenTextures(1, &m_depthTexViews[i]);

#ifndef ARA_USE_GLES31
        // Now, create a view of the depth textures
        glTextureView(m_depthTexViews[i],      // New texture view
                      GL_TEXTURE_2D,         // Target for the new view
                      s_fbo->getDepthImg(),  // Original texture
                      GL_DEPTH24_STENCIL8,   // depth component m_format
                      0, 1,                  // All mipmaps, but it's only one
                      i, 1);                 // the specific layer, only one layer
#endif
        glBindTexture(GL_TEXTURE_2D, m_depthTexViews[i]);
        setShadowTexPar(GL_TEXTURE_2D);
    }

    m_nrLights = nrLights;
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
    ara::Shaders::end();
    s_fbo->unbind();
}

void ShadowMapArray::setNrLights(int32_t nrLights) {
    if (m_nrLights != nrLights) {
        rebuildFbo(nrLights);
        rebuildShader(nrLights);
    }

    m_nrLights = nrLights;
}

void ShadowMapArray::setScreenSize(uint width, uint height) {
    s_scrWidth  = static_cast<int32_t>(width);
    s_scrHeight = static_cast<int32_t>(height);
    rebuildFbo(m_nrLights);
}

void ShadowMapArray::bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs) const {
    for (uint i = 0; i < nrTexs; ++i) {
        glActiveTexture(GL_TEXTURE0 + baseTexUnit + i);
        glBindTexture(GL_TEXTURE_2D, m_depthTexViews[texOffs + i]);
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
