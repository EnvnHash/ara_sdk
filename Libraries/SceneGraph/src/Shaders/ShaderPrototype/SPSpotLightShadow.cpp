//
//  SPSpotLightShadow.cpp
//
//  Created by Sven Hahne on 11.08.17.
//

#include <Shaders/ShaderPrototype/SPSpotLightShadow.h>
#include <Lights/Light.h>
#include <CameraSets/CameraSet.h>

using namespace glm;
using namespace std;

namespace ara {

SPSpotLightShadow::SPSpotLightShadow(sceneData *sd) : ShaderProto(sd), m_nrActSurfPasses(0), m_nrLightPasses(0) {
    s_name = getTypeName<SPSpotLightShadow>();

    // use a ShaderBuffer to transfer the LighParameters
    m_lightSb = make_unique<ShaderBuffer<LightPar>>(1);

    // get max nr Texture Binds
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_max_tex_units);

    m_maxNrParLights = (m_max_tex_units - 1) / 2;
}

void SPSpotLightShadow::rebuildShader() {
    string vert = ara::ShaderCollector::getShaderHeader() + "// SPSpotLightShadow Light Prototype\n";
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 1) in vec4 normal; \n
        layout(location = 2) in vec2 texCoord; \n
        layout(location = 3) in vec4 color; \n);

    if (!s_lights.empty()) {
        vert += s_lights[0]->getLightShaderBlock();  // add LightPar BufferBlock
    }

    for (uint i = 0; i < 4; i++) {
        vert += "uniform mat4 " + getStdMatrixNames()[i] + "; \n";
    }

    vert += "uniform mat3 " + getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] +
            "; \n"
            "uniform mat4 shadow_matrix[" +
            to_string(m_maxNrParLights) +
            "]; \n"
            "uniform int nrLights;\n"
            "\n"
            "out VS_FS { \n"
            "vec4 shadow_coord[" +
            to_string(m_maxNrParLights) +
            "]; \n"
            "vec4 rawPos; \n"
            "vec3 normal; \n"
            "vec2 tex_coord; \n"
            "vec4 color; \n"
            "} vertex_out; \n"
            "\n"
            "void main() { \n"
            "vec4 wPos = " +
            getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
            " * position; \n"
            "vertex_out.rawPos = wPos; \n"
            "wPos = " +
            getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] +
            " * wPos; \n"
            "for (int i=0;i<nrLights; i++) {\n"
            "vertex_out.shadow_coord[i] = shadow_matrix[i] * position;\n"
            "} \n"
            "vertex_out.tex_coord = texCoord; \n"
            "vertex_out.color = color; \n"
            "vertex_out.normal = normalize(" +
            getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] +
            " * normal.xyz); \n"
            "\n"
            "gl_Position = " +
            getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + " * " +
            getStdMatrixNames()[toType(StdMatNameInd::ViewMat)] +
            " * wPos; \n"
            "}";

    //------------------------------------------------------------------

    string frag = ShaderCollector::getShaderHeader();
    if (!s_lights.empty()) {
        frag += s_lights[0]->getLightShaderBlock();  // add LightPar BufferBlock
    }

    frag +=
        "uniform vec4 ambient; \n"     // material parameter, ambient amount
        "uniform vec4 diffuse; \n"     // material parameter
        "uniform vec4 emissive; \n"    // material parameter
        "uniform vec4 specular; \n"    // material parameter
        "uniform float shininess; \n"  // exponent for sharping highlights
        "uniform float strength; \n"   // extra factor to adjust shininess
        "uniform int nrLights;\n"
        "uniform sampler2D tex;\n"
        "uniform sampler2DShadow depth_tex[" +
        to_string(m_maxNrParLights) +
        "];\n"
        "uniform sampler2D light_col_tex[" +
        to_string(m_maxNrParLights) +
        "];\n"
        "uniform int hasTexture;\n"
        "uniform int lightMode;\n"
        "uniform float highLight;\n"
        "uniform int lightIndIsActMesh;\n"

        "in VS_FS {\n"
        "vec4 shadow_coord[" +
        to_string(m_maxNrParLights) +
        "]; \n"
        "vec4 rawPos; \n"
        "vec3 normal; \n"
        "vec2 tex_coord; \n"
        "vec4 color; \n"
        "} vertex_in;\n";

    frag += STRINGIFY(
        vec4 procCol; \n out vec4 fragColor; \n

            vec4 dirLight(vec4 texCol, int ind) {
                \n
                    // compute cosine of the directions, using dot products,
                    // to see how much light would be reflected
                    // calculate normal in both directions
                    float diffuseAmt  = max(0.0, dot(vertex_in.normal, -lightPars[ind].LDirection.xyz));
                \n float  specularAmt = max(0.0, dot(vertex_in.normal, -lightPars[ind].halfVector.xyz));
                \n

                    if (diffuseAmt == 0.0)\n specularAmt = 0.0;
                \n else \n                   specularAmt = pow(specularAmt, shininess);  // sharpen the highlight\n

                vec4    baseCol        = hasTexture == 1 ? texCol + diffuse : diffuse;
                \n vec3 ambientLight   = ambient.rgb * lightPars[ind].ambientColor.rgb;
                \n vec3 scatteredLight = ambientLight + baseCol.rgb * diffuseAmt;
                \n vec3 reflectedLight = lightPars[ind].LColor.rgb * specularAmt * specular.rgb * strength;
                \n return vec4(max(scatteredLight + reflectedLight + emissive.rgb, vec3(0.0)), 0.0);
                \n
            }\n\n

                vec4 spot(vec4 texCol, float shadowVal, int ind) {
                    \n
                        // find the direction and distance of the light,
                        // which changes fragment to fragment for a local light
                        vec3 lightDirection = lightPars[ind].LPosition.xyz - vertex_in.rawPos.xyz;
                    \n float lightDistance  = length(lightDirection);
                    \n

                        // normalize the light direction vector, so
                        // that a dot products give cosines
                        lightDirection = lightDirection / lightDistance;
                    \n

                        // model how much light is available for this fragment
                        float attenuation = 1.0 / (lightPars[ind].constantAttenuation +
                                                   lightPars[ind].linearAttenuation * lightDistance +
                                                   lightPars[ind].quadraticAttenuation * lightDistance * lightDistance);
                    \n

                        // how close are we to being in the spot?
                        float spotCos = dot(lightDirection, -lightPars[ind].coneDirection.xyz);
                    \n

                        // attenuate more, based on spot-relative position
                        if (spotCos < lightPars[ind].spotCosCutoff)\n attenuation = 0.0;
                    \n else \n attenuation *= pow(spotCos, lightPars[ind].spotExponent);
                    \n

                        // the direction of maximum highlight also changes per
                        // fragment
                        vec3 halfVector  = normalize(lightDirection + lightPars[ind].eyeDirection.xyz);
                    \n float diffuseAmt  = max(0.0, dot(vertex_in.normal, lightDirection));
                    \n float specularAmt = max(0.0, dot(vertex_in.normal, lightPars[ind].halfVector.xyz));
                    \n

                        if (diffuseAmt == 0.0)  \n specularAmt = 0.0;
                    \n else \n                     specularAmt = pow(specularAmt, shininess) * strength;
                    \n

                        vec4 baseCol        = hasTexture == 1 ? texCol * emissive : diffuse;
                    \n vec3  ambientLight   = ambient.rgb * lightPars[ind].ambientColor.rgb;
                    \n vec3  scatteredLight = ambientLight + baseCol.rgb * diffuseAmt * attenuation * shadowVal;
                    \n vec3  reflectedLight =
                        lightPars[ind].LColor.rgb * specularAmt * specular.rgb * attenuation * strength * shadowVal;
                    \n reflectedLight = (scatteredLight + reflectedLight +
                                         (hasTexture == 1 ? texCol.rgb * emissive.rgb : emissive.rgb));
                    \n

                        return vec4(max(reflectedLight, vec3(0.0)), 0.0);
                    \n
                }\n\n

                    vec4 projector(float shadowVal, int ind) {
                        \n vec4  proj_tex_coord = vertex_in.shadow_coord[ind];
                        \n float bright         = (lightPars[ind].aspect * 18.0) /
                                          pow(tan(lightPars[ind].fovY) * 1.0 * proj_tex_coord.z, 2.0);
                        \n      proj_tex_coord /= proj_tex_coord.w;
                        \n vec4 outCol = texture(light_col_tex[ind], proj_tex_coord.xy) * shadowVal * bright;
                        \n return outCol;
                        \n
                    }\n\n

        void main() {
            \n
			\n vec4    texCol  = texture(tex, vertex_in.tex_coord);
            \n procCol = vec4(0.0);
            \n for (int i = 0; i < nrLights; i++)\n {
                \n float shadow = textureProj(depth_tex[i], vertex_in.shadow_coord[i]);
                \n       procCol += lightMode == 0 ? dirLight(texCol, i)
                                    : int(lightPars[i].lightMode) == 0
                                        ? (i != lightIndIsActMesh ? spot(texCol, shadow, i) : emissive)
                                        : (i != lightIndIsActMesh ? projector(shadow, i) : vec4(0.0));
                \n
            }
            \n

                float alpha =
                    (vertex_in.color.a + (hasTexture == 1 ? texCol.a : 0.0) + diffuse.a) + min(procCol.a, 1.0);
            \n if (alpha < 0.001) discard;
            \n  // performance optimization. may be critical with alpha
                // blending...
                fragColor = vec4(procCol.rgb + highLight * vec3(0.75), alpha);
            \n
        });

    s_shader = s_shCol->add("SPSpotLightShadow", vert, frag);
}

