/*
 * SNMultKinectPointCloud.cpp
 *
 *  Created on: 12.01.2016
 *      Copyright by Sven Hahne
 *
 *	multiple kinects, should all run in the same resolution
 *	output texture should be square
 * */

#include "SNMultKinectPointCloud.h"
#define STRINGIFY(A) #A

namespace tav
{

SNMultKinectPointCloud::SNMultKinectPointCloud(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), nrDevices(0), maxNrPointAmp(50)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	osc = static_cast<OSCData*>(scd->osc);
    kin = static_cast<KinectInput*>(scd->kin);
    mapConf = static_cast<SNFreenect2Motion::mapConfig*>(scd->fnMapConf);

	nrDevices = kin->getNrDevices();

	texNrAr = new GLint[nrDevices];
	rotMats = new glm::mat4[nrDevices];
}


void SNMultKinectPointCloud::initTrigGrid(camPar* cp)
{
	// init a grid for iterating through the depth textures and drawing them as point clouds
	float divisor = 2.f;
	while (divisor < (static_cast<float>(maxNrPointAmp) / static_cast<float>(nrDevices) / static_cast<float>(cp->nrCams) / divisor))
		divisor += 2.f;

	emitTrigCellSize.x = divisor;
	emitTrigCellSize.y = static_cast<unsigned int>(static_cast<float>(maxNrPointAmp) / static_cast<float>(nrDevices) / static_cast<float>(cp->nrCams) / divisor);
	emitTrigNrPartPerCell =  emitTrigCellSize.x * emitTrigCellSize.y;

	emitTrigGridSize.x = kin->getDepthWidth(0) / emitTrigCellSize.x;
	emitTrigGridSize.y = kin->getDepthWidth(0) / emitTrigCellSize.y;

	GLfloat initEmitPos[emitTrigGridSize.x * emitTrigGridSize.y *4];
	unsigned int posOffset=0;

	// init in Pixeln, weil spaeter texelFetch mit integern
	for (int y=0;y<emitTrigGridSize.y;y++)
	{
		for(int x=0;x<emitTrigGridSize.x;x++)
		{
			initEmitPos[posOffset *4   ] = static_cast<float>(x * emitTrigCellSize.x);
			initEmitPos[posOffset *4 +1] = static_cast<float>(y * emitTrigCellSize.y);
			initEmitPos[posOffset *4 +2] = 0.f;
			initEmitPos[posOffset *4 +3] = 1.f;
			posOffset++;
		}
	}

	emitTrig = new VAO("position:4f", GL_STATIC_DRAW);
	emitTrig->initData(emitTrigGridSize.x * emitTrigGridSize.y, initEmitPos);
}


void SNMultKinectPointCloud::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (!inited)
	{
		initFlipAxisShdr(cp);
		initTrigGrid(cp);
		inited = true;

	} else
	{
		for (short i=0;i<nrDevices;i++)
		{
			texNrAr[i] = mapConf->xtionAssignMap[i];

			rotMats[i] = glm::translate(glm::mat4(1.f), mapConf->trans[ mapConf->xtionAssignMap[i] ] );
			rotMats[i] = glm::scale(rotMats[i], mapConf->scale[ mapConf->xtionAssignMap[i] ]);
			rotMats[i] = glm::rotate(rotMats[i],
					mapConf->rotAngleY[ mapConf->xtionAssignMap[i] ],
					glm::vec3(0.f, 1.f, 0.f));
			rotMats[i] = glm::rotate(rotMats[i],
					mapConf->rotAngleX[ mapConf->xtionAssignMap[i] ],
					glm::vec3(1.f, 0.f, 0.f));
		}

		glm::mat4 rotYMat = glm::rotate(glm::mat4(1.f),
				float(M_PI) * 0.5f,
				glm::vec3(0.f, 1.f, 0.f));

		// use the flipaxis shader
		glEnable(GL_BLEND);

		flipAxisShdr->begin();
		//flipAxisShdr->setIdentMatrix4fv("m_pvm");

		flipAxisShdr->setUniformMatrix4fv("projection_matrix_g", cp->multicam_projection_matrix, cp->nrCams);
		flipAxisShdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix, cp->nrCams );

		flipAxisShdr->setUniformMatrix4fv("rotMats", &rotMats[0][0][0], nrDevices);
		flipAxisShdr->setUniformMatrix4fv("rotY", &rotYMat[0][0]);

		flipAxisShdr->setUniform1i("nrDevices", nrDevices);
		flipAxisShdr->setUniform2f("inTex_Size", float(kin->getDepthWidth()), float(kin->getDepthHeight()));
		flipAxisShdr->setUniform2i("cellSize", emitTrigCellSize.x, emitTrigCellSize.y);
		flipAxisShdr->setUniform1f("maxDepth", mapConf->roomDim->x * 0.5f);		// blick von der seite auf den tunnel
		flipAxisShdr->setUniform1iv("depthTex", texNrAr, nrDevices);
		flipAxisShdr->setUniform3fv("scale", &mapConf->scale[0][0], nrDevices);
		flipAxisShdr->setUniform3fv("trans", &mapConf->trans[0][0], nrDevices);
		flipAxisShdr->setUniform2fv("cropX", &mapConf->cropX[0]);
		flipAxisShdr->setUniform2fv("cropY", &mapConf->cropY[0]);
		flipAxisShdr->setUniform2fv("cropZ", &mapConf->cropZ[0]);
		flipAxisShdr->setUniform2f("kinFov", mapConf->kinFov.x, mapConf->kinFov.y);

		flipAxisShdr->setUniform2f("inTex_Size", float(kin->getDepthWidth()),
				float(kin->getDepthHeight()));

		for (int i=0; i<nrDevices; i++)
		{
			glActiveTexture(GL_TEXTURE0 +i);
			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false, mapConf->xtionAssignMap[i] ));
		}

		emitTrig->draw(GL_POINTS);
	}
}


