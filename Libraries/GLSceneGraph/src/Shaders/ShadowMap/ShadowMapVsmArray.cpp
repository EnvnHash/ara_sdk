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

ShadowMapVsmArray::ShadowMapVsmArray(CameraSet* _cs, int _scrWidth, int _scrHeight, uint _initNrLights)
    : ShadowMap(_cs->getGLBase()), shCol(&_cs->getGLBase()->shaderCollector()), nrLights(_initNrLights),
      blurAlpha(1.f) {
    s_cs              = _cs;
    s_scrWidth        = _scrWidth;
    s_scrHeight       = _scrHeight;
    s_shadow_map_coef = 1.f;
    max_shader_invoc  = s_glbase->maxShaderInvocations();

    rebuildFbo(_initNrLights);
    rebuildShader(_initNrLights);
}

void ShadowMapVsmArray::rebuildShader(uint _nrLights) {
    if (_nrLights > 0) {
        // since the number of lights used in the scene will vary during runtime
        // we will use ShaderBufferObjects to not have to recompile the shader
        // every time
        // ...and they are faster anyway
        uint limitNrLights = std::max<uint>(std::min<uint>(_nrLights, (uint)max_shader_invoc), 1);

        string vert = shCol->getShaderHeader() + "// ShadowMapVsmArray vertex Shader\n";
        vert += STRINGIFY(layout(location = 0) in vec4 position; \n void main(void) {
            \n gl_Position = position;
            \n
        });

        std::string geom = shCol->getShaderHeader() + "// ShadowMapVsmArray geom Shader\n";
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

        std::string frag = shCol->getShaderHeader() + "// ShadowMapVsmArray fragment Shader\n";

        frag += STRINGIFY(
            in GS_FS { vec4 position; } v_in; \n
			\n layout(location = 0) out vec4 color; \n void main(void) {
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

        if (s_shadowShader) shCol->deleteShader("ShadowMapVsmArray");

        s_shadowShader = shCol->add("ShadowMapVsmArray", vert.c_str(), geom.c_str(), frag.c_str());
    }
}

void ShadowMapVsmArray::rebuildFbo(uint _nrLights) {
    if (_nrLights > 0) {
        if (s_fbo) {
            s_fbo.reset();
            fboBlur.reset();

            // delete textures views
            // for (uint i = 0; i < (uint)depthTexViews.size(); i++)
            //	glDeleteTextures(1, &depthTexViews[i]);
        }

        // s_fbo for saving the depth information
        s_fbo = make_unique<FBO>(s_glbase, s_scrWidth, s_scrHeight, std::max<uint>(_nrLights, 1), GL_RG32F,
                                 GL_TEXTURE_2D_ARRAY, true, 1, 1, 1, GL_CLAMP_TO_BORDER, true);

        fboBlur = make_unique<FastBlurMem>(s_glbase, blurAlpha, s_scrWidth, s_scrHeight, GL_TEXTURE_2D_ARRAY, GL_RG32F,
                                           std::max<uint>(_nrLights, 1), false, FastBlurMem::KERNEL_3, true);
        fboBlur->setOffsScale(0.8f);

        // set the necessary texture parameters for the depth textures
        // get last bound texture
        int lastBound = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &lastBound);

        glBindTexture(GL_TEXTURE_2D_ARRAY, fboBlur->getLastResult());
        setShadowTexPar(GL_TEXTURE_2D_ARRAY);

        // bind last bound again
        glBindTexture(GL_TEXTURE_2D_ARRAY, lastBound);

        /*
        // build textures views
        depthTexViews.resize(_nrLights);
        for (uint i = 0; i < _nrLights; i++)
        {
            glGenTextures(1, &depthTexViews[i]);

            // Now, create a view of the depth textures
            glTextureView(depthTexViews[i],		// New texture view
                GL_TEXTURE_2D,					// Target for
        the new view fboBlur->getLastResult(),		// Original texture
                GL_RG32F,						// two
        component 32F texture with moments 0, 1,
        // All mipmaps, but it's only one i, 1);
        // the specific layer, only one layer

            glBindTexture(GL_TEXTURE_2D, depthTexViews[i]);
            setShadowTexPar(GL_TEXTURE_2D);
        }*/
    }
    nrLights = _nrLights;
}

void ShadowMapVsmArray::setShadowTexPar(GLenum type) {
    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // setup depth comparison
    // glTexParameteri(type, GL_TEXTURE_COMPARE_MODE,
    // GL_COMPARE_REF_TO_TEXTURE); glTexParameteri(type,
    // GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // setup wrapping
    glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(type, 0);
}

void ShadowMapVsmArray::begin() {
    if (s_fbo) s_fbo->bind();
    s_shadowShader->begin();

    // by default render all scenes, this can be set to a light id, to avoid
    // rendering the lights scene m_mesh (self-shadowing)
    s_shadowShader->setUniform1i("lightIndIsActMesh", -1);
}

void ShadowMapVsmArray::end() {
    s_shadowShader->end();
    if (s_fbo) s_fbo->unbind();
}

void ShadowMapVsmArray::setNrLights(uint _nrLights) {
    if (nrLights != _nrLights) {
        rebuildFbo(_nrLights);
        rebuildShader(_nrLights);
    }

    nrLights = _nrLights;
}

void ShadowMapVsmArray::setScreenSize(uint _width, uint _height) {
    s_scrWidth  = _width;
    s_scrHeight = _height;
    rebuildFbo(nrLights);
}

void ShadowMapVsmArray::bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs) {
    for (uint i = 0; i < nrTexs; i++) {
        glActiveTexture(GL_TEXTURE0 + baseTexUnit + i);
        glBindTexture(GL_TEXTURE_2D, depthTexViews[texOffs + i]);
    }
}

void ShadowMapVsmArray::blur() {
    if (s_fbo && fboBlur) fboBlur->proc(s_fbo->getColorImg());  // blur
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
