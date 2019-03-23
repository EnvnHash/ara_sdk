//
// SNTunnelCokeBubbles.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "SNTunnelCokeBubbles.h"

namespace tav
{
SNTunnelCokeBubbles::SNTunnelCokeBubbles(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), flWidth(200), flHeight(200), maxNrPart(300000),
		intrv(1.0)
{
	kin = static_cast<KinectInput*>(_scd->kin);
	osc = static_cast<OSCData*>(_scd->osc);
	shCol = static_cast<ShaderCollector*>(shCol);

    nrDevices = kin->getNrDevices();

    lastFrame = new int[nrDevices];
    nisFrameNr = new int[nrDevices];
    nisTexInited = new bool[nrDevices];
    nisTexId = new GLuint[nrDevices];
    for (short i=0;i<nrDevices;i++)
    {
        kin->getNis(i)->setUpdateImg(true);
        nisFrameNr[i] = -1;
        nisTexInited[i] = false;
        nisTexId[i] = 0;
        lastFrame[i] = 0;
    }

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	ps = new GLSLParticleSystemFbo(shCol, maxNrPart,
			float(_scd->screenWidth) / float(_scd->screenHeight));
	ps->setGravity(glm::vec3(0.f));
	ps->setFriction(0.f);
	ps->setLifeTime(8.f);
	ps->setAging(true);
    ps->setAgeFading(true);
    // emit with depth texture
    data.emitVel = normalize(glm::vec3(0.f, 1.f, 0.f));
    // data.posRand = 0.01f;
    data.dirRand = 0.2f;
    data.speed = 0.05f;
    data.speedRand = 0.02f;
    data.size = 0.005f;
    data.sizeRand = 0.5f;
    data.emitCol = glm::vec4(1.f);
    ps->setEmitData(&data);
    
	litTex = new TextureManager();
	litTex->loadTexture2D((*scd->dataPath) + "textures/litspheres/Unknown-38.jpeg");

	bumpTex = new TextureManager();
	bumpTex->loadTexture2D((*scd->dataPath) + "textures/sphere_norm.png");

	bubbleTex = new TextureManager();
    bubbleTex->loadTexture2D((*scd->dataPath) + "textures/smooth_circle2.tif");
	//bubbleTex->loadTexture2D((*scd->dataPath) + "textures/bubble.png");
//    bubbleTex->loadTexture2D((*scd->dataPath)+"textures/marriott/stars_one.tif");

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube(
			((*scd->dataPath) + "textures/coke_skybox.png").c_str());

    emitTex = new TextureManager();
    emitTex->loadTexture2D((*_scd->dataPath)+"/textures/test_emit.png");
    
	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
}

//----------------------------------------------------

void SNTunnelCokeBubbles::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);

	if (!inited)
	{
		initQuadShader(cp->nrCams);
		inited = true;
	}

//	glClear(GL_DEPTH_BUFFER_BIT);
//	glEnable(GL_DEPTH_TEST);
    
    glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_CULL_FACE);	// wichtig!!!
	glEnable(GL_DEPTH_BOUNDS_TEST_EXT);     // is needed for drawing cubes

