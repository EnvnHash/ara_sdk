//
//  SPSpotLightShadowVsm.cpp
//
//  Created by Sven Hahne on 23.10.19.
//

#include "Shaders/ShaderPrototype/SPSpotLightShadowVsm.h"
#include "CameraSets/CameraSet.h"
#include "Lights/Light.h"

using namespace glm;
using namespace std;

namespace ara {

SPSpotLightShadowVsm::SPSpotLightShadowVsm(sceneData *sd) : ShaderProto(sd), m_shineThrough(false) {
    s_name                = getTypeName<SPSpotLightShadowVsm>();
    s_maxSceneLightDens   = 30.f;
    s_usesNodeMaterialPar = true;

#ifdef STAGE3D_USE_UB
    s_useUniformBlock = false;
#endif

#ifndef __APPLE__
    // use a ShaderBuffer to transfer the LightParameters
    m_lightSb = make_unique<ShaderBuffer<LightPar>>(1);
#endif

    m_maxNrParLights = (sd->glbase->maxTexUnits() - 1) / 2;
    rebuildShader(1);
}

void SPSpotLightShadowVsm::rebuildShader(uint32_t nrCameras) {
    if (!nrCameras) {
        return;
    }

    string vert = ShaderCollector::getShaderHeader() + "// SPSpotLightShadowVsm Light Prototype\n";
    vert += STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 1) in vec4 normal; \n
        layout(location = 2) in vec2 texCoord; \n
        layout(location = 3) in vec4 color; \n);

#ifndef __APPLE__
    if (!s_lights.empty()) {
        vert += s_lights[0]->getLightShaderBlock();  // add LightPar BufferBlock
    }
#endif

    if (!s_useUniformBlock) {
        vert += "uniform mat4 " + getStdMatrixNames()[toType(StdMatNameInd::ModelMat)] +
                "; \n"
                "uniform mat3 " +
                getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] + "; \n";

        if (!s_lights.empty()) vert += "uniform mat4 shadow_matrix[" + std::to_string(s_lights.size()) + "]; \n";
    } else
        vert += getNodeDataUb(nrCameras);

    vert +=
        "\n"
        "out VS_GS { \n";
    if (!s_lights.empty()) vert += "vec4 shadow_coord[" + std::to_string(s_lights.size()) + "]; \n";
    vert +=
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
        "vertex_out.rawPos = wPos; \n";
    if (!s_lights.empty())
        vert += "for (int i=0;i<" + std::to_string(s_lights.size()) +
                "; i++) vertex_out.shadow_coord[i] = shadow_matrix[i] * "
                "position;\n";
    vert += "vertex_out.normal = normalize(" + getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] +
            " * normal.xyz); \n"
            "vertex_out.tex_coord = texCoord; \n"
            "vertex_out.color = color; \n"
            "gl_Position = position; \n"
            "}";

    //------------------------------------------------------------------------

    std::string geom = ShaderCollector::getShaderHeader() +
                       "layout(triangles, invocations=" + to_string(nrCameras) +
                       ") in;\n"
                       "layout(triangle_strip, max_vertices=3) out;\n"

                       "in VS_GS { \n";
    if (!s_lights.empty()) geom += "vec4 shadow_coord[" + to_string(s_lights.size()) + "]; \n";
    geom +=
        "\tvec4 rawPos; \n"
        "\tvec3 normal; \n"
        "\tvec2 tex_coord; \n"
        "\tvec4 color; \n"
        "} vertex_in[]; \n"

        "out GS_FS { \n";
    if (!s_lights.empty()) geom += "\tvec4 shadow_coord[" + to_string(s_lights.size()) + "]; \n";
    geom +=
        "\tvec4 rawPos; \n"
        "\tvec3 normal; \n"
        "\tvec2 tex_coord; \n"
        "\tvec4 color; \n"
        "\tfloat camDist;\n"  // marcog.g : Added for floor render
        "} vertex_out; \n";

    if (!s_lights.empty()) geom += "uniform mat4 shadow_matrix[" + to_string(s_lights.size()) + "]; \n";

    if (!s_useUniformBlock) {
        geom += "uniform mat4 " + getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] + "[" + to_string(nrCameras) +
                "];\n"
                //"uniform mat4 " +
                // getStdMatrixNames()[toType(StdMatNameInd::ViewMat)] + "[" +
                // to_string(nrCameras) + "];\n"
                "uniform mat4 " +
                getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] + "[" + to_string(nrCameras) +
                "];\n"
                "uniform int skipForInd; \n"
                "uniform int camLimit; \n"
                "uniform vec2 texMin;\n"
                "uniform vec2 texMax;\n"
                "uniform int normTexCoord; \n"
                "uniform ivec2 flipTc; \n"
                "uniform ivec2 invertTc; \n"
                "uniform int fishEye[" +
                to_string(nrCameras) +
                "]; \n"
                "uniform vec4 fishEyeAdjust[" +
                to_string(nrCameras) + "]; \n";
    } else {
        geom += getNodeDataUb(nrCameras);
    }

    geom += ShaderCollector::getFisheyeVertSnippet(nrCameras) + "\n";

    geom +=
        "void main() {\n"
        "\tif(skipForInd != gl_InvocationID && gl_InvocationID <= camLimit) {\n"
        "\t\tbool emitPrimitive = true; \n "
        "\t\tfor (int i = 0; i < gl_in.length(); i++) { \n"
        "\t\t\tgl_Layer = gl_InvocationID; \n"