void SNMultKinectPointCloud::initFlipAxisShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);
	std::string shdr_Header = "#version 410\n\n";

	std::string vert =
			STRINGIFY(layout( location = 0 ) in vec4 position;\n
			void main(void) {\n
				gl_Position = position;\n
			});
	vert = shdr_Header + vert;

	std::string geom =
			STRINGIFY(
					out GS_FS_VERTEX {
		vec4 color;
	} vertex_out;

	uniform mat4 m_pvm;\n
	uniform mat4 rotY;\n
	uniform vec2 inTex_Size;\n

	uniform int nrDevices;
	uniform ivec2 cellSize;
	uniform float maxDepth;
	uniform vec2 kinFov;

	uniform vec2 cropX;\n
	uniform vec2 cropY;\n
	uniform vec2 cropZ;\n

	ivec2 readOffset;
	vec4 depth;
	vec4 tempPos;
	vec3 rwPos;
	vec4 rotPos;
	float scaledDepth;
	int drawSwitch;

	vec3 getKinRealWorldCoord(vec3 inCoord)
	{
		// asus xtion tends to measure lower depth with increasing distance
		// experimental correction
		//float depthScale = 1.0 + pow(inCoord.z * 0.00033, powDepth);
		scaledDepth = inCoord.z;

		float xzFactor = tan(kinFov.x * 0.5) * 2.0;
		float yzFactor = tan(kinFov.y * 0.5) * 2.0;

		return vec3(inCoord.x * 0.5 * scaledDepth * xzFactor,
				inCoord.y * 0.5 * scaledDepth * yzFactor,
				scaledDepth);
	}

	void main()
	{
		gl_ViewportIndex = gl_InvocationID;

		vertex_out.color = vec4(1.0, 1.0, 1.0, 1.0);

		for (int y=0;y<cellSize.y;y++)
		{
			for (int x=0;x<cellSize.x;x++)
			{
				for (int i=0;i<1;i++)
				{
					readOffset = ivec2(gl_in[0].gl_Position.xy) + ivec2(x, y);
					depth = texelFetch(depthTex[i], readOffset, 0);

					vec3 rwPos = vec3(
							gl_in[0].gl_Position.x / inTex_Size.x * 2.0 -1.0,
							(1.0 - (gl_in[0].gl_Position.y / inTex_Size.y)) * 2.0 -1.0,
							depth.r);

					rwPos = getKinRealWorldCoord(rwPos);
					rwPos = (rotMats[i] * vec4(rwPos.xyz, 1.0)).xyz;

					drawSwitch = int(rwPos.x > cropX.x) * int(rwPos.x < cropX.y)
									//* int(rwPos.y > cropY.x) * int(rwPos.y < cropY.y)
									* int(rwPos.z > cropZ.x) * int(rwPos.z < cropZ.y);

					if(drawSwitch > 0)
					{
						// flip x and Z axis
						//rwPos = vec3(rwPos.z, rwPos.y, rwPos.x);

						// rotate around y axis - for debugging
						rwPos = (rotY * vec4(rwPos, 1.0)).xyz;
						rwPos /= maxDepth;

						// schiebe die punkte nach links und rechts in den sichtbaren bereich
						rwPos.z = (abs(rwPos.z) + 1.0) * sign(rwPos.z);
						rwPos.y -= 0.25;
						rwPos.y *= 3.0;
						rwPos.x *= 2.0 * (i==0 ? -1.0 : -1.0);

						vertex_out.color = vec4(1.0, 1.0, 1.0, 1.0);
						//gl_Position = m_pvm * vec4(rwPos, 1.0);

						gl_Position = projection_matrix_g[gl_InvocationID]
									  * view_matrix_g[gl_InvocationID]
									  * vec4(rwPos, 1.0);

						EmitVertex();
						EndPrimitive();
					}
				}
			}
		}
	});


	geom = shdr_Header + "layout (points) in;\n	layout (points, max_vertices = "+std::to_string(maxNrPointAmp)+") out;\n"
			+ "uniform sampler2D depthTex["+std::to_string(nrDevices)+"];\n"
			+ "uniform vec3 scale["+std::to_string(nrDevices)+"];\n"
			+ "uniform vec3 trans["+std::to_string(nrDevices)+"];\n"
			+ "uniform mat4 rotMats["+std::to_string(nrDevices)+"];\n"
			+ "uniform mat4 view_matrix_g["+nrCams+"];\n"
			+ "uniform mat4 projection_matrix_g["+nrCams+"];\n"
			+ geom;

	std::string frag =
			STRINGIFY(uniform sampler2D Data;\n
			in GS_FS_VERTEX
			{
		vec4 color;
			} vertex_out;

			layout(location = 0) out vec4 Color;\n

			void main(void) {\n
				Color = vertex_out.color;\n
			});

	frag = shdr_Header + frag;

	flipAxisShdr = shCol->addCheckShaderText("MultKinectPointCloud", vert.c_str(),
			geom.c_str(), frag.c_str());
}


void SNMultKinectPointCloud::update(double time, double dt)
{}


SNMultKinectPointCloud::~SNMultKinectPointCloud()
{
	delete emitTrig;
	delete texNrAr;
	delete rotMats;
}
}