/*
	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	glActiveTexture(GL_TEXTURE0);
	texShader->setUniform1i("tex", 0);
//	glBindTexture(GL_TEXTURE_2D, ps->getActPosTex());
    glBindTexture(GL_TEXTURE_2D, nisTexId[0]);
//    glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
    
	quad->draw();
 */

    //ps->draw(cp->mvp);
    //ps->drawQuadDirect(cp->mvp, bubbleTex->getId(), bumpTex->getId(), litTex->getId());

    
	glm::vec4 mCol = glm::vec4(1.0, 1.0, 1.0, 1.0);
	glm::vec3 lightPos = glm::vec3(-1.0, 0.2, 1.0);
	glm::vec4 ambient = glm::vec4(0.5, 0.5, 0.5, 1.0);
	glm::vec4 sceneColor = glm::vec4(0.9, 0.9, 0.9, 0.7);
	glm::vec4 diffuse = glm::vec4(1.0, 1.0, 1.0, 1.0);
	glm::vec4 specular = glm::vec4(0.2, 0.2, 0.2f, 1.0);

	glm::vec3 camPos = glm::vec3(0.f, 0.f, 0.f);

	float shininess = 10.0;

    
    drawShaderQuad->begin();
    drawShaderQuad->setUniformMatrix4fv("m_pvm", (GLfloat*) &cp->multicam_mvp_mat4[0][0][0], cp->nrCams);
    drawShaderQuad->setUniformMatrix3fv("m_normal", (GLfloat*) &cp->multicam_normal_matrix, cp->nrCams);
    
    drawShaderQuad->setUniform2i("cellSize", ps->basePTex.cellSize.x, ps->basePTex.cellSize.y);
    drawShaderQuad->setUniform1i("nrSegColTex", 16);
    drawShaderQuad->setUniform3fv("lightPos", &lightPos[0]);
    drawShaderQuad->setUniform1f("shininess", shininess);
    drawShaderQuad->setUniform4fv("specular", &specular[0]);
    drawShaderQuad->setUniform4fv("diffuse", &diffuse[0]);
    drawShaderQuad->setUniform4fv("ambient", &sceneColor[0]);
    drawShaderQuad->setUniform2i("texSize", ps->basePTex.texSize.x, ps->basePTex.texSize.y);
	drawShaderQuad->setUniform1f("aspect", float(scd->screenHeight) / float(scd->screenWidth));
	drawShaderQuad->setUniform3fv("eyePosW", &camPos[0]);

    
	// Vacuum 1.0, Air 1.0003, Water 1.3333
    // Glass 1.5, Plastic 1.5, Diamond 2.417
	drawShaderQuad->setUniform1f("etaRatio", 1.5);

	// bind particle data
	for (short i = 0; i < 4; i++)
	{
		drawShaderQuad->setUniform1i(samplerNames[i], i);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, ps->getPTexSrcId(i));
	}

	glActiveTexture(GL_TEXTURE0 + 4);
	drawShaderQuad->setUniform1i("colTex", 4);
	glBindTexture(GL_TEXTURE_2D, bubbleTex->getId());

	glActiveTexture(GL_TEXTURE0 + 5);
	drawShaderQuad->setUniform1i("normalTex", 5);
	glBindTexture(GL_TEXTURE_2D, bumpTex->getId());

	glActiveTexture(GL_TEXTURE0 + 6);
	drawShaderQuad->setUniform1i("litsphereTexture", 6);
	glBindTexture(GL_TEXTURE_2D, litTex->getId());

	glActiveTexture(GL_TEXTURE0 + 7);
	drawShaderQuad->setUniform1i("cubeMap", 7);
	cubeTex->bind(7);

    ps->basePTex.trigVaoPoints->draw(GLSLParticleSystemFbo::POINTS);
    
}

//----------------------------------------------------

