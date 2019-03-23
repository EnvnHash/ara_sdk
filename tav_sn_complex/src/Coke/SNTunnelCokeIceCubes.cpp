//
// SNTunnelCokeIceCubes.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "SNTunnelCokeIceCubes.h"

using namespace boost::filesystem;

namespace tav
{
SNTunnelCokeIceCubes::SNTunnelCokeIceCubes(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), flWidth(200), flHeight(200), nrPartH(20), nrPartV(8),
		picWidth(512), picHeight(512), totalNumPics(16),
		bigTexName("cokeBigIce_yellow.png"), scaleFact(1.f),
		destWidth(1280.f), destHeight(720.f)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(shCol);

    maxNrPart = nrPartH * nrPartV;
    
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	ps = new GLSLParticleSystem2(shCol, maxNrPart,
			scd->screenWidth, scd->screenHeight);
	ps->setFriction(0.f);
	ps->setLifeTime(4.f);
	ps->setCheckBounds(false);
	ps->setAging(false);
	ps->setAgeFading(false);
	ps->setAgeSizing(false);
	ps->setVelTexAngleStepSize(1.f / static_cast<float>(flWidth));
	ps->setReturnToOrg(true, 0.07f);
	ps->setGravity(glm::vec3(0.f));
	ps->init();


    fluidSim = new GLSLFluid(false, shCol);
    fluidSim->allocate(flWidth, flHeight, 0.5f);
    fluidSim->dissipation = 0.95f;
    fluidSim->velocityDissipation = 0.98f;
    fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

    forceScale = glm::vec2(static_cast<float>(flWidth) * 0.3f,
                           static_cast<float>(flHeight) * 0.3f);

//
//	litTex = new TextureManager();
//	litTex->loadTexture2D((*scd->dataPath) + "textures/litspheres/white_sphere.jpeg");

	iceTex = new TextureManager();
	iceTex->loadTexture2D((*scd->dataPath) + "textures/ice_texture.png");

	bumpTex = new TextureManager();
	bumpTex->loadTexture2D((*scd->dataPath) + "textures/cokeBigIce_white_bump.png");

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube(
			((*scd->dataPath) + "textures/coke_skybox.png").c_str());


	allPicsTex = new TextureManager();

	// checke ob die grosse bilder textur existier
	// check if calibration file exists, if this is the case load it
	bigTexName = *scd->dataPath+"/textures/"+bigTexName;
	std::string picPath = *scd->dataPath+"/textures/ice_cube_tex";

	// wenn die texture noch nicht existiert
	if (access(bigTexName.c_str(), F_OK) == -1)
	{
		struct recursive_directory_range
		{
		    typedef recursive_directory_iterator iterator;
		    recursive_directory_range(path p) : p_(p) {}

		    iterator begin() { return recursive_directory_iterator(p_); }
		    iterator end() { return recursive_directory_iterator(); }

		    path p_;
		};

		// gehe durch das verzeichnis und lade die Bilder
		std::vector<cv::Mat> pics;
		std::string strs;
		int picCount = 0;
		for (auto it : recursive_directory_range(picPath))
		{
			if( std::strcmp(".DS_Store", it.path().filename().c_str()) != 0
					&& picCount <= totalNumPics)
			{
				pics.push_back( cv::imread( it.path().c_str(), CV_LOAD_IMAGE_UNCHANGED ) );
			    std::cout << "loading: " << it.path() << std::endl;
			    picCount++;
			}
		}

		if(picCount < totalNumPics)
		{
			printf("SNTunnelCokeIceCubes: ES FEHLEN BILDER!!!!, vorhanden: %d \n", picCount);
		} else
		{
			// mache ein grosse Mat fuer alle Bilder
			cv::Mat allPics = cv::Mat::zeros(picHeight * std::sqrt(totalNumPics),
					picWidth * std::sqrt(totalNumPics), CV_8UC4);


			for(short y=0; y<short(std::sqrt(totalNumPics)); y++)
			{
				for(short x=0; x<short(std::sqrt(totalNumPics)); x++)
				{
					short ind = y * short(std::sqrt(totalNumPics)) +x;
					//printf("copying pic ind %d\n", ind);

					pics[ind].copyTo( allPics( cv::Rect(x * picWidth,
							y * picHeight, pics[ind].cols, pics[ind].rows) ) );
				}
			}

#ifdef HAVE_OPENCV
			cv::imwrite(bigTexName, allPics);
			printf("saving big image %s \n", bigTexName.c_str());
#endif
		}
	}
	else
	{
		allPicsTex->loadTexture2D(bigTexName);
		allPicsTex->setFiltering(GL_NEAREST, GL_NEAREST);
	}

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
}

