//
// SNTestSphereTexture.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestSphereTexture.h"

#define STRINGIFY(A) #A

namespace tav
{
    SNTestSphereTexture::SNTestSphereTexture(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs, "NoLight")
    {
    	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
#ifdef HAVE_OPENCV
    	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
    	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];
#endif
    	//TextureManager** texs = static_cast<TextureManager**>(scd->texObjs);
    	//tex0 = texs[static_cast<GLuint>(_sceneArgs->at("tex0"))];

        quad = new Quad(-0.5f, -0.5f, 1.f, 1.f,
                        glm::vec3(0.f, 0.f, 1.f),
                        1.f, 0.f, 0.f, 1.f);    // color will be replace when rendering with blending on
        
        sphere = new Sphere(2.f, 64);
        //cubeElem = new CubeElem(0.5f, 0.5f, 0.5f);
    }
    
    SNTestSphereTexture::~SNTestSphereTexture()
    {
        delete quad;
        delete sphere;
    }
    
    void SNTestSphereTexture::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
    {
        if (_tfo) {
            _tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        if (!isInited)
        {
            initShdr(cp);
            isInited = true;
        }

        glm::mat4 trans = glm::scale(glm::vec3(1.f, -1.f, 1.f))
        	* glm::rotate(float(time * 0.4f), glm::normalize(glm::vec3(float(std::sin(time) * 0.12f), 1.f, 0.f)));

        glDisable(GL_CULL_FACE);

        texShdr->begin();
        texShdr->setUniformMatrix4fv("trans", &trans[0][0]);
        texShdr->setUniformMatrix4fv("projection_matrix_g", cp->multicam_projection_matrix, cp->nrCams);
        texShdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix, cp->nrCams);
       // texShdr->setUniformMatrix4fv("m_pvm", camSimu->getMVPPtr() );

    	//useTextureUnitInd(0, vt->texIDs[0], texShdr, _tfo);
        texShdr->setUniform1i("tex", 0);

    	glActiveTexture(GL_TEXTURE0);
    	glBindTexture(GL_TEXTURE_2D, actUplTexId);

        //useTextureUnitInd(0, tex0->getId(), _shader, _tfo);
        sphere->draw();
    }
    
    //----------------------------------------------------

   void SNTestSphereTexture::initShdr(camPar* cp)
   {
	   std::string nrCams = std::to_string(cp->nrCams);

	   //------ Position Shader -----------------------------------------

	   std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	   std::string vert = STRINGIFY(layout (location=0) in vec4 position;
									layout (location=1) in vec3 normal;
									layout (location=2) in vec2 texCoord;
									layout (location=3) in vec4 color;
									out vec2 tex_coord;

									void main() {
										tex_coord = texCoord;
										tex_coord.x = tex_coord.x;
										gl_Position = position;
									});
	   vert = "// SNTestFaceLola shader\n" +shdr_Header +vert;


	   shdr_Header = "#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="+nrCams+") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["+nrCams+"];uniform mat4 trans;\nuniform mat4 projection_matrix_g["+nrCams+"];";

	   std::string geom = STRINGIFY(in vec3 Normal[]; // surface normal, interpolated between vertices
									in vec2 tex_coord[];

									out vec2 gs_texCo;
									out float invocID;

									void main()
									{
										gl_ViewportIndex = gl_InvocationID;
										invocID = float(gl_InvocationID);
										for (int i=0; i<gl_in.length(); i++)
										{
											gs_texCo = tex_coord[i];

											gl_Position = projection_matrix_g[gl_InvocationID]
														   * view_matrix_g[gl_InvocationID]
														   * trans
														   * gl_in[i].gl_Position;
											EmitVertex();
										}
										EndPrimitive();
									});

	   geom = "// SNTestFaceLola  geom shader\n" +shdr_Header +geom;



	   shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	   std::string frag = STRINGIFY(layout (location = 0) out vec4 FragColor;
	   	   	   	   	   	   	   	   uniform sampler2D tex;
									in vec2 gs_texCo;
									in float invocID;

									void main()
									{
										FragColor = texture(tex, gs_texCo);
									});
	   frag = "// SNTestFaceLola shader\n"+shdr_Header+frag;

	   texShdr = shCol->addCheckShaderText("SNTestSphereTexture", vert.c_str(), geom.c_str(), frag.c_str());
   }


    void SNTestSphereTexture::update(double time, double dt)
    {
#ifdef HAVE_OPENCV
    	vt->updateDt(time, false);
    	actUplTexId = vt->loadFrameToTexture();
#endif
    }
}