///< called every time when a new light is added with proto

void SPSpotLightShadow::calcLights(CameraSet *cs, renderPass pass) {
    if (static_cast<uint>(s_lights.size()) != s_nrLights || !s_shader) {
        if (m_shadowGen) {
            m_shadowGen->rebuildFbo(static_cast<uint>(s_lights.size()));
            m_shadowGen->rebuildShader(static_cast<uint>(s_lights.size()));
            if (pass == GLSG_SHADOW_MAP_PASS) {
                m_shadowGen->begin();  // when the shader is rebuilt it is also unbound - so bind it again.
            }
        }

        if (!s_shader) {
            rebuildShader();
        }

        m_lightSb->resize(static_cast<uint>(s_lights.size()));
    }

    s_nrLights = static_cast<uint>(s_lights.size());

    // update ShaderBuffer
    auto ptr = m_lightSb->map(GL_MAP_WRITE_BIT);
    for (const auto &it : s_lights) {
        calcLight(cs, it, ptr);
        ++ptr;
    }
    m_lightSb->unmap();
}

void SPSpotLightShadow::calcLight(CameraSet *cs, Light *lightPtr, LightPar *lightParPtr) {
    m_halfVector = glm::normalize(lightPtr->s_direction + cs->getViewerVec());
    lightPtr->getLightShdrProp()->setHalfVector(m_halfVector.x, m_halfVector.y, m_halfVector.z);

    lightParPtr->ambientColor         = *((vec4 *)lightPtr->getPtr("ambientColor"));
    lightParPtr->LColor               = *((vec4 *)lightPtr->getPtr("LColor"));
    lightParPtr->LDirection           = *((vec4 *)lightPtr->getPtr("LDirection"));
    lightParPtr->LPosition            = *((vec4 *)lightPtr->getPtr("LPosition"));
    lightParPtr->halfVector           = *((vec4 *)lightPtr->getPtr("halfVector"));
    lightParPtr->eyeDirection         = *((vec4 *)lightPtr->getPtr("eyeDirection"));
    lightParPtr->coneDirection        = *((vec4 *)lightPtr->getPtr("coneDirection"));
    lightParPtr->constantAttenuation  = *((float *)lightPtr->getPtr("constantAttenuation"));
    lightParPtr->linearAttenuation    = *((float *)lightPtr->getPtr("linearAttenuation"));
    lightParPtr->quadraticAttenuation = *((float *)lightPtr->getPtr("quadraticAttenuation"));
    lightParPtr->spotCosCutoff        = *((float *)lightPtr->getPtr("spotCosCutoff"));
    lightParPtr->spotExponent         = *((float *)lightPtr->getPtr("spotExponent"));
    lightParPtr->lightMode            = *((float *)lightPtr->getPtr("lightMode"));
    lightParPtr->aspect               = *((float *)lightPtr->getPtr("m_aspect"));
    lightParPtr->throwRatio           = *((float *)lightPtr->getPtr("throwRatio"));
}