//----------------------------------------------------

void SNTunnelCokeIceCubes::draw(double time, double dt, camPar* cp,
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

		//float aspect = float( scd->screenWidth ) / float( scd->screenHeight );

		for (int y=0;y<nrPartV;y++)
		{
			for (int x=0;x<nrPartH;x++)
			{
				data.emitOrg = glm::vec3(
						(float(x) / float(nrPartH) + 0.5f / float(nrPartH)) * 2.f -1.f,
						(float(y) / float(nrPartV) + 0.5f / float(nrPartV)) * 2.f -1.f,
						getRandF(-0.02f, 0.f));
				data.posRand = glm::vec3(0.1f, 0.1f, 0.1f);
				data.emitVel = glm::vec3(0.0f, 1.f, 0.f);
				data.speed = 0.0;
				data.size = 0.028f;
				data.sizeRand = 0.1f;
				data.texUnit = 1;
				data.texRand = 1.f;
				data.angleRand = 1.f;
				data.colInd = 0.f;

				ps->emit(time, 1, data, true);
			}
		}

		ps->update(time, false);
		inited = true;
	}

	// hardcore reset to one viewport of the last camera
	glViewportIndexedf(0, destWidth * 6.f, 0.f, destWidth, destHeight);

   // fluidSim->drawVelocity();


    if (inited)
    {
    	glDisable(GL_DEPTH_TEST);
    	glEnable(GL_BLEND);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    	glDisable(GL_CULL_FACE);	// wichtig!!!


    	glm::vec3 lightPos = glm::vec3(-0.2, 0.2, 1.0);
    	glm::vec4 diffuse = glm::vec4(0.8, 0.8, 0.8, 1.0);
    	glm::vec4 specular = glm::vec4(0.2, 0.2, 0.2f, 1.0);
    	glm::vec3 camPos = glm::vec3(0.f, 0.f, -1.f);

    	float shininess = 5.0;
    	float scrAspect = 1.f;

        glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), float(time) * 0.3f, glm::vec3(0.f, 1.f, 0.f));


    	drawShaderQuad->begin();
    	drawShaderQuad->setIdentMatrix4fv("m_pvm");
    	drawShaderQuad->setUniformMatrix4fv("tc_rot", &rotMat[0][0]);
//    	drawShaderQuad->setUniformMatrix3fv("m_normal", (GLfloat*) &cp->multicam_normal_matrix, cp->nrCams);
    	drawShaderQuad->setUniform1i("nrSegColTex", totalNumPics);
    	drawShaderQuad->setUniform3fv("lightPos", &lightPos[0]);
    	drawShaderQuad->setUniform1f("shininess", shininess);
    	drawShaderQuad->setUniform4fv("specular", &specular[0]);
    	drawShaderQuad->setUniform4fv("diffuse", &diffuse[0]);
    	drawShaderQuad->setUniform1f("scaleX", (1.0 / scrAspect) * 2.0);
    	drawShaderQuad->setUniform1f("scaleY", (1.0) * 2.0);
    	drawShaderQuad->setUniform3fv("eyePosW", &camPos[0]);
    	drawShaderQuad->setUniform1f("aspect", 720.f / 1280.f);
    	//drawShaderQuad->setUniform1f("size", 0.45f);

    	// Vacuum 1.0, Air 1.0003, Water 1.3333
        // Glass 1.5, Plastic 1.5, Diamond 2.417
    	drawShaderQuad->setUniform1f("etaRatio", 1.5);

    	glActiveTexture(GL_TEXTURE0);
    	drawShaderQuad->setUniform1i("colTex", 0);
    	glBindTexture(GL_TEXTURE_2D, allPicsTex->getId());

    	glActiveTexture(GL_TEXTURE0 + 1);
    	drawShaderQuad->setUniform1i("normalTex", 1);
    	glBindTexture(GL_TEXTURE_2D, bumpTex->getId());

    	glActiveTexture(GL_TEXTURE0 + 2);
    	drawShaderQuad->setUniform1i("cubeMap", 2);
    	cubeTex->bind(2);
        
        
    	ps->draw(GL_POINTS);
	}

	glActiveTexture(GL_TEXTURE0);

    // reconnect shader from previous step
    _shader->begin();
}