void SNTunnelCokeBubbles::update(double time, double dt)
{
	if (inited)
	{
        for (short i=0;i<nrDevices;i++)
        {
            if (kin->isReady() && kin->isDeviceReady(i))
            {
                if (kin->getDepthFrameNr(false, i) != lastFrame[i])
                {
                    lastFrame[i] = kin->getDepthFrameNr(i, false);

                    uploadNisImg(i);
                    kin->uploadDepthImg(false, i);
                }
            }

            if(nisTexInited[i])
            {
                ps->emit(400, nisTexId[i], kin->getDepthWidth(), kin->getDepthHeight());
//                         kin->getDepthTexId(false), kin->getDepthWidth(), kin->getDepthHeight());
            }
        }

        
        /*
		if (time - lastTime > intrv)
		{
			data.emitOrg = glm::vec3(
					getRandF(0.f, 1.f) > 0.f ? -0.2f : 0.2f,
					-1.f,
					getRandF(0.f, 1.f) > 0.f ? -0.2f : 0.2f);
			data.posRand = 0.3f;
			data.emitVel = glm::normalize(glm::vec3(0.f, 1.f, 0.f));
			data.speed = 0.0001f;
			data.speedRand = 0.4f;
			data.size = 0.001f;
			data.sizeRand = 4.0f;
            data.emitCol = glm::vec4(1.f);
			ps->emit(100, &data);

			lastTime = time;
		}
*/
/*
			if(osc->sliderHasNewVal)
			{
				data.posRand = 0.02f;
				data.emitVel = glm::normalize(glm::vec3(0.f, 1.f, 0.f));
				data.speed = 0.001f;
				data.speedRand = 0.2f;
				data.size = 0.002f;
				data.sizeRand = 1.0f;
				data.emitOrg = glm::vec3(osc->sliderX - 0.5f,
						-0.4f,
						osc->sliderY - 0.5f );

				//printf("x: %f y: %f \n", osc->sliderX, osc->sliderY);

				ps->emit(200, &data);
				osc->sliderHasNewVal = false;
			}
*/

		ps->update(time);
		//ps->update(time, velTexID);

		//fluidSim->update();
	}
}

//----------------------------------------------------
    
void SNTunnelCokeBubbles::uploadNisImg(int device)
{
    if (!nisTexInited[device])
    {
        glGenTextures(1, &nisTexId[device]);
        glBindTexture(GL_TEXTURE_2D, nisTexId[device]);
        
        // define inmutable storage
        glTexStorage2D(GL_TEXTURE_2D,
                       1,                 // nr of mipmap levels
                       GL_RGBA8,
                       kin->getDepthWidth(device),
                       kin->getDepthHeight(device));
        
        glTexSubImage2D(GL_TEXTURE_2D,              // target
                        0,                          // First mipmap level
                        0, 0,                       // x and y offset
                        kin->getDepthWidth(device),
                        kin->getDepthHeight(device),                 // width and height
                        GL_BGRA,
                        GL_UNSIGNED_BYTE,
                        kin->getNis(device)->getResImg());
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        nisTexInited[device] = true;
    }
    
    glBindTexture(GL_TEXTURE_2D, nisTexId[device]);
    glTexSubImage2D(GL_TEXTURE_2D,             // target
                    0,                          // First mipmap level
                    0, 0,                       // x and y offset
                    kin->getDepthWidth(device),
                    kin->getDepthHeight(device),              // width and height
                    GL_BGRA,
                    GL_UNSIGNED_BYTE,
                    kin->getNis(device)->getResImg());
}
    
//----------------------------------------------------

