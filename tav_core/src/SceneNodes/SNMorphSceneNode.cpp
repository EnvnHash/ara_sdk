//
// SNMorphSceneNode.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNMorphSceneNode.h"

#define STRINGIFY(A) #A

namespace tav
{
//, TFO** _tfos, scBlendData* _scbData, float* _modelM, float* _viewM, float* _projM) :
SNMorphSceneNode::SNMorphSceneNode(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), reqSnapshot(false)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	osc = static_cast<OSCData*>(scd->osc);
	/*
	 geoRecorders = _tfos;
	 modelM = _modelM;
	 viewM = _viewM;
	 projM = _projM;
	 scbData = _scbData;

	 texNrs = new int[MAX_NUM_SIM_TEXS];
	 for (int i=0;i<MAX_NUM_SIM_TEXS;i++) texNrs[i] = i+4;
	 
	 // setupMorphShdr();
	 
	 quad = new Quad();
	 texQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
	 glm::vec3(0.f, 0.f, 1.f),
	 0.f, 0.f, 0.f, 1.f);
	 texQuad->rotate(M_PI, 0.f, 0.f, 1.f);
	 texQuad->rotate(M_PI, 0.f, 1.f, 0.f);
	 
	 snapWidth = 8198;
	 snapHeight = 8198;
	 snapShotFbo = new FBO(shCol, snapWidth, snapHeight);
	 
	 //- Setup Test Vao ---
	 
	 resVAO = new GLuint[2];
	 // make a test vao for drawing the xfb results
	 glGenVertexArrays(2, &resVAO[0]);
	 
	 for (auto i=0;i<2;i++)
	 {
	 glBindVertexArray(resVAO[i]);
	 
	 glBindBuffer(GL_ARRAY_BUFFER, geoRecorders[i]->getTFOBuf(POSITION));
	 glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	 glEnableVertexAttribArray(0);
	 
	 glBindBuffer(GL_ARRAY_BUFFER, geoRecorders[i]->getTFOBuf(NORMAL));
	 glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	 glEnableVertexAttribArray(1);
	 
	 glBindBuffer(GL_ARRAY_BUFFER, geoRecorders[i]->getTFOBuf(TEXCOORD));
	 glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);
	 glEnableVertexAttribArray(2);
	 
	 glBindBuffer(GL_ARRAY_BUFFER, geoRecorders[i]->getTFOBuf(COLOR));
	 glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);
	 glEnableVertexAttribArray(3);
	 
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 
	 glBindVertexArray(0);
	 }
	 
	 auxCol = new float*[MAX_NUM_COL_SCENE];
	 for (auto i=0;i<MAX_NUM_COL_SCENE;i++)
	 {
	 auxCol[i] = new float[4];
	 for (auto j=0;j<3;j++) auxCol[i][j] = 0.f;
	 auxCol[i][3] = 1.f;
	 }
	 */
}


//---------------------------------------------------------------

