//
//  SNTrustLogo.cpp
//  Tav_App
//
//  Created by Sven Hahne on 15/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//  parts from assimp simpleopenglexample part from assimp of implementation
//

#include "SNTrustLogo.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
    SNTrustLogo::SNTrustLogo(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs)
    {
    	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
        vt = static_cast<VideoTextureCv*>(vts[0]);

        addPar("reflAmt", &reflAmt);
        addPar("brightScale", &brightScale);

        addPar("exp", &exp);
        addPar("dens", &dens);
        addPar("decay", &decay);
        addPar("weight", &weight);
        addPar("lightX", &lightX);
        addPar("lightY", &lightY);
        addPar("morph", &morph);
        addPar("rot", &rot);
        addPar("alpha", &alpha);
        addPar("ssaoAlpha", &ssaoAlpha);
        addPar("grAlpha", &grAlpha);

      //  aImport = new AssimpImport(_scd->screenWidth, _scd->screenHeight, false); // as gl_triangles drawarrays
     //   aImport->loadModel((*scd->dataPath)+"models/trust_logo_extrude2.obj");
    	logoNode = this->addChild();
    	logoNode->setName(name);
    	logoNode->setActive(true);

    	aImport = new AssimpImport(scd, true);
    	aImport->load(((*scd->dataPath)+"models/trust_logo_extrude2.obj").c_str(), logoNode, [this](){
    	});


        // make morph quad
        unsigned int nrVert = 0;
        for(int i=0;i<int(aImport->getMeshCount());i++)
        	nrVert += aImport->getMeshHelper(i)->vao->getNrVertices();

        // write morph quad data
        // each quad pair has 6 vertices = 2 triangles
        unsigned int nrQuadPairs = nrVert / 6;
        unsigned int nrPairsX = 1;
        unsigned int nrPairsY = nrQuadPairs;

        while (nrPairsX < nrPairsY)
        {
        	nrPairsX += 1;
        	nrPairsY = nrQuadPairs / nrPairsX;
        }
        nrPairsY = static_cast<unsigned int>( std::round( static_cast<float>(nrQuadPairs) / static_cast<float>(nrPairsX) ) );

        nrVert = nrPairsX * nrPairsY * 6;
        float stepX = 2.f / float(nrPairsX -1);
        float stepY = 2.f / float(nrPairsY -1);

        GLfloat vertices[nrVert * 4];	//each vertice has 4 coordinates
        GLfloat* vert = &vertices[0];

        for (int y=0;y<nrPairsX;y++)
        {
        	for (int x=0;x<nrPairsX;x++)
        	{
        		glm::vec2 basePoint = glm::vec2(float(x) * stepX - 1.f, float(y) * stepY - 1.f);
        	//	std::cout << glm::to_string(basePoint) << std::endl;

        		// li untere ecke ind 0
       			*vert = basePoint.x; vert++;
       			*vert = basePoint.y; vert++;
       			*vert = 0.f; vert++;
       			*vert = 1.f; vert++;

       			// re untere ecke ind 1
       			*vert = basePoint.x + stepX; vert++;
       			*vert = basePoint.y; vert++;
       			*vert = 0.f; vert++;
       			*vert = 1.f; vert++;

       			// li obere ecke ind 2
       			*vert = basePoint.x; vert++;
       			*vert = basePoint.y + stepY; vert++;
       			*vert = 0.f; vert++;
       			*vert = 1.f; vert++;

       			// li obere ecke ind 3
       			*vert = basePoint.x; vert++;
       			*vert = basePoint.y + stepY; vert++;
       			*vert = 0.f; vert++;
       			*vert = 1.f; vert++;

       			// li obere ecke ind 4
       			*vert = basePoint.x + stepX; vert++;
       			*vert = basePoint.y + stepY; vert++;
       			*vert = 0.f; vert++;
       			*vert = 1.f; vert++;

       			// li obere ecke ind 5
       			*vert = basePoint.x + stepX; vert++;
       			*vert = basePoint.y; vert++;
       			*vert = 0.f; vert++;
       			*vert = 1.f; vert++;
        	}
        }

        mQuad = new TextureBuffer(nrVert, 4, &vertices[0]);

        litsphereTex = new TextureManager();
        litsphereTex->loadTexture2D((*scd->dataPath)+"textures/litspheres/Unknown-30.jpeg");
        //litsphereTex->loadTexture2D((*scd->dataPath)+"textures/litspheres/ikin_logo_lit.jpeg");

        cubeTex = new TextureManager();
        cubeTex->loadTextureCube((*scd->dataPath)+"textures/trust/trust_cube.png");

        godRays = new GodRays(shCol, _scd->screenWidth/2, _scd->screenHeight/2);
        lightCol = glm::vec4(1.f);

        shdr = shCol->getStdDirLight();
        stdColShdr = shCol->getStdCol();
       // stdTexShdr = shCol->getStdTexAlpha();

        initAlphaShdr();
        initLitShader();
        initColShader();
        initCheapReflShader();

    	ssao = new SSAO(_scd, SSAO::ALGORITHM_HBAO_CACHEAWARE, true,  4.f, 10.f);

        quad = new Quad(-1.0f, -1.f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        1.f, 1.f, 1.f, 1.f);
        rawQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        1.f, 1.f, 1.f, 1.f);

        // create layered texture
        reflFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, 2, GL_RGBA8,
        		GL_TEXTURE_2D_ARRAY, true, 1, 1, 1, GL_CLAMP_TO_EDGE, true);

        actScreenCpy = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, 2, GL_RGBA8,
        		GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

        renderFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, 2, GL_RGBA8,
        		GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
    }
    
    //----------------------------------------------------

    SNTrustLogo::~SNTrustLogo()
    {
        delete shdr;
    }

    //----------------------------------------------------
    
    void SNTrustLogo::initAlphaShdr()
    {
    	std::string shdr_Header = "#version 430 core\n#pragma optimize(on)\n";
    	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;\n
    		layout( location = 1 ) in vec4 normal;\n
    		layout( location = 2 ) in vec2 texCoord;\n
    		layout( location = 3 ) in vec4 color;\n
    		uniform mat4 m_pvm;\n
    		out vec2 tex_coord;\n

    		void main(){\n
    			tex_coord = texCoord;\n
    			gl_Position = m_pvm * position;\n
    	});

    	vert = "// SNTrustLogo alpha texture shader, vert\n"+shdr_Header+vert;

        

    	std::string frag = STRINGIFY(uniform sampler2DArray tex;\n
    		uniform float alpha;
    		in vec2 tex_coord;\n
    		layout (location = 0) out vec4 color;\n

    		void main(){\n
    			color = texture(tex, vec3(tex_coord, 0.0));\n
    			color.a *= alpha;\n
    	});

    	frag = "// SNTrustLogo alpha texture shader, frag\n"+shdr_Header+frag;

    	stdTexShdr = shCol->addCheckShaderText("SNTrustLogo_alpha", vert.c_str(), frag.c_str());
    }

    //----------------------------------------------------

    void SNTrustLogo::initLitShader()
    {
        std::string shdr_Header = "#version 430 core\n#pragma optimize(on)\n";
        std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                        layout( location = 1 ) in vec4 normal;
                                        layout( location = 2 ) in vec2 texCoord;
                                        layout( location = 3 ) in vec4 color;

                                        uniform samplerBuffer quadPos;
                                        uniform float morph;

                                        out TO_GS {
                                        	vec2 tex_coord;
                                        	vec2 quadTex_coord;
                                         	vec3 normal;
                                         	vec4 rawPos;
                                        } vertex_out;

                                        void main()
                                        {
                                        	vec4 quadVert = texelFetch(quadPos, gl_VertexID);
                                        	vertex_out.tex_coord = texCoord;
                                        	vertex_out.quadTex_coord = quadVert.xy * 0.5 + 0.5;
                                        	vertex_out.normal = normal.xyz;
                                        	vertex_out.rawPos = position;
                                            gl_Position = mix(position, vec4(quadVert.x, 0.0, quadVert.y, 1.0), morph);
                                        });

        stdVert = "// SNTrustLogo vertex shader\n" +shdr_Header +stdVert;

        

        shdr_Header = "#version 430 core\n#pragma optimize(on)\nlayout(triangles, invocations = 2) in;\nlayout(triangle_strip, max_vertices = 3) out;\n uniform mat3 m_norm[2];\n uniform mat4 m_m[2];\n";

        std::string geom = STRINGIFY(uniform mat4 m_p;
        							uniform mat4 m_v;

                                     in TO_GS {
                                    	vec2 tex_coord;
                                    	vec2 quadTex_coord;
                                     	vec3 normal;
                                     	vec4 rawPos;
                                     } vertex_in[];

                                     out TO_FS {
                                    	vec2 tex_coord;
                                    	vec2 quadTex_coord;
                                     	vec3 eye_pos;
                                     	vec3 normal;
                                     } vertex_out;

                                     void main()\n
                                     {\n
                                         for (int i=0; i<gl_in.length(); i++)\n
                                         {\n
                                        	 //gl_Layer = gl_InvocationID;
                                        	 vertex_out.tex_coord = vertex_in[i].tex_coord;\n
                                        	 vertex_out.quadTex_coord = vertex_in[i].quadTex_coord;\n
                                             vertex_out.normal = normalize(m_norm[gl_InvocationID] * vertex_in[i].normal);\n
                                             vertex_out.eye_pos = normalize( vec3( m_v * m_m[gl_InvocationID] * vertex_in[i].rawPos ) );
                                             gl_Layer = gl_InvocationID;
                                             gl_Position = m_p * m_v * m_m[gl_InvocationID] * gl_in[i].gl_Position;\n
                                             EmitVertex();\n
                                         }\n
                                         EndPrimitive();\n
                                     });

        geom = "// SNTrustLogo geom shader\n" +shdr_Header +geom;

        

        shdr_Header = "#version 430 core\n#pragma optimize(on)\n";

        std::string frag = STRINGIFY(uniform sampler2D normMap;
                                     uniform sampler2D litSphereTex;
                                     uniform sampler2D videoTex;
                                     uniform samplerCube cubeMap;

                                     uniform float reflAmt;
                                     uniform float brightScale;
                                     uniform float morph;
                                     uniform float alpha;

                                     const float Eta = 0.15; // Water

                                     in TO_FS {
                                    	vec2 tex_coord;
                                    	vec2 quadTex_coord;
                                     	vec3 eye_pos;
                                     	vec3 normal;
                                     } vertex_in;

                                     vec4 orgCol;
                                     vec4 litColor;
                                     float outVal;

                                     layout (location = 0) out vec4 color;

                                     void main()
                                     {
                                    	 vec3 n = vertex_in.normal;

                                    	 // litsphere
                                    	 vec3 reflection = reflect( vertex_in.eye_pos, n );
                                    	 vec3 refraction = refract( vertex_in.eye_pos, n, Eta );

                                    	 float m = 2.0 * sqrt(
                                    	        pow( reflection.x, 2.0 ) +
                                    	        pow( reflection.y, 2.0 ) +
                                    	        pow( reflection.z + 1.0, 2.0 )
                                    	 );
                                    	 vec2 vN = reflection.xy / m + 0.5;

                                         litColor = texture(litSphereTex, vN);

                                         vec4 reflectionCol = texture( cubeMap, reflection );
                                         vec4 refractionCol = texture( cubeMap, refraction );

                                         float fresnel = Eta + (1.0 - Eta) * pow(max(0.0, 1.0 - dot(-vertex_in.eye_pos, n)), 5.0);
                                         //vec4 reflCol = mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0));
                                         vec4 reflCol = reflAmt * mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0) * 0.75);
                                         vec4 vidColor = texture(videoTex, vec2(vertex_in.quadTex_coord.x,
                                        		 1.0 - vertex_in.quadTex_coord.y) );

                                         color = mix((litColor + reflCol) * brightScale, vidColor, morph);
                                         color.a = alpha;
                                     });

        frag = "// SNTrustLogo fragment shader\n"+shdr_Header+frag;

        litShdr = shCol->addCheckShaderText("SNTrustLogo", stdVert.c_str(), geom.c_str(), frag.c_str());
    }

    //----------------------------------------------------

    void SNTrustLogo::initColShader()
    {
        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                        layout( location = 1 ) in vec4 normal;
                                        layout( location = 2 ) in vec2 texCoord;
                                        layout( location = 3 ) in vec4 color;

                                        uniform mat4 m_pvm;
                                        uniform samplerBuffer quadPos;
                                        uniform float morph;
                                        uniform vec4 setCol;
                                        out vec4 col;
                                        void main()
                                        {
                                        	col = setCol;
                                        	vec4 quadVert = texelFetch(quadPos, gl_VertexID);
                                            gl_Position = m_pvm * mix(position, vec4(quadVert.x, 0.0, quadVert.y, 1.0), morph);
                                        });

        stdVert = "// SNTrustLogo vertex shader\n" +shdr_Header +stdVert;

        std::string frag = STRINGIFY(uniform float morph;
        							in vec4 col;
                                     layout (location = 0) out vec4 color;
                                     void main() {
                                         color = col;
                                     });

        frag = "// SNTrustLogo fragment shader\n"+shdr_Header+frag;

        colShdr = shCol->addCheckShaderText("SNTrustLogo_col", stdVert.c_str(), frag.c_str());
    }

    //----------------------------------------------------

    void SNTrustLogo::initCheapReflShader()
    {
        std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
        std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                        layout( location = 1 ) in vec4 normal;
                                        layout( location = 2 ) in vec2 texCoord;
                                        layout( location = 3 ) in vec4 color;
                                        uniform mat4 m_m;
                                        uniform mat4 m_pv;
                                        out float zPos;
                                        void main() {
                                        	zPos = 1.0 + (m_m * position).z * 0.1;
                                            gl_Position = m_pv * m_m * position;
                                        });

        stdVert = "// SNTrustLogo vertex shader\n" +shdr_Header +stdVert;

        std::string frag = STRINGIFY(uniform sampler2DArray tex;
        							 uniform float alpha;
        							 uniform vec2 scrSize;
                                     in float zPos;

                                     layout (location = 0) out vec4 color;
                                     void main() {
                                    	 color = vec4( zPos * 0.4 );
                                         color *= texture(tex, vec3(float(gl_FragCoord.x) / scrSize.x,
                                        		 	 	 	 	    float(gl_FragCoord.y) / scrSize.y,
                                        		 	 	 	 	 	1.0));
                                         color.a = zPos * 0.4 * alpha;
                                     });

        frag = "// SNTrustLogo fragment shader\n"+shdr_Header+frag;

        reflShdr = shCol->addCheckShaderText("SNTrustLogo_refl", stdVert.c_str(), frag.c_str());
    }

    //----------------------------------------------------

    void SNTrustLogo::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo)
        {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    	glClearDepth(1.0);

    	// copy act screen content
    	int lastBoundFbo;
    	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
    	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, actScreenCpy->getFbo());
    	glBindFramebuffer(GL_READ_FRAMEBUFFER, lastBoundFbo);
    	glBlitFramebuffer(0, 0, int(cp->actFboSize.x), int(cp->actFboSize.y),
    			0, 0, int(cp->actFboSize.x), int(cp->actFboSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);


        // translate model back to origin
        //glm::mat4 rotMatr = *aImport->getNormCubeAndCenterMatr();
        glm::mat4 rotMatr = *aImport->getNormAndCenterMatr()
        	* glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -0.8f))
            * glm::rotate(glm::mat4(1.f), rot, glm::vec3(0.f, 1.f, 0.f))
        	* glm::rotate(glm::mat4(1.f), float(M_PI) * 0.5f, glm::vec3(1.f, 0.f, 0.f))
         	* _modelMat;

        glm::mat4 m_m[2];
        m_m[0] = rotMatr;
        m_m[1] = glm::scale(rotMatr, glm::vec3(1.f, 1.f, -1.f))
        		* glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -0.3f));

        glm::mat3 normMat[2];
        for (int i=0;i<2;i++)
        	normMat[i] = glm::mat3(cp->view_matrix_mat4 * m_m[i]);

        m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * m_m[0];

        godRays->setExposure( exp );
        godRays->setDensity( dens );
        godRays->setDecay( decay );
        godRays->setWeight( weight );
        godRays->setLightPosScr( lightX, lightY );
        godRays->setAlpha( grAlpha );

        // -

        // calc godrays
        godRays->bind();	// bind ist auch clear()

        colShdr->begin();
        colShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
        colShdr->setUniform1i("quadPos", 0);
        colShdr->setUniform1f("morph", 0.f);
        colShdr->setUniform4f("setCol", 0.5f, 0.5f, 0.5f, alpha);

        glActiveTexture(GL_TEXTURE0);
        mQuad->bindTex(0);

        aImport->draw(GL_TRIANGLES, colShdr);

        godRays->unbind();

        // -
        // render logo two times with different model matrices to a 2-layered Fbo

		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);

		reflFbo->bind();
		reflFbo->clearAlpha(0.f, 1.f);