void SPSpotLightShadow::clear(renderPass pass) {
    if (pass == GLSG_SHADOW_MAP_PASS && m_shadowGen) {
        m_shadowGen->clear();
    }
}

void SPSpotLightShadow::sendPar(CameraSet *cs, double time, SceneNode *node, SceneNode *parent, renderPass pass,
                                uint loopNr) {
    // check if it is necessary to rebuild all light parameters and shadow maps
    if (s_reqCalcLights) {
        calcLights(cs, pass);
        s_reqCalcLights = false;
    }

    // check if it is necessary to rebuild a individual light (parameters and shadow maps)
    for (size_t i = 0; i < s_lights.size(); i++)
        if (s_lights[i]->s_needsRecalc.load()) {
            auto ptr = m_lightSb->map(GL_MAP_WRITE_BIT);
            ptr += i;
            calcLight(cs, s_lights[i], ptr);
            m_lightSb->unmap();

            s_lights[i]->s_needsRecalc = false;
        }

    ShaderProto::sendPar(cs, time, node, parent, pass);

    if (pass == GLSG_SHADOW_MAP_PASS) {
        // send projection/view matrices for all lights
        if (m_pv_mats.size() != s_lights.size()) {
            m_pv_mats.resize(s_lights.size());
        }

        for (uint i = 0; i < s_lights.size(); i++) {
            m_pv_mats[i] = s_lights[i]->s_proj_mat * s_lights[i]->s_view_mat;

            // check if we are rendering a light source, in this case, avoid that it casts light onto itself
            if (node->m_nodeType == GLSG_SNT_LIGHT_SCENE_MESH && s_lights[i] == parent) {
                m_shadowGen->getShader()->setUniform1i("lightIndIsActMesh", i);
            }
        }

        m_shadowGen->getShader()->setUniformMatrix4fv("m_pv", &m_pv_mats[0][0][0], static_cast<uint>(s_lights.size()));
        m_shadowGen->getShader()->setUniformMatrix4fv(getStdMatrixNames()[toType(StdMatNameInd::ModelMat)],
                                                    value_ptr(node->getModelMat(parent)));

    } else if ((pass == GLSG_SCENE_PASS || pass == GLSG_GIZMO_PASS) && s_shader) {
        // estimate nr passes to render all active surfaces and lights
        // one sceneNode con only contain one active surface, so the maximum of
        // active surfacs textures bound is one
        if (loopNr == 0) {
            // nr of active surfaces
            if (s_lights.size() <= m_maxNrParLights) {
                s_nrPasses = 1;
            } else {
                auto div  = static_cast<float>(s_lights.size()) / static_cast<float>(m_maxNrParLights);
                auto frac = fmod(div, 1.f);
                s_nrPasses = static_cast<uint>(round(div + (frac > 0.f ? 0.5f : 0.f)));
            }
        }

        //-------------------------------------------------------------------------------

        uint lightOffs        = loopNr * static_cast<int>(m_maxNrParLights);
        uint nrLightsThisPass = std::min<int>(static_cast<int>(s_lights.size() - lightOffs), static_cast<int>(m_maxNrParLights));
        s_shader->setUniform1i("lightIndIsActMesh", -1);

        if (m_shadowMat.size() != nrLightsThisPass) {
            m_shadowMat.resize(nrLightsThisPass);
            m_depthTexUnits.resize(nrLightsThisPass);
            m_lightColTexUnits.resize(nrLightsThisPass);
        }

        for (uint i = 0; i < nrLightsThisPass; i++) {
            m_shadowMat[i]        = s_lights[i + lightOffs]->s_shadow_mat * node->getModelMat(parent);
            m_depthTexUnits[i]    = i + 1;
            m_lightColTexUnits[i] = m_maxNrParLights + i + 1;

            // check if we are rendering a light source, in this case, avoid that it casts light onto itself
            if (node->m_nodeType == GLSG_SNT_LIGHT_SCENE_MESH && s_lights[i + lightOffs] == parent)
                s_shader->setUniform1i("lightIndIsActMesh", i);

            // bind light color textures
            if (s_lights[i + lightOffs]->getColTex() != 0) {
                glActiveTexture(GL_TEXTURE0 + (m_maxNrParLights + i + 1));
                glBindTexture(GL_TEXTURE_2D, s_lights[i + lightOffs]->getColTex());
            }
        }

        s_shader->setUniformMatrix4fv("shadow_matrix", &m_shadowMat[0][0][0], nrLightsThisPass);
        s_shader->setUniform1iv("depth_tex", &m_depthTexUnits[0], nrLightsThisPass);
        s_shader->setUniform1iv("light_col_tex", &m_lightColTexUnits[0], nrLightsThisPass);
        s_shader->setUniform1i("nrLights", (int)nrLightsThisPass);
        s_shader->setUniform1i("lightMode", 1);

        // bind depth textures
        m_shadowGen->bindDepthTexViews(1, nrLightsThisPass, lightOffs);

        // bind Light Parameters
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_lightSb->getBuffer());
        glDepthMask(!(s_nrPasses > 1 && loopNr < s_nrPasses - 1));
    }
}

