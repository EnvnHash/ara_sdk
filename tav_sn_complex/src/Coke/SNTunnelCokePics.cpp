//
// SNTunnelCokePics.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "SNTunnelCokePics.h"

using namespace boost::filesystem;

namespace tav
{
SNTunnelCokePics::SNTunnelCokePics(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), flWidth(200), flHeight(200), maxNrPart(4096),
		intrv(1.0), picWidth(1024), picHeight(1024), totalNumPics(9),
		bigTexName("samsBig.png")
{
	shCol = static_cast<ShaderCollector*>(shCol);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	ps = new GLSLParticleSystemFbo(shCol, maxNrPart,
			float(_scd->screenWidth) / float(_scd->screenHeight));
	ps->setGravity(glm::vec3(0.f));
	ps->setFriction(0.f);
	ps->setAging(true);
	ps->setLifeTime(60.0);
    
    data.posRand = 2.f;
    data.emitVel = glm::normalize(glm::vec3(0.f, 0.f, 1.f));
    data.speed = 0.04f;
    data.speedRand = 0.4f;
    data.size = 0.2f;
    data.sizeRand = 0.f;
    data.maxNrTex = totalNumPics;
    data.texRand = 0.f;
    ps->setEmitData(&data);


	litTex = new TextureManager();
	litTex->loadTexture2D((*scd->dataPath) + "textures/litspheres/white_sphere.jpeg");

	bumpTex = new TextureManager();
	bumpTex->loadTexture2D((*scd->dataPath) + "textures/sphere_norm.png");

	bubbleTex = new TextureManager();
	bubbleTex->loadTexture2D((*scd->dataPath) + "textures/bubble.png");

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube(
			((*scd->dataPath) + "textures/coke_skybox.png").c_str());

    allPicsTex = new TextureManager();

        // checke ob die grosse bilder textur existier
    // check if calibration file exists, if this is the case load it
    bigTexName = *scd->dataPath+"/textures/"+bigTexName;
    std::string picPath = *scd->dataPath+"/textures/samsung_ambient";


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
			printf("SNTunnelCokePics: ES FEHLEN BILDER!!!!, vorhanden: %d \n", picCount);
		} else
		{
			// mache ein grosse Mat fuer alle Bilder
			cv::Mat allPics = cv::Mat::zeros(picHeight * int(std::sqrt(totalNumPics)),
					picWidth * int(std::sqrt(totalNumPics)), CV_8UC4);

			for(short y=0; y<short(std::sqrt(totalNumPics)); y++)
			{
				for(short x=0; x<short(std::sqrt(totalNumPics)); x++)
				{
					short ind = y * short(std::sqrt(totalNumPics)) +x;
					printf("copying pic ind %d\n", ind);

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
	}


	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
}

//----------------------------------------------------

void SNTunnelCokePics::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//sendStdShaderInit(_shader);

	if (!inited)
	{
		initQuadShader(cp->nrCams);
		inited = true;
	}

    glFrontFace(GL_CW);                // counter clockwise definition means front, as default
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);	// wichtig!!!

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::vec4 mCol = glm::vec4(0.7f, 0.7f, 0.7f, 1.0);
	glm::vec3 lightPos = glm::vec3(-1.0, 0.2, 1.0);
	glm::vec4 ambient = glm::vec4(0.5, 0.5, 0.5, 1.0);
	glm::vec4 sceneColor = glm::vec4(0.9, 0.9, 0.9, 0.7);
	glm::vec4 diffuse = glm::vec4(1.0, 1.0, 1.0, 1.0);
	glm::vec4 specular = glm::vec4(0.2, 0.2, 0.2f, 1.0);

	glm::vec3 camPos = glm::vec3(0.f, 0.f, 0.f);

	float shininess = 10.0;
	float scrAspect = 1.f;


	drawShaderQuad->begin();
	drawShaderQuad->setUniformMatrix4fv("m_pvm", (GLfloat*) &cp->multicam_mvp_mat4[0][0][0], cp->nrCams);

	//drawShaderQuad->setUniformMatrix3fv("m_normal", cp->multicam_normal_matrix );
	drawShaderQuad->setUniformMatrix3fv("m_normal", (GLfloat*) &cp->multicam_normal_matrix, cp->nrCams);

    drawShaderQuad->setUniform1i("pixPerGrid", ps->basePTex.cellSize.x * ps->basePTex.cellSize.y ); // ?
//	drawShaderQuad->setUniform1i("pixPerGrid", ps->maxNrCellSqrtQuadAmp); // ?
	drawShaderQuad->setUniform1f("invNrPartSqrt",
			1.f / static_cast<float>(ps->nrPartSqrt));

    drawShaderQuad->setUniform1i("nrSegColTex", totalNumPics);
	drawShaderQuad->setUniform3fv("lightPos", &lightPos[0]);
	drawShaderQuad->setUniform1f("shininess", shininess);
	drawShaderQuad->setUniform4fv("specular", &specular[0]);
	drawShaderQuad->setUniform4fv("diffuse", &diffuse[0]);
	drawShaderQuad->setUniform4fv("ambient", &sceneColor[0]);
	drawShaderQuad->setUniform1f("scaleX", (1.0 / scrAspect) * 2.0);
	drawShaderQuad->setUniform1f("scaleY", (1.0) * 2.0);
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
	glBindTexture(GL_TEXTURE_2D, allPicsTex->getId());


	glActiveTexture(GL_TEXTURE0 + 5);
	drawShaderQuad->setUniform1i("normalTex", 5);
	glBindTexture(GL_TEXTURE_2D, bumpTex->getId());

	glActiveTexture(GL_TEXTURE0 + 6);
	drawShaderQuad->setUniform1i("litsphereTexture", 6);
	glBindTexture(GL_TEXTURE_2D, litTex->getId());