#ifndef ARA_USE_GLES31  // GLES does not support multiple viewports
        "\t\t\tgl_ViewportIndex = gl_InvocationID; \n"
#endif
        "\t\t\tvec4 wPos = " +
        getStdMatrixNames()[toType(StdMatNameInd::CamModelMat)] +
        "[gl_InvocationID] * vertex_in[i].rawPos; \n"
        "\t\t\tvec4 p = " +
        getStdMatrixNames()[toType(StdMatNameInd::ProjectionMat)] +
        "[gl_InvocationID] * wPos; \n"
        "\t\t\tgl_Position = bool(fishEye[gl_InvocationID]) ? "
        "fishEyePos(wPos): p; \n";
    if (!s_lights.empty())
        geom += "\t\t\tfor(int j=0;j<" + to_string(s_lights.size()) +
                ";j++) vertex_out.shadow_coord[j] = "
                "vertex_in[i].shadow_coord[j]; \n";
    geom +=
        "\t\t\tvertex_out.normal = vertex_in[i].normal; \n"
        "\t\t\tvec2 tc = bool(normTexCoord) ? (vertex_in[i].tex_coord - "
        "texMin) / (texMax - texMin) : vertex_in[i].tex_coord; \n"
        "\t\t\tvertex_out.tex_coord = vec2(bool(flipTc.x) ? 1.0 - tc.x : "
        "(bool(invertTc.x) ? (texMin.x + (texMax.x - tc.x)) : tc.x), \n"
        "\t\t\t\tbool(flipTc.y) ? 1.0 - tc.y : (bool(invertTc.y) ? (texMin.y + "
        "(texMax.y - tc.y)) : tc.y)); \n"
        "\t\t\tvertex_out.color = vertex_in[i].color; \n"
        "\t\t\tvertex_out.rawPos = vertex_in[i].rawPos; \n"
        "\t\t\tvertex_out.camDist=-wPos.z;\n"
        "\t\t\tEmitVertex(); \n"
        "\t\t}\n "
        "\t\tEndPrimitive(); \n"
        "\t} \n"
        "}";

    //------------------------------------------------------------------

    string frag = ShaderCollector::getShaderHeader();

#ifndef __APPLE__
    if (!s_lights.empty()) {
        frag += s_lights[0]->getLightShaderBlock();  // add LightPar BufferBlock
    }
#endif

#ifdef ARA_USE_GLES31
    frag += "highp uniform sampler2DArray shadow_tex;\n";
#else
    frag += "uniform sampler2DArray shadow_tex;\n";
