/**
 *  Standard ShadowMap Generator
 *
 *  Sven Hahne on oct 2019
 *
 */

#include "ShadowMapVsmArray.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

ShadowMapVsmArray::ShadowMapVsmArray(CameraSet* cs, int scrWidth, int scrHeight, int32_t initNrLights)
    : ShadowMap(cs->getGLBase()), m_blurAlpha(1.f), m_shCol(&cs->getGLBase()->shaderCollector()),
      m_nrLights(initNrLights) {
    s_cs              = cs;
    s_scrWidth        = scrWidth;
    s_scrHeight       = scrHeight;
    s_shadow_map_coef = 1.f;
    m_maxShaderInvoc  = s_glbase->maxShaderInvocations();

    rebuildFbo(initNrLights);
    rebuildShader(initNrLights);
}

void ShadowMapVsmArray::rebuildShader(uint nrLights) {
    if (nrLights > 0) {
        // since the number of lights used in the scene will vary during runtime
        // we will use ShaderBufferObjects to not have to recompile the shader
        // every time
        // ...and they are faster anyway
        uint limitNrLights = std::max<uint>(std::min<uint>(nrLights, static_cast<uint>(m_maxShaderInvoc)), 1);

        string vert = ShaderCollector::getShaderHeader() + "// ShadowMapVsmArray vertex Shader\n";
        vert += STRINGIFY(layout(location = 0) in vec4 position; \n void main() {
            \n gl_Position = position;
            \n
        });

        std::string geom = ShaderCollector::getShaderHeader() + "// ShadowMapVsmArray geom Shader\n";
        geom += "layout(triangles, invocations= " + std::to_string(limitNrLights) +
                ") in;\n"
                "layout(triangle_strip, max_vertices = 3) out;\n"
                "uniform mat4 m_pv[" +
                std::to_string(limitNrLights) +
                "];\n"
                "uniform mat4 " +
                getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
                "; \n"
                "uniform int lightIndIsActMesh;\n"
                "out GS_FS { vec4 position; } v_out; \n";

        geom += STRINGIFY(void main() {
            if (lightIndIsActMesh != gl_InvocationID) {
                for (int i = 0; i < gl_in.length(); i++) {
                    gl_Layer = gl_InvocationID;
                                        vec4 pos = m_pv[gl_InvocationID] *) + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] + STRINGIFY(*gl_in[i].gl_Position;
					gl_Position = pos;
					v_out.position = pos;
					EmitVertex();
                }
                EndPrimitive();
            }
        });

        std::string frag = ShaderCollector::getShaderHeader() + "// ShadowMapVsmArray fragment Shader\n";

        frag += STRINGIFY(
            in GS_FS { vec4 position; } v_in; \n
			\n layout(location = 0) out vec4 color; \n void main() {
                float depth   = v_in.position.z / v_in.position.w;
                depth         = depth * 0.5 + 0.5;  // from NDC z [-1,1] to [0,1]
                float moment1 = depth;
                // Adjusting moments (this is sort of bias per pixel) using
                // derivative
                float dx      = dFdx(depth);
                float dy      = dFdy(depth);
                float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);
                color         = vec4(moment1, moment2, 0.0, 1.0);
            });

        if (s_shadowShader) m_shCol->deleteShader("ShadowMapVsmArray");

        s_shadowShader = m_shCol->add("ShadowMapVsmArray", vert, geom, frag);
    }
}

void ShadowMapVsmArray::rebuildFbo(uint nrLights) {
    if (nrLights > 0) {
        if (s_fbo) {
            s_fbo.reset();
            m_fboBlur.reset();
        }

        // s_fbo for saving the depth information
        s_fbo = make_unique<FBO>(FboInitParams{
            .glbase = s_glbase,
            .width = s_scrWidth,
            .height = s_scrHeight,
            .depth = std::max<int>(nrLights, 1),
            .type = GL_RG32F,
            .target = GL_TEXTURE_2D_ARRAY,
            .depthBuf = true,
            .wrapMode =  GL_CLAMP_TO_BORDER,
            .layered = true
        });

        m_fboBlur = make_unique<FastBlurMem>(FastBlurMemParams{
            .glbase = s_glbase,
            .alpha = m_blurAlpha,
            .blurSize = {s_scrWidth, s_scrHeight},
            .target = GL_TEXTURE_2D_ARRAY,
            .intFormat = GL_RG32F,
            .nrLayers = std::max<uint>(nrLights, 1),
            .kSize = KERNEL_3,
            .singleFbo = true
        });
        m_fboBlur->setOffsScale(0.8f);

        // set the necessary texture parameters for the depth textures
        // get last bound texture
        int lastBound = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &lastBound);

        glBindTexture(GL_TEXTURE_2D_ARRAY, m_fboBlur->getLastResult());
        setShadowTexPar(GL_TEXTURE_2D_ARRAY);

        // bind last bound again
        glBindTexture(GL_TEXTURE_2D_ARRAY, lastBound);
    }
    m_nrLights = nrLights;
}

void ShadowMapVsmArray::setShadowTexPar(GLenum type) {
    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // setup wrapping
    glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(type, 0);
}

void ShadowMapVsmArray::begin() {
    if (s_fbo) {
        s_fbo->bind();
    }
    s_shadowShader->begin();

    // by default render all scenes, this can be set to a light id, to avoid
    // rendering the lights scene m_mesh (self-shadowing)
    s_shadowShader->setUniform1i("lightIndIsActMesh", -1);
}

void ShadowMapVsmArray::end() {
    Shaders::end();
    if (s_fbo) {
        s_fbo->unbind();
    }
}

void ShadowMapVsmArray::setNrLights(uint num) {
    if (m_nrLights != num) {
        rebuildFbo(num);
        rebuildShader(m_nrLights);
    }
    m_nrLights = num;
}

void ShadowMapVsmArray::setScreenSize(uint width, uint height) {
    s_scrWidth  = static_cast<int32_t>(width);
    s_scrHeight = static_cast<int32_t>(height);
    rebuildFbo(m_nrLights);
}

void ShadowMapVsmArray::bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs) const {
    for (uint i = 0; i < nrTexs; i++) {
        glActiveTexture(GL_TEXTURE0 + baseTexUnit + i);
        glBindTexture(GL_TEXTURE_2D, m_depthTexViews[texOffs + i]);
    }
}

void ShadowMapVsmArray::blur() const {
    if (s_fbo && m_fboBlur) {
        m_fboBlur->proc(s_fbo->getColorImg());  // blur
    }
}

void ShadowMapVsmArray::clear() {
    if (s_fbo) {
        s_fbo->bind();

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepthf(1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        s_fbo->unbind();
    }
}
}  // namespace ara
