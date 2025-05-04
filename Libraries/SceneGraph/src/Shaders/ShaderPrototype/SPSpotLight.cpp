//
//  SPSpotLight.cpp
//
//  Created by Sven Hahne on 11.08.17.
//

#include "SPSpotLight.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SPSpotLight::SPSpotLight(sceneData* sd) : ShaderProto(sd) {
    s_name = getTypeName<SPSpotLight>();

    m_lightProp.setAmbientColor(0.04f, 0.04f, 0.04f);
    m_lightProp.setColor(1.0f, 1.0f, 1.0f);
    m_lightProp.setPosition(-1.0f, 4.0f, -2.0f);
    m_lightProp.setConstantAttenuation(0.0f);
    m_lightProp.setLinearAttenuation(0.0f);
    m_lightProp.setQuadraticAttenuation(0.025f);
    m_lightProp.setSpotCosCutoff(0.3f);
    m_lightProp.setSpotExponent(0.002f);

    m_lightDir = normalize(vec3(0.f, 0.5f, 1.f));
    m_lightProp.setEyeDirection(m_lightDir.x, m_lightDir.y, m_lightDir.z);
    m_lightProp.setConeDirection(m_lightDir.x * -1.0f, m_lightDir.y * -1.0f,
                               m_lightDir.z * -1.0f);  // must point opposite of eye dir

    std::string vert = ShaderCollector::getShaderHeader();
    vert += "// SPSpotLight Light Prototype\n";

    vert += STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                      layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;);

    for (uint i = 0; i < 4; i++) vert += "uniform mat4 " + getStdMatrixNames()[i] + "; \n";
    vert += "uniform mat3 " + getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] + "; \n";

    vert += STRINGIFY(
		out VS_FS {
	        vec4    rawPos; \n
	        vec4    color; \n
			vec3 normal;\n
			vec2 tex_coord;\n
		} vertex_out;

		void main() {
	        vertex_out.rawPos    = position;\n
			vertex_out.color     = color;\n
			vertex_out.tex_coord = texCoord;\n
			vertex_out.normal    = m_normal * normal.xyz; \n);

    vert += getStdPvmMult();
    vert += "}";

    std::string frag = ShaderCollector::getShaderHeader();
    frag += "// SPSpotLight Light Prototype\n";

        frag += STRINGIFY(
		uniform vec4 ambient; \n			// material parameter, ambient amount
		uniform vec4 diffuse; \n			// material parameter
		uniform vec4 emissive; \n			// material parameter
		uniform vec4 specular; \n			// material parameter
		uniform float shininess; \n			// exponent for sharping highlights
		uniform float strength; \n			// extra factor to adjust shininess

		uniform vec3 ambientColor; \n		// material parameter
		uniform vec3 LColor; \n				// Light Parameter
		uniform vec3 LPosition; \n			// location of the light, eye space
		uniform vec3 eyeDirection; \n		// sollte der NO_SCALEblickwinkel der kamera sein?
		uniform float constantAttenuation; \n
		uniform float linearAttenuation; \n
		uniform float quadraticAttenuation; \n
		uniform vec3 coneDirection; \n		// adding spotlight attributes
		uniform float spotCosCutoff; \n		// how wide the spot is, as a cosine
		uniform float spotExponent; \n		// control light fall-off in the spot

		uniform sampler2D tex;

		in VS_FS {
			vec4 rawPos; \n
			vec4 color; \n
			vec3 normal; \n
			vec2 tex_coord; \n
		} vertex_in;

		out vec4 fragColor; \n

		void main() {
			\n
			// find the direction and distance of the light,
			// which changes fragment to fragment for a local light
			vec3 lightDirection = LPosition - vec3(vertex_in.rawPos); \n
			float lightDistance = length(lightDirection); \n

			// normalize the light direction vector, so
			// that a dot products give cosines
			lightDirection = lightDirection / lightDistance; \n

			// model how much light is available for this fragment
			float attenuation = 1.0 / (constantAttenuation + linearAttenuation * lightDistance + quadraticAttenuation * lightDistance * lightDistance); \n

			// how close are we to being in the spot?
			float spotCos = dot(lightDirection, -coneDirection); \n

			// attenuate more, based on spot-relative position
			if (spotCos < spotCosCutoff)
				attenuation = 0.0;
			else
				attenuation *= pow(spotCos, spotExponent); \n

			// the direction of maximum highlight also changes per fragment
			vec3 halfVector = normalize(lightDirection + eyeDirection); \n
			float diffuseAmt = max(0.0, dot(vertex_in.normal, lightDirection)); \n
			float specularAmt = max(0.0, dot(vertex_in.normal, halfVector)); \n

			if (diffuseAmt == 0.0)  \n
				specularAmt = 0.0;  \n
			else \n
				specularAmt = pow(specularAmt, shininess) * strength; \n

			//vec4 texCol = texture(tex, vertex_in.tex_coord); \n
			//vec4 baseCol = texCol + diffuse;
			vec4 baseCol =  diffuse;
			vec3 ambientLight = ambient.rgb * ambientColor.rgb;
			vec3 scatteredLight = ambientLight + baseCol.rgb * diffuseAmt * attenuation; \n
			vec3 reflectedLight = LColor * specularAmt * specular.rgb * attenuation * strength; \n

			float alpha = (vertex_in.color.a + diffuse.a); \n
//			float alpha = (vertex_in.color.a + texCol.a + diffuse.a); \n
			if (alpha < 0.001) discard; \n // performance optimization. may be critical with alpha blending...

			fragColor.rgb = (scatteredLight + reflectedLight) + emissive.rgb; \n
			fragColor.a = alpha;
	});

    s_shader = s_shCol->add("SPSpotLight", vert, frag);
}

void SPSpotLight::clear(renderPass _pass) {
}

void SPSpotLight::sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr) {
    ShaderProto::sendPar(cs, time, scene, parent, pass);
	if (pass == renderPass::scene) {
		m_lightProp.sendToShader(s_shader->getProgram());
	}
}

bool SPSpotLight::begin(CameraSet* cs, renderPass pass, uint loopNr) {
    switch (pass) {
        case renderPass::shadowMap:
        	return false;
        case renderPass::scene:
            s_shader->begin();
            return true;
        case renderPass::gizmo:
            s_shader->begin();
            return true;
        default:
        	return false;
        }
}

bool SPSpotLight::end(renderPass pass, uint loopNr) {
    if (pass == renderPass::scene ) {
	    Shaders::end();
    }

    return false;
}

Shaders* SPSpotLight::getShader(renderPass pass, uint loopNr) {
    return s_shader;
}

}