#endif

    frag +=
        "uniform vec4 ambient; \n"      // material parameter, ambient amount
        "uniform vec4 diffuse; \n"      // material parameter
        "uniform vec4 emissive; \n"     // material parameter
        "uniform vec4 specular; \n"     // material parameter
        "uniform float shininess; \n"   // exponent for sharping highlights
        "uniform float strength; \n"    // extra factor to adjust shininess
        "uniform float ambientAmt; \n"  // multiplier to material ambient amount

        "uniform sampler2D tex0;\n"
        "uniform sampler2D tex2;\n"
        "uniform sampler2D tex1;\n";
    if (!s_lights.empty()) frag += "uniform sampler2D light_col_tex[" + to_string(s_lights.size()) + "];\n";

    if (!s_useUniformBlock) {
        frag +=
            "uniform int hasTexture;\n"
            "uniform int lightMode;\n"
            "uniform int shineThrough;\n"
            "uniform float highLight;\n"
            "uniform float maxSceneLightDens;\n"
            "uniform int showProjBright;\n"
            "uniform int lightIndIsActMesh;\n"
            "uniform int isGizmo;\n"
            "uniform int drawGridTexture;\n"
            "uniform int polyFill;\n"
            "uniform int lumaKey;\n"
            "uniform float lumaThresLow;\n"
            "uniform float lumaThresHigh;\n"
            "uniform int isYuv;\n"
            "uniform int isNv12;\n"
            "uniform int depthFromTex;\n"
            "uniform vec2 gridNrSteps;\n"
            "uniform float gridBgAlpha;\n"
            "uniform float gridLineThickness;\n"
            "uniform vec2 texMin;\n"
            "uniform vec2 texMax;\n"
            "uniform vec2 resolution;\n";
    } else {
        frag += getNodeDataUb(nrCameras);
    }

    frag += "float pi=3.14159265358979323846;\n"
        "in GS_FS {\n";

    if (!s_lights.empty()) {
        frag += "vec4 shadow_coord[" + to_string(s_lights.size()) + "]; \n";
    }

    frag += STRINGIFY(vec4 rawPos; \n
	\t	vec3 normal; \n
	\t	vec2 tex_coord; \n
	\t	vec4 color; \n
	\t	float camDist;\n
}
vertex_in; \n\n
    layout(location = 0) out vec4 fragColor;\n
    \n
    // hard coded standard directional light
    vec4    dirLight(vec4 texCol) {    \n
        // compute cosine of the directions, using dot products, to see how much
        // light would be reflected calculate normal in both directions
        vec3 lightSrc = vec3(-0.35, 0.4, 0.6);\n
        vec3  eye      = vec3(0.0, 0.0, 1.0);\n
        // in case of flat shapes, the back and front would be calculated with
        // the same normal. correct this here manually
        vec3 twoSideNormal = gl_FrontFacing ? vertex_in.normal : -vertex_in.normal;
        float diffuseAmt    = max(0.0, dot(twoSideNormal, lightSrc));\n
        float specularAmt   = max(0.0, dot(twoSideNormal, eye - lightSrc)); \n

        if (diffuseAmt == 0.0) { \n
            specularAmt = 0.0; \n
        } else { \n
            specularAmt = pow(specularAmt, shininess); \n  // sharpen the highlight
        }

        vec4 baseCol        = bool(hasTexture) ? texCol + diffuse : diffuse; \n
        vec3  ambientLight   = ambient.rgb; \n
        vec3  scatteredLight = ambientLight + baseCol.rgb * diffuseAmt; \n
        vec3  reflectedLight = specularAmt * specular.rgb * strength; \n
        return vec4(mix(texCol.rgb, (scatteredLight + reflectedLight + emissive.rgb) * ambientAmt, 0.5), 1.0); \n
    }\n\n);