//----------------------------------------------------

void SNTunnelCokeIceCubes::update(double time, double dt)
{

    // osc par
    fluidSim->velocityDissipation = 0.999f;
//                fluidSim->velocityDissipation = std::pow(osc->blurFboAlpha, 0.25);
   fluidSim->addVelocity(velTexID, 5.f);
    fluidSim->update();


    // update particle system
    //ps->update(time, false, fluidSim->getVelocityTex(), GLSLParticleSystem2::POINTS);

    // osc sender

}

//----------------------------------------------------

void SNTunnelCokeIceCubes::initQuadShader(unsigned int nrCams)
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout(location=0) in vec4 position;
			layout(location=5) in vec4 velocity;
			layout(location=6) in vec4 aux0;
			layout(location=7) in vec4 aux1;

			out VS_GS_VERTEX {
				vec4 position;
				vec4 aux0;
				vec4 aux1;
			} vertex_out;

			void main()
			{
				vertex_out.position = position;
				vertex_out.aux0 = aux0;
				vertex_out.aux1 = aux1;
                gl_Position = position;
			});

	vert = "// GLSLParticleSystemFbo  Draw Quad Shader vertex shader\n" + shdr_Header
			+ vert;


	std::string geom = STRINGIFY(
			uniform int nrSegColTex;
			uniform float etaRatio;
			uniform float aspect;
			//uniform float size;
			uniform vec3 eyePosW;

			uniform mat4 m_pvm;

			mat4 trans_matrix;
			mat4 scale_matrix;
                                 
			vec4 column0;
			vec4 column1;
			vec4 column2;
			vec4 column3;

            in VS_GS_VERTEX {
				vec4 position;
				vec4 aux0;
				vec4 aux1;
			} vertex_in[];

			out GS_FS_VERTEX
			{
			    vec4 pos;
			    vec2 tex_coord;
			    vec3 R;
			    vec3 T;
			    vec3 I;
			} vertex_out;

			vec4 center;
			vec4 triangPos;
			float orgZ;
			float size;

			void main()
			{
			    float sqrtNrSegColTex = sqrt( float(nrSegColTex) );
			    float nrSegColSideLen = 1.0 / sqrt( float(nrSegColTex) );

	            if (vertex_in[0].aux0.x > 0.001)
	            {
					center = vertex_in[0].position;
					orgZ = center.z;

					column0 = vec4(1.0, 0.0, 0.0, 0.0);
					column1 = vec4(0.0, 1.0, 0.0, 0.0);
					column2 = vec4(0.0, 0.0, 1.0, 0.0);
					column3 = vec4(center.x, center.y, center.z, 1.0);
					trans_matrix = mat4(column0, column1, column2, column3);

					vec2 texCoordOff = vec2(float(int(vertex_in[0].aux0.w) % int(sqrtNrSegColTex)) / sqrtNrSegColTex,
											float(int(vertex_in[0].aux0.w / sqrtNrSegColTex)) / sqrtNrSegColTex );

					for (int i=0;i<4;i++)
					{
					//	gl_ViewportIndex = gl_InvocationID;
                        
                        // Compute the incident and reflected vectors
                        vertex_out.I = center.xyz - eyePosW;
                        
						// Ermittle die Position der Koordinaten aux_par.r = size, gl_Position ist der Mittelpunkt
                        size = vertex_in[0].aux0.x;
                        center = vec4((i == 1 || i == 3) ? size * aspect : -size * aspect,
                                      (i == 0 || i == 1) ? size : -size,
                                      0.0, 1.0);

    					vertex_out.pos = vec4(center.xy, orgZ, 1.0);

                        //triangPos = m_pvm[gl_InvocationID] * trans_matrix * center;
						gl_Position = m_pvm * trans_matrix * center;

						vertex_out.tex_coord = vec2((i == 1 || i == 3) ? texCoordOff.x + nrSegColSideLen : texCoordOff.x,
													(i == 0 || i == 1) ? texCoordOff.y + nrSegColSideLen : texCoordOff.y);

						EmitVertex();
					}
					EndPrimitive();
				}
			});

    	std::string gHeader = "#version 410\n";
    	gHeader += "layout(points) in;\n";
    	gHeader += "layout(triangle_strip, max_vertices=4) out;\n";
    	//gHeader += "uniform mat4 m_pvm[" +std::to_string(nrCams)+ "];\n";
    	//gHeader += "uniform mat3 m_normal[" +std::to_string(nrCams)+ "];\n";
    	geom = gHeader+geom;


		std::string frag = STRINGIFY(
				layout (location = 0) out vec4 color;

				in GS_FS_VERTEX
				{
				    vec4 pos;
				    vec2 tex_coord;
				    vec3 R;
				    vec3 T;
				    vec3 I;
				} vertex_in;

				uniform sampler2D colTex;
				uniform sampler2D normalTex;
				uniform samplerCube cubeMap;

				uniform vec3 lightPos;
				uniform float shininess;
				uniform vec4 specular;
				uniform vec4 diffuse;
				uniform vec4 sceneColor;
				uniform float etaRatio;

				uniform mat4 tc_rot;

				vec4 procCol;
				vec4 lumi;

				void main()
				{
				    vec4 tex = texture(colTex, vertex_in.tex_coord);

				    vec3 eyeNormal = texture(normalTex, vertex_in.tex_coord).xyz;
				    vec3 L = normalize(lightPos - vertex_in.pos.xyz);
				    vec3 E = normalize(-vertex_in.pos.xyz); // we are in Eye Coordinates, so EyePos is (0,0,0)
				    vec3 R = normalize(-reflect(L, eyeNormal));

				    //vertex_out.R = reflect(vertex_in.I, vertex_out.N);
				    vec3 T = refract(vertex_in.I, eyeNormal, etaRatio);

					vec4 reflectedColor = texture(cubeMap, (tc_rot * vec4(R, 0.0)).xyz);
					vec4 refractedColor = texture(cubeMap, (tc_rot * vec4(T, 0.0)).xyz);

				    //calculate Diffuse Term:
				    vec4 Idiff = diffuse * max(dot(eyeNormal, L), 0.0);
				    Idiff = clamp(Idiff, 0.0, 1.0);

				    // calculate Specular Term:
				    vec4 Ispec = specular * pow(max(dot(R, E), 0.0), shininess);
				    Ispec = clamp(Ispec, 0.0, 1.0);
                    
				    lumi = vec4( pow(Idiff.r, 2.0) ) + Ispec;

				    //procCol = lumi * 0.1 +
				    procCol = (reflectedColor * 0.2 + refractedColor) * tex * 2.0;
				    procCol.r *= 0.8;
				    procCol.g *= 1.2;
				    procCol.a = (tex.r + tex.g + tex.b) * 0.7 * tex.a;

				    if ( tex.a > 0.05 ) {
				    	color = procCol;
				    } else {
				    	discard;
				    }
				    
				    //color = vec4(reflectedColor.rgb, min(sqrt(tex.a * 2.0), 1.0));
				});

		frag = "// GLSLParticleSystemFBO Draw Quad Shader,\n"
				+ shdr_Header + frag;

		drawShaderQuad = shCol->addCheckShaderText("cokeIceCubeDrawQuad", vert.c_str(),
				geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTunnelCokeIceCubes::setEmitTex(GLuint _emitTexID)
{
	emitTexID = _emitTexID;
}

//----------------------------------------------------

void SNTunnelCokeIceCubes::setVelTex(GLuint _velTexID)
{
	velTexID = _velTexID;
}

//----------------------------------------------------

SNTunnelCokeIceCubes::~SNTunnelCokeIceCubes()
{
	delete quad;
    delete ps;
}

}