void SNMorphSceneNode::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	/*
	 if (snapShotMode)
	 {
	 snapShotFbo->bind();
	 snapShotFbo->clearAlpha(osc->feedback);
	 }
	 
	 _shader->begin();
	 
	 float blend = static_cast<float>(geoRecorders[1]->totalPrimsWritten)
	 / static_cast<float>(geoRecorders[0]->totalPrimsWritten);

	 // send uniforms to shader
	 _shader->setUniform1f("relBlend", scbData->relBlend);
	 _shader->setUniform1f("mapScaleFact", blend);

	 // assign the samplers
	 _shader->setUniform1i("pos_tbo", 0);
	 _shader->setUniform1i("nor_tbo", 1);
	 _shader->setUniform1i("tex_tbo", 2);
	 _shader->setUniform1i("col_tbo", 3);
	 _shader->setUniform1iv("texs", texNrs, MAX_NUM_SIM_TEXS);

	 // bind the tbos
	 for (int i=0;i<4;i++)
	 {
	 glActiveTexture(GL_TEXTURE0+i);
	 glBindTexture(GL_TEXTURE_BUFFER, geoRecorders[1]->getTbo((coordType)i));
	 }

	 // bind textures
	 // alle aufgenommenen textures werden gleichzeitig an den Shader gebunden
	 // amd 6490m, max 4 gleichzeitig pro szene, weil 4 schon für die texture_buffers weg sind
	 bool useText = false;
	 short ind = 0;
	 std::vector< auxTexPar >::iterator it;
	 for (it = geoRecorders[0]->textures.begin(); it != geoRecorders[0]->textures.end(); it++)
	 {
	 glActiveTexture(GL_TEXTURE0+((*it).unitNr+4));
	 if(strcmp("texs", (*it).name.c_str()) != 0)
	 {
	 //printf(" morph scene node bind tex %s unit %d tex %d\n", (*it).name.c_str(), (*it).unitNr+4, (*it).texNr);
	 _shader->setUniform1i((*it).name, (*it).unitNr+4);
	 }

	 glBindTexture((*it).target, (*it).texNr);
	 useText = true;
	 
	 ind++;
	 }

	 if(_shader->needsTime)
	 {
	 _shader->setUniform1f("time", time);
	 }
	 
	 //         glDisable(GL_CULL_FACE);
	 
	 //        geoRecorders[0]->recallCullFaceState();
	 geoRecorders[0]->recallDepthTestState();
	 geoRecorders[0]->recallBlendMode();

	 //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	 
	 glBindVertexArray(resVAO[0]);
	 
	 glDrawArrays(GL_TRIANGLES, 0, geoRecorders[0]->totalPrimsWritten);
	 glBindVertexArray(0);
	 
	 _shader->end();

	 if (snapShotMode)
	 {
	 snapShotFbo->unbind();

	 glDisable(GL_DEPTH_TEST);
	 glDisable(GL_CULL_FACE);

	 shCol->getStdTex()->begin();
	 shCol->getStdTex()->setUniform1i("tex", 0);
	 shCol->getStdTex()->setIdentMatrix4fv("m_pvm");
	 
	 glActiveTexture(GL_TEXTURE0);
	 glBindTexture(GL_TEXTURE_2D, snapShotFbo->getColorImg());
	 texQuad->draw();

	 if(reqSnapshot)
	 {
	 // save snapshot to disk
	 cv::Mat bigpic (snapHeight, snapWidth, CV_8UC4);
	 glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, bigpic.data);
	 
	 char fileName [100];
	 sprintf(
	 fileName,
	 "/home/sven/tav_data/screenshots/out%d%d%f.jpg",
	 static_cast<int>( ( static_cast<double>(savedNr) / 10.0 ) ),
	 savedNr % 10,
	 time
	 );
	 
	 cv::imwrite(fileName, bigpic);
	 
	 printf("image written: %s\n", fileName);
	 reqSnapshot = false;
	 }
	 }
	 
	 //glEnable(GL_DEPTH_TEST);
	 glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	 */
}

//---------------------------------------------------------------

void SNMorphSceneNode::update(double time, double dt)
{
}

//---------------------------------------------------------------

void SNMorphSceneNode::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_S:
			reqSnapshot = true;
			break;
		}
	}
}

//---------------------------------------------------------------

void SNMorphSceneNode::setSnapShotMode(bool _val)
{
	snapShotMode = _val;
}

//---------------------------------------------------------------

void SNMorphSceneNode::setupMorphShdr()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec3 normal; layout( location = 2 ) in vec4 texCoord; layout( location = 3 ) in vec4 color;

					uniform samplerBuffer pos_tbo; uniform samplerBuffer nor_tbo; uniform samplerBuffer tex_tbo; uniform samplerBuffer col_tbo;

					uniform int triangle_count;

					uniform mat4 model_matrix; uniform mat4 view_matrix; uniform mat4 projection_matrix;

					out vec4 col;

					void main(){ col = color; vec4 pos = texelFetch(pos_tbo, gl_VertexID); gl_Position = projection_matrix * view_matrix * model_matrix * position; });

	vert = "// standard scene blending morphing vertex shader, vert\n"
			+ shdr_Header + vert;

	std::string frag = STRINGIFY(
			in vec4 col; layout (location = 0) out vec4 color;

			void main(){ color = vec4(1.0); });

	frag = "// standard scene blending morphing fragment shader, frag\n"
			+ shdr_Header + frag;

	shCol->addCheckShaderText("morphScene", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

SNMorphSceneNode::~SNMorphSceneNode()
{
	delete morphShader;
	delete quad;
	delete texQuad;
	delete resVAO;
	delete[] resVAO;
	delete snapShotFbo;
}

}