#ifndef __APPLE__
// add this part only if there are light sources
if (!s_lights.empty())
    frag += STRINGIFY(
        vec4 spot(float shadowVal, int ind) {\n
            // find the direction and distance of the light,
            // which changes fragment to fragment for a local light
            vec3 lightDirection = lightPars[ind].LPosition.xyz - vertex_in.rawPos.xyz;\n
            float lightDistance  = length(lightDirection);\n

            // normalize the light direction vector, so
            // that a dot products give cosines
            lightDirection = lightDirection / lightDistance; \n

            // model how much light is available for this fragment
            float attenuation =
                1.0 / (lightPars[ind].constantAttenuation + lightPars[ind].linearAttenuation * lightDistance +
                       lightPars[ind].quadraticAttenuation * lightDistance * lightDistance);\n

            // how close are we to being in the spot?
            float spotCos = dot(lightDirection, -lightPars[ind].coneDirection.xyz);\n

            // attenuate more, based on spot-relative position
            if (spotCos < lightPars[ind].spotCosCutoff) { \n
                attenuation = 0.0; \n
            } else { \n
                attenuation *= pow(spotCos, lightPars[ind].spotExponent); \n
            } \n

            // the direction of maximum highlight also changes per fragment
            vec3 halfVector  = normalize(lightDirection + lightPars[ind].eyeDirection.xyz); \n
            float diffuseAmt  = max(0.0, dot(vertex_in.normal, lightDirection)); \n
            float specularAmt = max(0.0, dot(vertex_in.normal, lightPars[ind].halfVector.xyz)); \n

            if (diffuseAmt == 0.0) { \n
                specularAmt = 0.0; \n
            } else { \n
                specularAmt = pow(specularAmt, shininess) * strength; \n
            }

            // vec4 baseCol = hasTexture == 1 ? texCol * emissive : diffuse;
            // \n
            vec4 baseCol        = diffuse;\n
            vec3  ambientLight   = ambient.rgb * ambientAmt * lightPars[ind].ambientColor.rgb;\n
            vec3  scatteredLight = ambientLight + baseCol.rgb * diffuseAmt * attenuation * shadowVal; \n
            vec3  reflectedLight = lightPars[ind].LColor.rgb * specularAmt * specular.rgb * attenuation * strength * shadowVal; \n
            reflectedLight = (scatteredLight + reflectedLight + emissive.rgb); \n
            // reflectedLight = (scatteredLight + reflectedLight +
            // (hasTexture == 1 ? texCol.rgb * emissive.rgb :
            // emissive.rgb)); \n
            return vec4(max(reflectedLight, vec3(0.0)), 0.0); \n
        }\n\n

        vec4 projector(float shadowVal, int ind) { \n
            vec3 viewDir        = vec3(0.0, 0.0, -1.0);\n
            vec4 proj_tex_coord = vertex_in.shadow_coord[ind];\n
            // light density in relation to distance, projector
            // brightness and throwRatio
            float bright = ((lightPars[ind].throwRatio * lightPars[ind].throwRatio) * lightPars[ind].aspect *
                            lightPars[ind].LColor.r) /
                           (proj_tex_coord.z * proj_tex_coord.z); \n

            // get the absolute normal of this spacial pixel position
            // in case of flat shapes, the back and front would be
            // calculated with the same normal. correct this here
            // manually
            vec3 twoSideNormal = gl_FrontFacing ? vertex_in.normal : -vertex_in.normal;

            // get the vector pointing from the projector to this pixel
            vec3 projectorLightVec = normalize(vertex_in.rawPos.xyz - lightPars[ind].LPosition.xyz);

            // this is still critical, object occlusion doesn't work with
            // planes, here we try to fix it by comparing normals, but this
            // doesn't work in any cases
            bright *= bool(shineThrough) ? 1.0 : float(dot(-twoSideNormal, projectorLightVec) > 0.0);

            // avoid projection into the positive z-direction
            bright *= float(bool(proj_tex_coord.z));

            proj_tex_coord /= proj_tex_coord.w;\n

                return (bool(showProjBright)
                           ? vec4(1.0, 0.6, 0.0, 1.0)
                           : texture(light_col_tex[ind], vec2(proj_tex_coord.x, proj_tex_coord.y))) *
                shadowVal * (bright / maxSceneLightDens);
            // return vec4(1.0, 0.0, 0.0, 1.0);
        }\n\n

        float chebyshevUpperBound(vec4 shadowCoordNDC, int mapInd) {\n
            vec2 moments = texture(shadow_tex, vec3(shadowCoordNDC.xy, float(mapInd))).rg; \n

            // The fragment is either in shadow or penumbra. We now use
            // chebyshev's upperBound to check How likely this pixel is to
            // be lit (p_max)
            float variance = moments.y - (moments.x * moments.x); \n
            variance = max(variance, 0.0000005); \n
            float  d        = moments.x - shadowCoordNDC.z;\n
            float  p_max    = variance / (variance + d * d);\n
            p_max    = max(p_max, float(shadowCoordNDC.z <= moments.x));\n
            p_max    = max(p_max * p_max - 0.01, 0.0);
            return p_max; \n
        }\n\n

        float shadowMapBorder(vec4 shadowCoordNDC) { \n
            // center tex_coordinates
            vec2 uv = shadowCoordNDC.xy * 2.0 - 1.0; \n
            // generate a quadratic frame with a gradient towards the
            // center, add a black border and a tiny gradient for correcting
            // rounding errors in the shadow generator
            return max(min((1.0 - max(abs(uv.x), abs(uv.y))) * 100.0 - 0.4, 1.0), 0.0);\n
        }\n\n);
#endif