//		reflFbo->clearAlpha(0.f, alpha);
		reflFbo->clearDepth();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        godRays->draw();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // das Logo
        litShdr->begin();
        litShdr->setUniformMatrix4fv("m_p", cp->projection_matrix);
        litShdr->setUniformMatrix4fv("m_v", cp->view_matrix);
        litShdr->setUniformMatrix4fv("m_m", &m_m[0][0][0], 2);
        litShdr->setUniformMatrix3fv("m_norm", &normMat[0][0][0], 2);
        litShdr->setUniform1i("litSphereTex", 0);
        litShdr->setUniform1i("cubeMap", 1);
        litShdr->setUniform1i("quadPos", 2);
        litShdr->setUniform1i("videoTex", 3);
        litShdr->setUniform1f("reflAmt", reflAmt);
        litShdr->setUniform1f("brightScale", brightScale);
        litShdr->setUniform1f("morph", morph);
        litShdr->setUniform1f("alpha", alpha);

        litsphereTex->bind(0);
        cubeTex->bind(1);
        mQuad->bindTex(2);

        //vt->bindActFrame(3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, actScreenCpy->getColorImg());

        aImport->draw(GL_TRIANGLES, litShdr);

		reflFbo->unbind();

        // -
		// copy result to last fbo

		// copy depth buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, reflFbo->getFbo());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastBoundFbo);
		glBlitFramebuffer(
				0, 0, reflFbo->getWidth(), reflFbo->getHeight(),
				0, 0, int(cp->actFboSize.x), int(cp->actFboSize.y), GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

		// copy color buffer with alpha
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthMask(false);

		stdTexShdr->begin();
		stdTexShdr->setIdentMatrix4fv("m_pvm");
		stdTexShdr->setUniform1i("tex", 0);
		stdTexShdr->setUniform1f("alpha",  alpha);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, reflFbo->getColorImg());

        rawQuad->draw();


        // -
		// eine weisse Flaeche unter dem Logo fuer Schatten
		// render eine reflektion mit rein, gelesen aus dem 2ten Layer des Fbo

		glDepthMask(true);
		glEnable(GL_DEPTH_TEST);

        glm::mat4 pv = cp->projection_matrix_mat4 * cp->view_matrix_mat4;
        glm::mat4 quadMat = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -1.9f, 0.f))
        	* glm::rotate(glm::mat4(1.f), float(M_PI) * -0.5f, glm::vec3(1.f, 0.f, 0.f))
    		* glm::scale(glm::mat4(1.f), glm::vec3(20.f, 20.f, 0.f));

        reflShdr->begin();
        reflShdr->setUniformMatrix4fv("m_pv", &pv[0][0]);
        reflShdr->setUniformMatrix4fv("m_m", &quadMat[0][0]);
        reflShdr->setUniform1i("tex", 0);
        reflShdr->setUniform2f("scrSize", float(cp->actFboSize.x), float(cp->actFboSize.y));
        reflShdr->setUniform1f("alpha", alpha * (1.0 - morph));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, reflFbo->getColorImg());

        quad->draw();

        // -
/*
        ssao->copyFbo(cp);
		ssao->bind();
        ssao->proc(cp);
        ssao->drawAlpha(cp, ssaoAlpha") );
        ssao->unbind();
        */
    }

    //----------------------------------------------------

    void SNTrustLogo::update(double time, double dt)
    {
    	vt->updateDt(time, false);
    	actUplTexId = vt->loadFrameToTexture();
    }

    //----------------------------------------------------

    void SNTrustLogo::onKey(int key, int scancode, int action, int mods)
    {}
}