	glActiveTexture(GL_TEXTURE0 + 7);
	drawShaderQuad->setUniform1i("cubeMap", 7);
	cubeTex->bind(7);

//	ps->quadDrawVao->draw(GL_POINTS);
    ps->basePTex.trigVaoQuads->draw(POINTS);


	glActiveTexture(GL_TEXTURE0);


//    glEnable(GL_DEPTH_TEST);
    
 //   glDisable(GL_DEPTH_TEST);
//    glEnable(GL_CULL_FACE);	// wichtig!!!
      glFrontFace(GL_CCW);
}

//----------------------------------------------------

void SNTunnelCokePics::update(double time, double dt)
{
	if (inited)
	{
		if (time - lastTime > getRandF(1.f, 3.f))
		{
			data.emitOrg = glm::vec3(getRandF(-2.f, 2.f), getRandF(-2.f, 2.f), getRandF(-5.f, -7.f));
			ps->emit(5);

			lastTime = time;
		}

		ps->update(time);
	}
}

//----------------------------------------------------

void SNTunnelCokePics::initQuadShader(unsigned int nrCams)
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

			uniform int pixPerGrid;
			uniform int nrSegColTex;
			uniform float invNrPartSqrt;
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
			   // float zVal;
			} vertex_out;

			vec4 pPos;
		    vec4 pVel;
		    vec4 pCol;
		    vec4 pAux0;
		    ivec2 texCoord;
			vec4 center;
			vec4 triangPos;
			float size = 0.05;
			float zOffs;

			void main()
			{
			    ivec2 baseTexCoord = ivec2(gl_in[0].gl_Position.xy);
			    float sqrtNrSegColTex = sqrt( float(nrSegColTex) );
			    float nrSegColSideLen = 1.0 / sqrt( float(nrSegColTex) );


                column0 = vec4(1.0, 0.0, 0.0, 0.0);
                column1 = vec4(0.0, 0.0, 1.0, 0.0);
                column2 = vec4(0.0, -1.0, 0.0, 0.0);
                column3 = vec4(0.0, 0.0, 0.0, 1.0);
                rot_matrix = mat4(column0, column1, column2, column3);

                triangPos = vec4(0.0, 0.0, 0.0, 1.0);

			    // read the textures
			    for (int y=0;y<pixPerGrid;y++)
			    {
			        for (int x=0;x<pixPerGrid;x++)
			        {
			            texCoord = baseTexCoord + ivec2(x, y);
			            pPos = texelFetch(pos_tex, texCoord, 0);
			            zOffs = pPos.z;
		                pAux0 = texelFetch(aux0_tex, texCoord, 0);

			            // switching doesn´t work, since we are double buffering...
			            if (pAux0.x > 0.001)
			            {
			                pCol = texelFetch(col_tex, texCoord, 0);
			                pAux0 = texelFetch(aux0_tex, texCoord, 0);
			                size = pAux0.y;

			                vertex_out.fsColor = vec4( max(min(abs(center.z), 4.0) / 2.0, 1.0) );
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
			    	        	center = vec4(  ((i == 1 || i == 3) ? pAux0.y : -pAux0.y) * 0.7,
			    	        					(i == 0 || i == 1) ? pAux0.y : -pAux0.y,
			    	        					0.0, 1.0);

//			                    if (gl_InvocationID != 2)
			                    	triangPos = m_pvm[gl_InvocationID] * trans_matrix * center;
//			                    else
//			                    	triangPos = m_pvm[gl_InvocationID] * trans_matrix * rot_matrix * center;

			                    gl_Position = triangPos;

			                    if (zOffs < 0.0 || gl_InvocationID == 2)
			                    {
			                    	vertex_out.tex_coord = vec2((i == 1 || i == 3) ? texCoordOff.x + nrSegColSideLen : texCoordOff.x,
			                    								(i == 0 || i == 1) ? texCoordOff.y + nrSegColSideLen : texCoordOff.y);
			                    } else {
			                    	vertex_out.tex_coord = vec2((i == 1 || i == 3) ?  texCoordOff.x : texCoordOff.x + nrSegColSideLen,
			                    								(i == 0 || i == 1) ? texCoordOff.y + nrSegColSideLen : texCoordOff.y);

			                    }


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
    	gHeader += "layout(triangle_strip, max_vertices=24) out;\n";
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
				    //vec4 normal = texture(normalTex, vertex_in.tex_coord);
				    vec4 normal = vec4(0.0, 0.0, 1.0, 0.0);

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
				    vec4 Ispec = specular * pow(max(dot(R, E), 0.0),0.3 * shininess);
				    Ispec = clamp(Ispec, 0.0, 1.0);

				    vec4 shading = texture(litsphereTexture, vec2(normal.xyz * vec3(0.495) + vec3(0.5)));

				    if ( tex.a > 0.3 ) {
				    	color = (((Iamb + Idiff + Ispec) * shading + 0.5 * Ispec) * tex
				    			+ vec4(refractedColor.rgb * 0.3, tex.a)) * tex.a * 0.7 * min((5.0 - abs(vertex_in.pos.z)) / 4.0, 1.0);
                        color *= 1.5;
				    } else {
				    	discard;
				    }


				    //color = vec4(reflectedColor.rgb, min(sqrt(tex.a * 2.0), 1.0));
				});

		frag = "// GLSLParticleSystemFBO Draw Quad Shader, pos.w = lifetime\n"
				+ shdr_Header + frag;

		drawShaderQuad = shCol->addCheckShaderText("tCcokePicsDrawQuad", vert.c_str(),
				geom.c_str(), frag.c_str());
}

//----------------------------------------------------

SNTunnelCokePics::~SNTunnelCokePics()
{
	delete quad;
    delete ps;
}

}