frag += STRINGIFY(
    // grid with constant line width
    vec4 grid(vec2 texC, float bgAlpha, float lineThickness) {
        vec2 tc = texC * gridNrSteps;                 // texture coordinates [0;1] multiplied by
                                                      // number of grid steps -> [0;nrGridSteps]
        vec2 speeds = fwidth(tc);                     // texture coordinates -> pixel size at this fragment position
        vec2 range  = 0.5 - abs(mod(tc, 1.0) - 0.5);  // -> [0.0 - 0.5 - 0.0] segments

        // the edges will be lineThickness/2 and not interpolated ... correct
        // this
        vec2  inset  = max(lineThickness, 1.0) * speeds;
        bvec2 isEdge = bvec2(tc.x < 0.5 || tc.x > (gridNrSteps.x - 0.5), tc.y < 0.5 || tc.y > (gridNrSteps.y - 0.5));
        range.x -= isEdge.x ? inset.x : 0.0;
        range.y -= isEdge.y ? inset.y : 0.0;

        vec2  pixelRange = range / speeds;
        float lineWeight = clamp(min(pixelRange.x, pixelRange.y) - lineThickness, 0.0, 1.0);

        vec2 edgeBlend  = vec2(1.0);
        vec2 blendRange = speeds;
        edgeBlend.x     = isEdge.x ? (tc.x < 0.5 ? (tc.x / blendRange.x) : (gridNrSteps.x - tc.x) / blendRange.x) : 1.0;
        edgeBlend.y     = isEdge.y ? (tc.y < 0.5 ? (tc.y / blendRange.y) : (gridNrSteps.y - tc.y) / blendRange.y) : 1.0;

        return vec4(diffuse.rgb, min(mix(diffuse.a, 0.0, lineWeight) * min(edgeBlend.x, edgeBlend.y), diffuse.a));
    }

    vec4 getYuvNv12(vec2 tex_coord, float alp) {\n
        float y = texture(tex0, tex_coord).r;\n
        float u = texture(tex1, tex_coord).r - 0.5;\n
        float v = texture(tex1, tex_coord).g - 0.5;\n
        return vec4(
            (vec3(y + 1.4021 * v, \n y - 0.34482 * u - 0.71405 * v, \n y + 1.7713 * u) - 0.05) * 1.07, \n alp);\n
    }\n

    vec4 getYuv420p(vec2 tex_coord, float alp) { \n
        float y = texture(tex0, tex_coord).r; \n
        float u = texture(tex1, tex_coord).r - 0.5; \n
        float v = texture(tex2, tex_coord).r - 0.5; \n
        \n
        float    r = y + 1.402 * v; \n
        float g = y - 0.344 * u - 0.714 * v; \n
        float b = y + 1.772 * u; \n
        \n
        return vec4(vec3(r, g, b), alp);\n
    }

    void main() {
        \n vec4 texCol =
            bool(isYuv) ? (bool(isNv12) ? getYuvNv12(vertex_in.tex_coord, 1.0) : getYuv420p(vertex_in.tex_coord, 1.0))
                        : texture(tex0, vertex_in.tex_coord);
        vec4 gridColor = bool(drawGridTexture) ? grid(vertex_in.tex_coord, gridBgAlpha, gridLineThickness) : vec4(0.0);
        vec4 procCol =
            bool(polyFill)
                ? (bool(drawGridTexture) ? gridColor
                                         : (bool(isGizmo) ? clamp(diffuse, vec4(0.3), vec4(0.8)) : dirLight(texCol)))
                : diffuse; \n
	);

#ifndef __APPLE__
        if (!s_lights.empty()) {
            // using std::to_string(lights.size()) as a uniform int causes severe performance drawback
            frag += "for (int i=0;i<" + std::to_string(s_lights.size()) + "; i++)\n"
                + STRINGIFY({\n
                    vec4  shadowCoordNDC = vertex_in.shadow_coord[i] / vertex_in.shadow_coord[i].w; \n
                    float shadow = chebyshevUpperBound(shadowCoordNDC, i) * shadowMapBorder(shadowCoordNDC);\n
                    procCol += float(!(!bool(lightMode) || lightIndIsActMesh == i)) *
                              //: int(lightPars[i].lightMode) == 0 ? spot(shadow, i)
                              (float(!bool(drawGridTexture)) * projector(shadow, i)); \n
                }\n);
        }
#endif

        frag += STRINGIFY(
		float alpha = bool(isGizmo) ? 1.0 : ( bool(lumaKey) ? smoothstep(lumaThresLow, lumaThresHigh, dot(texCol.rgb, vec3(1.0))) : procCol.a ); \n
		fragColor = vec4(procCol.rgb + highLight * vec3(0.25), bool(drawGridTexture) ? gridColor.a : alpha); \n

		// since we are always drawing the floorgrid above everything else, with GL_DEPTH_TEST enabled, alpha=0 must result in depthValue =1
		gl_FragDepth = bool(drawGridTexture) ? (gridColor.a > 0.2 ? gl_FragCoord.z : 1.0)
                : (bool(lumaKey) ? (alpha > 0.2 ? gl_FragCoord.z : 1.0) : gl_FragCoord.z);
    });