void SNTunnelCokeBubbles::initQuadShader(unsigned int nrCams)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout(location = 0) in vec4 position;
			uniform mat4 m_pvm;
			void main() {
			    gl_Position = position;
			});

	vert = "// GLSLParticleSystemFbo  Draw Quad Shader vertex shader\n" + shdr_Header
			+ vert;


	std::string geom = STRINGIFY(
			uniform sampler2D pos_tex;
			uniform sampler2D vel_tex;
			uniform sampler2D col_tex;
			uniform sampler2D aux0_tex;

            uniform int nrSegColTex;
            uniform float aspect;
            uniform ivec2 cellSize;
			uniform float etaRatio;
			uniform vec3 eyePosW;

			mat4 rot_matrix;
			mat4 trans_matrix;
			mat4 scale_matrix;
			vec4 column0;
			vec4 column1;
			vec4 column2;
			vec4 column3;

			out GS_FS_VERTEX
			{
			    vec4 fsColor;
			    vec4 pos;
			    vec2 tex_coord;
			    vec3 R;
			    vec3 T;
			    vec3 I;
			} vertex_out;

			vec4 pPos;
		    vec4 pVel;
		    vec4 pCol;
		    vec4 pAux0;
		    ivec2 texCoord;
			vec4 center;
			vec4 triangPos;
			float size = 0.05;

			void main()
			{
                float sqrtNrSegColTex = sqrt( float(nrSegColTex) );
                float nrSegColSideLen = 1.0 / sqrt( float(nrSegColTex) );

                ivec2 baseTexCoord = ivec2(gl_in[0].gl_Position.xy);

                column0 = vec4(1.0, 0.0, 0.0, 0.0);
                column1 = vec4(0.0, 0.0, 1.0, 0.0);
                column2 = vec4(0.0, -1.0, 0.0, 0.0);
                column3 = vec4(0.0, 0.0, 0.0, 1.0);
                rot_matrix = mat4(column0, column1, column2, column3);

			    // read the textures
                for (int y=0;y<cellSize.y;y++)
                {
                    for (int x=0;x<cellSize.x;x++)
                    {
                        texCoord = baseTexCoord + ivec2(x, y);
                        pAux0 = texelFetch(aux0_tex, texCoord, 0);

			            if (pAux0.x > 0.001)
			            {
                            pPos = texelFetch(pos_tex, texCoord, 0);
			                pCol = texelFetch(col_tex, texCoord, 0);

			                vertex_out.fsColor = pCol;
			                center = vec4(pPos.xyz, 1.0);
			                vertex_out.pos = center;

			                column0 = vec4(cos(pAux0.z), sin(pAux0.z), 0.0, 0.0);
			                column1 = vec4(-sin(pAux0.z), cos(pAux0.z), 0.0, 0.0);
			                column2 = vec4(0.0, 0.0, 1.0, 0.0);
			                column3 = vec4(center.x, center.y, center.z, 1.0);
			                trans_matrix = mat4(column0, column1, column2, column3);

			                vec2 texCoordOff = vec2(float(int(pAux0.w) % int(sqrtNrSegColTex)) / sqrtNrSegColTex,
			                                        float(int(pAux0.w / sqrtNrSegColTex)) / sqrtNrSegColTex );

			                for (int i=0;i<4;i++)
			                {
			    	        	gl_ViewportIndex = gl_InvocationID;

			                    // Ermittle die Position der Koordinaten aux_par.r = size, gl_Position ist der Mittelpunkt
                                center = vec4(((i > 1) ? pAux0.y : -pAux0.y) * aspect,
                                              (i == 0 || i == 2) ? pAux0.y : -pAux0.y,
                                              -1.0, 1.0);
                                
                                triangPos = m_pvm[gl_InvocationID] * trans_matrix * center;

//			                    if (gl_InvocationID != 2)
//				                    	triangPos = m_pvm[gl_InvocationID] * trans_matrix * center;
//				                    else
//				                    	triangPos = m_pvm[gl_InvocationID] * trans_matrix * rot_matrix * center;

			                    gl_Position = triangPos;

			                    vertex_out.tex_coord = vec2((i > 1) ? 1.0 : 0.0,
			                                                (i == 0 || i == 2) ? 1.0 : 0.0);
//			                    vertex_out.tex_coord = vec2((i == 1 || i == 3) ? texCoordOff.x + nrSegColSideLen : texCoordOff.x,
//			                                                (i == 0 || i == 1) ? texCoordOff.y + nrSegColSideLen : texCoordOff.y);


			                    // Compute position and normal in world space
//			                    vertex_out.N = vec3(0.0, 0.0, 1.0);	 // m_normal[gl_InvocationID] * normal
//			                    vertex_out.N = normalize(vertex_out.N);

			                    // Compute the incident and reflected vectors
			                    vertex_out.I = triangPos.xyz - eyePosW;

			                    EmitVertex();
			                }
			                EndPrimitive();
			            }
			        }
			    }
			});

		printf("nrCams: %d \n", nrCams);

    	std::string gHeader = "#version 410\n";
    	gHeader += "layout(points, invocations=" + std::to_string(nrCams)+ ") in;\n";
    	gHeader += "layout(triangle_strip, max_vertices = "+std::to_string(ps->maxGeoAmpTriStrip)+") out;\n";
    	gHeader += "uniform mat4 m_pvm[" +std::to_string(nrCams)+ "];\n";
    	gHeader += "uniform mat3 m_normal[" +std::to_string(nrCams)+ "];\n";
    	geom = gHeader+geom;


		std::string frag = STRINGIFY(
				layout (location = 0) out vec4 color;

				in GS_FS_VERTEX
				{
				    vec4 fsColor;    // aux_par0: (r: size, g: farbIndex (0-1), b: angle, a: textureUnit)
				    vec4 pos;
				    vec2 tex_coord;
				    vec3 R;
				    vec3 T;
				    vec3 I;
				} vertex_in;

				uniform sampler2D colTex;
				uniform sampler2D litsphereTexture;
				uniform sampler2D normalTex;
				uniform samplerCube cubeMap;

				uniform vec3 lightPos;
				uniform float shininess;
				uniform vec4 specular;
				uniform vec4 diffuse;
				uniform vec4 ambient;
				uniform vec4 sceneColor;
				uniform float etaRatio;

				void main()
				{

				    vec4 tex = texture(colTex, vertex_in.tex_coord);
				    vec4 normal = texture(normalTex, vertex_in.tex_coord);

				    vec3 eyeNormal = normal.xyz;
				    vec3 L = normalize(lightPos - vertex_in.pos.xyz);
				    vec3 E = normalize(-vertex_in.pos.xyz); // we are in Eye Coordinates, so EyePos is (0,0,0)
				    vec3 R = normalize(-reflect(L, eyeNormal));
				    //vertex_out.R = reflect(vertex_in.I, vertex_out.N);
				    vec3 T = refract(vertex_in.I, eyeNormal, etaRatio);

					//vec4 reflectedColor = texture(cubeMap, R);
					vec4 refractedColor = texture(cubeMap, T);

				    //calculate Ambient Term:
				    vec4 Iamb = ambient;

				    //calculate Diffuse Term:
				    vec4 Idiff = diffuse * max(dot(eyeNormal, L), 0.0);
				    Idiff = clamp(Idiff, 0.0, 1.0);

				    // calculate Specular Term:
				    vec4 Ispec = specular * pow(max(dot(R, E), 0.0), 0.3 * shininess);
				    Ispec = clamp(Ispec, 0.0, 1.0);

				    vec4 shading = texture(litsphereTexture, vec2(normal.xyz * vec3(0.495) + vec3(0.5)));

				    if (tex.a > 0.01)
				    {
				    	color = ((sceneColor + Iamb + Idiff + Ispec) * shading + Ispec) * tex
				    			+ vec4(refractedColor.rgb, tex.a);
				    } else {
				    	discard;
				    }
                    
                    color.r *= 0.0;
                    color.g *= 0.0;
                    color *= 0.8;
                    color *= vertex_in.fsColor.a;
				    //color = vec4(reflectedColor.rgb, min(sqrt(tex.a * 2.0), 1.0));
				});

		frag = "// GLSLParticleSystemFBO Draw Quad Shader, pos.w = lifetime\n"
				+ shdr_Header + frag;

		drawShaderQuad = shCol->addCheckShaderText("cokeBubblesDrawQuad", vert.c_str(),
				geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTunnelCokeBubbles::setEmitTex(GLuint _emitTexID, int width, int height)
{
	emitTexID = _emitTexID;
	emitTexWidth = width;
	emitTexHeight = height;
}

//----------------------------------------------------

void SNTunnelCokeBubbles::setVelTex(GLuint _velTexID)
{
	velTexID = _velTexID;
}

//----------------------------------------------------

SNTunnelCokeBubbles::~SNTunnelCokeBubbles()
{
	delete quad;
    delete ps;
    delete fluidSim;
}

}