bool SPSpotLightShadow::begin(CameraSet *cs, renderPass pass, uint loopNr) {
    // init a ShadowMap Generator. Has to be done here, since the FboSize is not known before ShadowMapArray ->
    // one ShadowMap generates all ShadowMaps for all lights
    if (!m_shadowGen)
        m_shadowGen = make_unique<ShadowMapArray>(cs, (int)cs->getActFboSize()->x, (int)cs->getActFboSize()->y,
                                                static_cast<uint>(s_lights.size()));

    switch (pass) {
        case GLSG_SHADOW_MAP_PASS:
            m_shadowGen->begin();
            return true;
            break;

        case GLSG_SCENE_PASS:
            if (s_shader) s_shader->begin();
            return true;
            break;

        case GLSG_GIZMO_PASS:
            if (s_shader) s_shader->begin();
            return true;
            break;

        default: return false; break;
    }
}

bool SPSpotLightShadow::end(renderPass pass, uint loopNr) {
    if (pass == GLSG_SHADOW_MAP_PASS) {
        m_shadowGen->end();
    } else if (pass == GLSG_SCENE_PASS || pass == GLSG_GIZMO_PASS) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        if (s_shader) {
            s_shader->end();
        }
    }

    return loopNr < (s_nrPasses - 1);
}

Shaders *SPSpotLightShadow::getShader(renderPass pass, uint loopNr) {
    switch (pass) {
        case GLSG_SHADOW_MAP_PASS:
            return m_shadowGen->getShader();
        case GLSG_SCENE_PASS:
            return s_shader ? s_shader : nullptr;
        case GLSG_GIZMO_PASS:
            return s_shader ? s_shader : nullptr;
        default: return nullptr; break;
    }
}

void SPSpotLightShadow::setScreenSize(uint width, uint height) {
    s_scrWidth = width, s_scrHeight = height;
    m_shadowGen->setScreenSize(width, height);
    if (s_sd) {
        s_sd->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
    }
}

}  // namespace ara