s_shader = s_shCol->add("SPSpotLightShadowVsm_" + std::to_string(nrCameras), vert, geom, frag);
}

///< called every time when a new light is added with proto

void SPSpotLightShadowVsm::calcLights(CameraSet *cs, renderPass pass) {
    s_nrLights = static_cast<uint>(s_lights.size());

    if (!(static_cast<uint>(s_lights.size()) != s_nrLights || !s_shader)) {
        return;
}
    if (m_shadowGen) {
        if (m_shadowGenBound) {
            m_shadowGen->getFbo()->unbind();
        }

        m_shadowGen->rebuildFbo(static_cast<uint>(s_lights.size()));

        if (m_shadowGenBound) {
            m_shadowGen->getFbo()->bind();
        }

        if (m_shadowGenBound) {
            Shaders::end();
        }

        m_shadowGen->rebuildShader(static_cast<uint>(s_lights.size()));

        if (m_shadowGenBound) {
            m_shadowGen->getShader()->begin();
        }
    }

    rebuildShader(cs->getNrCameras());  // unbounds the actual shaders
    if (m_shadowGenBound) {
        m_shadowGen->getShader()->begin();
    }

    m_lightSb->resize(static_cast<uint>(s_lights.size()));

#ifndef __APPLE__
    // update ShaderBuffer
    auto ptr = (LightPar *)m_lightSb->map(GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    for (const auto &it : s_lights) {
        calcLight(cs, it, ptr);
        ++ptr;
    }
    m_lightSb->unmap();
#endif
}

void SPSpotLightShadowVsm::calcLight(CameraSet *cs, Light *lightPtr, LightPar *lightParPtr) {
#ifndef __APPLE__
    m_halfVector = glm::normalize(lightPtr->s_direction + cs->getViewerVec());
    lightPtr->getLightShdrProp()->setHalfVector(m_halfVector.x, m_halfVector.y, m_halfVector.z);

    lightParPtr->ambientColor         = *(static_cast<vec4 *>(lightPtr->getPtr("ambientColor")));
    lightParPtr->LColor               = *(static_cast<vec4 *>(lightPtr->getPtr("LColor")));
    lightParPtr->LDirection           = *(static_cast<vec4 *>(lightPtr->getPtr("LDirection")));
    lightParPtr->LPosition            = *(static_cast<vec4 *>(lightPtr->getPtr("LPosition")));
    lightParPtr->halfVector           = *(static_cast<vec4 *>(lightPtr->getPtr("halfVector")));
    lightParPtr->eyeDirection         = *(static_cast<vec4 *>(lightPtr->getPtr("eyeDirection")));
    lightParPtr->coneDirection        = *(static_cast<vec4 *>(lightPtr->getPtr("coneDirection")));
    lightParPtr->constantAttenuation  = *(static_cast<float *>(lightPtr->getPtr("constantAttenuation")));
    lightParPtr->linearAttenuation    = *(static_cast<float *>(lightPtr->getPtr("linearAttenuation")));
    lightParPtr->quadraticAttenuation = *(static_cast<float *>(lightPtr->getPtr("quadraticAttenuation")));
    lightParPtr->spotCosCutoff        = *(static_cast<float *>(lightPtr->getPtr("spotCosCutoff")));
    lightParPtr->spotExponent         = *(static_cast<float *>(lightPtr->getPtr("spotExponent")));
    lightParPtr->lightMode            = *(static_cast<float *>(lightPtr->getPtr("lightMode")));
    lightParPtr->aspect               = *(static_cast<float *>(lightPtr->getPtr("aspect")));
    lightParPtr->throwRatio           = *(static_cast<float *>(lightPtr->getPtr("throwRatio")));
#endif
}

void SPSpotLightShadowVsm::clear(renderPass pass) {
    if (pass == renderPass::shadowMap && m_shadowGen) {
        m_shadowGen->clear();
    }
}

void SPSpotLightShadowVsm::sendPar(CameraSet *cs, double time, SceneNode *node, SceneNode *parent, renderPass pass,
                                   uint loopNr) {
    if (!m_shadowGen) {
        return;
    }

    // check if it is necessary to rebuild all light parameters and shadow maps
    if (s_reqCalcLights) {
        calcLights(cs, pass);
        s_reqCalcLights = false;
    }

#ifndef __APPLE__
    // check if it is necessary to rebuild a individual light (parameters and shadow maps)
    for (auto i = 0; i < s_lights.size(); i++)
        if (s_lights[i]->s_needsRecalc.load()) {
            auto ptr = m_lightSb->map(GL_MAP_WRITE_BIT);
            calcLight(cs, s_lights[i], &ptr[i]);
            m_lightSb->unmap();
            s_lights[i]->s_needsRecalc = false;
        }
#endif

    ShaderProto::sendPar(cs, time, node, parent, pass, loopNr);

    if (pass == renderPass::shadowMap) {
        sendParShadowMapPass(node, parent);
    } else if ((pass == renderPass::scene || pass == renderPass::gizmo) && s_shader) {
        sendParSceneAndGizmoPass(node, parent, loopNr);
    }
}

void SPSpotLightShadowVsm::sendParShadowMapPass(SceneNode *node, SceneNode *parent) {
    // send projection/view matrices for all lights
    if (m_pv_mats.size() != s_lights.size()) {
        m_pv_mats.resize(s_lights.size());
    }

    for (uint i = 0; i < s_lights.size(); ++i) {
        m_pv_mats[i] = s_lights[i]->s_proj_mat * s_lights[i]->s_view_mat;

        // check if we are rendering a light source, in this case, avoid that it casts light onto itself
        if (node->m_nodeType == sceneNodeType::lightSceneMesh && s_lights[i] == parent) {
            m_shadowGen->getShader()->setUniform1i("lightIndIsActMesh", i);
        }
    }

    if (!s_lights.empty() && m_shadowGen->getShader()) {
        m_shadowGen->getShader()->setUniformMatrix4fv("m_pv", &m_pv_mats[0][0][0], static_cast<int>(s_lights.size()));
        m_shadowGen->getShader()->setUniformMatrix4fv(getStdMatrixNames()[toType(StdMatNameInd::ModelMat)], value_ptr(node->getModelMat(parent)));
    }
}

void SPSpotLightShadowVsm::estimateNumPasses(uint loopNr) {
    // estimate nr passes to render all active surfaces and lights one sceneNode con only contain one active surface,
    // so the maximum of active surfaces textures bound is one
    if (loopNr == 0) {
        if (s_lights.size() > m_maxNrParLights) {
            float div  = static_cast<float>(s_lights.size()) / static_cast<float>(m_maxNrParLights);
            float frac = fmod(div, 1.f);
            s_nrPasses = static_cast<uint>(round(div + (frac > 0.f ? 0.5f : 0.f)));
        } else {
            s_nrPasses = 1;
        }
    }
}

void SPSpotLightShadowVsm::sendParSceneAndGizmoPass(SceneNode *node, SceneNode *parent, uint loopNr) {
    estimateNumPasses(loopNr);

    if (s_shader && !s_lights.empty()) {
        uint lightOffs        = loopNr * static_cast<int>(m_maxNrParLights);
        int nrLightsThisPass = std::min<int>(static_cast<int>(s_lights.size() - lightOffs), static_cast<int>(m_maxNrParLights));
        s_shader->setUniform1i("lightIndIsActMesh", -1);

        // if the number of lights has changed, adjust the shadowTexture lists
        if (m_shadowMat.size() != nrLightsThisPass) {
            m_shadowMat.resize(nrLightsThisPass);
            m_lightColTexUnits.resize(nrLightsThisPass);
        }

        sendLightPar(node, parent, nrLightsThisPass, lightOffs);

        s_shader->setUniformMatrix4fv("shadow_matrix", &m_shadowMat[0][0][0], nrLightsThisPass);
        s_shader->setUniform1i("shadow_tex", 1);
        s_shader->setUniform1iv("light_col_tex", &m_lightColTexUnits[0], nrLightsThisPass);
        s_shader->setUniform1i("lightMode", 1);
        s_shader->setUniform1i("shineThrough", (int)m_shineThrough);
        s_shader->setUniform1i("lumaKey", 0);
        s_shader->setUniform1f("lumaThresLow", 0);
        s_shader->setUniform1f("lumaThresHigh", 0);
        s_shader->setUniform1i("isYuv", 0);         // y
        s_shader->setUniform1i("isNv12", 0);        // y
        s_shader->setUniform1i("depthFromTex", 0);  // y

        // bind depth textures
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowGen->getTex());

        // bind Light Parameters
#ifndef __APPLE__
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_lightSb->getBuffer());
#endif
        glDepthMask(!(s_nrPasses > 1 && loopNr < (s_nrPasses - 1)));
    }

    // default is no textures
    s_shader->setUniform1i("hasTexture", 0);

    // gizmo treating
    s_shader->setUniform1i("isGizmo", parent->m_nodeType == sceneNodeType::gizmo);

    // overwrite material ambient parameter
    s_shader->setUniform1f("ambientAmt", s_ambientBrightness);
    s_shader->setUniform1f("maxSceneLightDens", s_maxSceneLightDens);
    s_shader->setUniform1i("showProjBright", s_showProjBright);
}

