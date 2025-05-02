//
//  SPDirLight.cpp
//
//  Created by Sven Hahne on 11.08.17
//

#include "SPDirLight.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {
SPDirLight::SPDirLight(sceneData* sd) : ShaderProto(sd) {
    s_name = getTypeName<SPDirLight>();

    lightProp.setAmbientColor(0.2f, 0.2f, 0.2f);

    vec3 lightPos = vec3(-3.5f, 1.0f, 0.0f);  // from the object and not to the object
    lightProp.setPosition(lightPos.x, lightPos.y,
                          lightPos.z);  // by scaling this, the intensity varies

    lightDir = normalize(vec3(0.3f, 1.f, 1.0f));  // from the object and not to the object
    lightProp.setDirection(lightDir.x, lightDir.y,
                           lightDir.z);    // by scaling this, the intensity varies
    lightProp.setColor(1.0f, 1.0f, 1.0f);  // LightColor

    // vertex shader
    std::string vert = ShaderCollector::getShaderHeader();
    vert += "//SPDirLight Directional Light Prototype\n";

    vert += STRINGIFY(layout(location = 0) in vec4 position; layout(location = 1) in vec4 normal;
                      layout(location = 2) in vec2 texCoord; layout(location = 3) in vec4 color;);

    for (uint i = 0; i < 4; i++) vert += "uniform mat4 " + getStdMatrixNames()[i] + "; \n";
    vert += "uniform mat3 " + getStdMatrixNames()[toType(StdMatNameInd::NormalMat)] + "; \n";

        vert += STRINGIFY(
	out VS_FS {
        vec4    position;
        \n vec3 normal;
        \n vec2 tex_coord;
        \n vec4 color;
        \n
	} vertex_out;

	void main() {
        vertex_out.position     = position;
        \n vertex_out.tex_coord = texCoord;
        \n vertex_out.normal    = m_normal * normal.xyz;
        \n vertex_out.color     = color; \n);

        vert += getStdPvmMult() + "}";

        std::string frag = ShaderCollector::getShaderHeader();
        frag += "//SPDirLight Directional Light Prototype\n";

        frag += STRINGIFY(
		uniform vec4 ambient; \n		// material definitions
		uniform vec4 diffuse; \n
		uniform vec4 emissive; \n
		uniform vec4 specular; \n
		uniform float shininess; \n		// exponent for sharping highlights
		uniform float strength; \n		// extra factor to adjust shininess
		uniform int hasTexture;

		uniform vec3 LColor; \n			// light definitions
		uniform vec3 halfVector; \n			// light definitions
		uniform vec3 LDirection; \n     // direction toward the light
		uniform vec3 ambientColor; \n
		
		uniform sampler2D tex; \n

		in VS_FS{
			vec4 position; \n
			vec3 normal; \n
			vec2 tex_coord; \n
			vec4 color; \n
		} vertex_in;

		out vec4 fragColor; \n

		void main() {
			vec4 texCol = texture(tex, vertex_in.tex_coord); \n

			// compute cosine of the directions, using dot products,
			// to see how much light would be reflected
			// calculate normal in both directions
			float diffuseAmt = max(0.0, dot(vertex_in.normal, LDirection)); \n
			float specularAmt = max(0.0, dot(vertex_in.normal, halfVector)); \n
			
			if (diffuseAmt == 0.0)
				specularAmt = 0.0;
			else 
				specularAmt = pow(specularAmt, shininess); // sharpen the highlight

			vec4 baseCol = hasTexture == 1 ? texCol + diffuse : diffuse;
			vec3 ambientLight = ambient.rgb * ambientColor;
			vec3 scatteredLight = ambientLight + baseCol.rgb * diffuseAmt; \n
			vec3 reflectedLight = LColor * specularAmt * specular.rgb * strength; \n
			
			float alpha = (vertex_in.color.a + texCol.a); \n
			if (alpha < 0.001) discard; \n // performance optimization. may be critical with alpha blending...

			fragColor = vec4((scatteredLight +reflectedLight), 1.0) + emissive; // quick and dirty brightness correction
			fragColor.a = alpha;
		}
	);

        s_shader = s_shCol->add("SPDirLight", vert, frag);
}


void SPDirLight::clear(renderPass _pass) {}


void SPDirLight::sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr)
{
        halfVector = normalize(lightDir + cs->getViewerVec());
        lightProp.setHalfVector(halfVector.x, halfVector.y, halfVector.z);

        if (pass == GLSG_SCENE_PASS || pass == GLSG_GIZMO_PASS) lightProp.sendToShader(s_shader->getProgram());

        ShaderProto::sendPar(cs, time, scene, parent, pass, loopNr);
}


bool SPDirLight::begin(CameraSet* cs, renderPass pass, uint loopNr)
{
        switch (pass) {
            case GLSG_SHADOW_MAP_PASS: return false; break;

            case GLSG_SCENE_PASS:
                s_shader->begin();
                return true;
                break;

            case GLSG_GIZMO_PASS:
                s_shader->begin();
                return true;
                break;

            default: return false; break;
        }
}


bool SPDirLight::end(renderPass pass, uint loopNr) {
    s_shader->end();
    return false;
}


Shaders* SPDirLight::getShader(renderPass pass, uint loopNr) {
    return s_shader;
}


SPDirLight::~SPDirLight() 
{
}

}