void SPSpotLightShadowVsm::sendLightPar(SceneNode *node, SceneNode *parent, int nrLightsThisPass, uint lightOffs) {
    for (int i = 0; i < nrLightsThisPass; i++) {
        m_shadowMat[i]        = s_lights[i + lightOffs]->s_shadow_mat * node->getModelMat(parent);
        m_lightColTexUnits[i] = i + 2;

        // check if we are rendering a light source or a Gizmo, in this case, avoid that it casts light onto itself
        if ((node->m_nodeType == sceneNodeType::lightSceneMesh && s_lights[i + lightOffs] == parent) ||
            parent->m_nodeType == sceneNodeType::gizmo) {
            s_shader->setUniform1i("lightIndIsActMesh", i);
        }

        // bind light color textures
        if (s_lights[i + lightOffs]->getColTex() != 0) {
            glActiveTexture(GL_TEXTURE0 + m_lightColTexUnits[i]);
            glBindTexture(GL_TEXTURE_2D, s_lights[i + lightOffs]->getColTex());
        }
    }
}

bool SPSpotLightShadowVsm::begin(CameraSet *cs, renderPass pass, uint loopNr) {
    // init a ShadowMap Generator. Has to be done here, since the FboSize is not known before ShadowMapArray ->
    // one ShadowMap generates all ShadowMaps for all lights
    if (!m_shadowGen && !s_lights.empty()) {
        m_shadowGen = make_unique<ShadowMapVsmArray>(cs, 512, 512, static_cast<uint>(s_lights.size()));
    }

    switch (pass) {
        case renderPass::shadowMap:
            if (m_shadowGen) {
                m_shadowGenBound = true;
                m_shadowGen->begin();
            }
            return true;
        case renderPass::scene:
            if (s_shader) {
                glEnable(GL_BLEND);  // something is disabling blending before randomly...
                s_shader->begin();
            }
            return true;
        case renderPass::gizmo:
            if (s_shader) {
                s_shader->begin();
            }
            return true;
        default:
            return false;
    }
}

bool SPSpotLightShadowVsm::end(renderPass pass, uint loopNr) {
    if (pass == renderPass::shadowMap) {
        if (m_shadowGen) {
            m_shadowGen->end();
            m_shadowGenBound = false;
        }
    } else if (pass == renderPass::scene || pass == renderPass::gizmo) {
#ifndef __APPLE__
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
#endif
        if (s_shader) {
            Shaders::end();
        }
    }

    return loopNr < (s_nrPasses - 1);
}

void SPSpotLightShadowVsm::postRender(renderPass pass) {
    if (pass == renderPass::shadowMap && m_shadowGen) m_shadowGen->blur();
}

Shaders *SPSpotLightShadowVsm::getShader(renderPass pass, uint loopNr) {
    switch (pass) {
        case renderPass::shadowMap:
            return m_shadowGen ? m_shadowGen->getShader() : nullptr;
        case renderPass::scene:
            return s_shader ? s_shader : nullptr;
        case renderPass::gizmo:
            return s_shader ? s_shader : nullptr;
        default:
            return nullptr;
    }
}

void SPSpotLightShadowVsm::setScreenSize(uint width, uint height) {
    s_scrWidth = width, s_scrHeight = height;
    if (m_shadowGen) {
        m_shadowGen->setScreenSize(width, height);
        if (s_sd) {
            s_sd->reqRenderPasses->at(renderPass::shadowMap) = true;
        }
    }
}

void SPSpotLightShadowVsm::setNrCams(int nrCams) {
    s_nrCams = nrCams;
    rebuildShader(s_nrCams);
}

std::string SPSpotLightShadowVsm::getUbPar(uint32_t nrCameras) {
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