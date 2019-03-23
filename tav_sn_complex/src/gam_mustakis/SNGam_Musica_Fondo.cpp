//
// SNGam_Musica_Fondo.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Funktioniert nur mit blending = true
//

#include "SNGam_Musica_Fondo.h"

#define STRINGIFY(A) #A

namespace tav
{
SNGam_Musica_Fondo::SNGam_Musica_Fondo(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "LitSphere"), texGridSize(64), nrSegs(60),
		brightAdj(0.f), heightScale(0.034f), distAmp(300.f), alpha(1.f), audioDistMed(13.f),
		lineThick(0.8f), nrLines(371.f), ringFadeOffs(-0.23), ringFade(1.44f), nrBaseCols(4)
{
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	baseCol = new glm::vec4[nrBaseCols];
	for (int i=0; i<nrBaseCols; i++)
		for (int j=0; j<4; j++)
			baseCol[i][j] = scd->colors[static_cast<unsigned int>(sceneArgs->at("col" + std::to_string(i)))][j];

	quadAr = new QuadArray(120, 40, -1.f, -1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f);
	quad = _scd->stdQuad;

	intModelMat = glm::mat4(1.f);
	normalMat = glm::mat3(glm::transpose(glm::inverse(_modelMat)));

	posTex = new PingPongFbo(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	normTex = new FBO(shCol, texGridSize, texGridSize, GL_RGB16F, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	litsphereTex = new TextureManager();
	litsphereTex->loadTexture2D( *scd->dataPath + "/textures/gam_mustakis/gam_litsphere.jpg");
	litsphereTex->setWraping(GL_REPEAT);

	bumpMap = new TextureManager();
	bumpMap->loadTexture2D( *scd->dataPath + "/textures/bump_maps/Unknown-2.jpeg");
	bumpMap->setWraping(GL_REPEAT);

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube( ((*scd->dataPath) + "textures/skyboxsun5deg2.png").c_str());
    //cubeTex->loadTextureCube((*scd->dataPath)+"textures/gam_mustakis/skybox_gam.png");

	addPar("aux1", &aux1);
	addPar("aux2", &aux2);
	addPar("alpha", &alpha);
	addPar("reflAmt", &reflAmt);
	addPar("brightAdj", &brightAdj);
	addPar("heightScale", &heightScale);
	addPar("distAmp", &distAmp);
	addPar("audioDistMed", &audioDistMed);
	addPar("lineThick", &lineThick);
	addPar("nrLines", &nrLines);
	addPar("ringFadeOffs", &ringFadeOffs);
	addPar("ringFade", &ringFade);


	initPreCalcShdr();
	initRenderShdr();

	calibFileName = (*scd->dataPath)+"calib_cam/gam_musica_fondo.yml";
	if (access(calibFileName.c_str(), F_OK) != -1)
		loadCalib();
}

//----------------------------------------------------

void SNGam_Musica_Fondo::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	glm::mat4 modelMat = glm::rotate(_modelMat, aux2 * float(M_PI) * 2.f,
			glm::vec3(0.f, 0.f, 1.f));

	glm::mat4 m_vm = cp->view_matrix_mat4 * modelMat;
	glm::mat4 m_pvm = cp->projection_matrix_mat4 * m_vm;
	glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	renderShdr->begin();

	renderShdr->setUniformMatrix4fv("m_vm", &m_vm[0][0]);
	renderShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
	renderShdr->setUniformMatrix3fv("normalMatrix", &normalMat[0][0]);
	renderShdr->setUniform1f("texGridSize", float(texGridSize));
	renderShdr->setUniform1i("posTex", 0);
	renderShdr->setUniform1i("normTex", 1);
	renderShdr->setUniform1i("litSphereTex", 2);
	renderShdr->setUniform1i("cubeMap", 3);

	renderShdr->setUniform1i("vertPerChan", nrSegs * nrSegs * 6);
	renderShdr->setUniform1f("ftime", float(time));
	renderShdr->setUniform1f("time", float(intTime));
	renderShdr->setUniform4fv("scCol", &baseCol[0][0], nrBaseCols);
	renderShdr->setUniform1f("alpha", alpha);
	renderShdr->setUniform1f("reflAmt", reflAmt);
	renderShdr->setUniform1f("brightAdj", brightAdj);
	renderShdr->setUniform1f("heightScale", heightScale + aux1 * 15.f);
	renderShdr->setUniform1f("lineThick", lineThick);
	renderShdr->setUniform1f("nrLines", nrLines);
	renderShdr->setUniform1f("ringFadeOffs", ringFadeOffs);
	renderShdr->setUniform1f("ringFade", ringFade);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTex->src->getColorImg());

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normTex->getColorImg());

	litsphereTex->bind(2);
	cubeTex->bind(3);

	quadAr->draw();

}

//----------------------------------------------------

void SNGam_Musica_Fondo::update(double time, double dt)
{
	intTime += dt * osc->speed;

	scd->audioTex->update(pa->getPll(), pa->getBlockCount());
	scd->audioTex->mergeTo2D(pa->getBlockCount());

	// - pre calculate positions from audioTexture ----
	// since linear filtering will produce an edge with weighted pixels
	// the fbo needs to be written with one pixel border

	posTex->dst->bind();
	posTex->dst->clear();

	posTexShdr->begin();
	posTexShdr->setUniform1i("audioTex", 0);
	posTexShdr->setUniform1i("lastResult", 1);
	posTexShdr->setUniform1i("nrChannels", scd->nrChannels);
	posTexShdr->setUniform1f("time", intTime);
	posTexShdr->setUniform1f("distAmp", distAmp);
	posTexShdr->setUniform1f("median", audioDistMed);
	posTexShdr->setIdentMatrix4fv("m_pvm");

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

	glActiveTexture (GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, posTex->src->getColorImg());

	quad->draw();

	posTex->dst->unbind();
	posTex->swap();

	// - pre calculate normals from precalculated positions -

	normTex->bind();
	normTex->clear();

	normTexShdr->begin();
	normTexShdr->setUniform1i("posTex", 0);
	normTexShdr->setUniform1f("texGridSize", float(texGridSize));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, posTex->src->getColorImg());

	quad->draw();

	normTex->unbind();

	if (_hasNewOscValues) {
		_hasNewOscValues = false;
		saveCalib();
	}
}

//----------------------------------------------------

void SNGam_Musica_Fondo::initPreCalcShdr()
{
	//- Position Shader ---
	// leave one pixel border

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(
		layout (location=0) in vec4 position;\n
		layout (location=1) in vec3 normal;\n
		layout (location=2) in vec2 texCoord;\n
		layout (location=3) in vec4 color;\n
		uniform mat4 m_pvm;\n
		out vec4 pos;
		out vec2 toTexPos;
		\n
		void main(void) {\n
			pos = position;
			toTexPos = position.xy * 0.5 + 0.5;
			gl_Position = m_pvm * position;
		});
	vert = "// AudioTunnelGPU pos tex vertex shader\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
		layout(location = 0) out vec4 pos_tex;\n
		in vec4 pos;\n
		in vec2 toTexPos;\n
		\n
		uniform sampler2D lastResult;\n
		uniform sampler2D audioTex;\n
		\n
		uniform int nrChannels;\n
		uniform float time;\n
		uniform float distAmp;\n
		uniform float median;\n
		\n
		void main() {\n
			float ampX;\n
			float ampY;\n
			int i;\n
			float chanOffs;\n

			ampX = 0.0; ampY = 0.0;\n
			\n
			for (i=0;i<nrChannels;i++) {\n
				chanOffs = float(i) / float(nrChannels);
				ampX += (texture(audioTex, vec2(toTexPos.x, chanOffs)).r + texture(audioTex, vec2(1.0 - toTexPos.x, chanOffs)).r) * 0.5;
				ampY += (texture(audioTex, vec2(toTexPos.y - time * 0.1, chanOffs)).r + texture(audioTex, vec2((1.0 - toTexPos.y) - time * 0.1, chanOffs)).r) * 0.5;
			}\n
			\n
			ampX /= float(nrChannels);\n
			ampY /= float(nrChannels);\n
			\n
			pos_tex = vec4(pos.x, pos.y,\n
					 (((ampY + ampX) * distAmp) + texture(lastResult, toTexPos).z * median) / (median + 1.0), // -1 to 1,
					1.0);
		});
	frag = "// AudioTunnelGPU pos tex shader\n" + shdr_Header + frag;

	posTexShdr = shCol->addCheckShaderText("gamMusicaFondoPosTex", vert.c_str(), frag.c_str());

	//- Normal Shader ---
	// relevant ist immer die mitte des Pixels, deshalb muessen die TexturKoordinate
	// geringfuegig um ein halbes Pixels auf allen Seiten verkleinert werden

	vert = STRINGIFY(
		layout (location=0) in vec4 position;\n
		layout (location=1) in vec3 normal;\n
		layout (location=2) in vec2 texCoord;\n
		layout (location=3) in vec4 color;\n
		\n
		uniform float texGridSize;\n
		out vec2 toTexPos;\n
		\n
		void main(void) {\n
			toTexPos = (position.xy * 0.5 + 0.5)
					* ((texGridSize - 1.0) / texGridSize)
					+ vec2(0.5 / texGridSize);
			gl_Position = position; \n
		});
	vert = "// AudioTunnelGPU norm tex vertex shader\n" + shdr_Header + vert;

	frag = STRINGIFY(
		layout(location = 0) out vec4 norm_tex;\n
		in vec2 toTexPos;\n
		uniform sampler2D posTex;\n
		uniform float texGridSize;\n
		\n
		vec3 posTop;\n
		vec3 posBottom;\n
		vec3 posCenter;\n
		vec3 posLeft;\n
		vec3 posRight;\n
		\n
		vec3 norms[2];\n
		\n
		float gridOffs;\n
		\n
		void main() {\n
			gridOffs = 1.0 / float(texGridSize);\n
			\n
			// read neighbour positions left, right, top, bottom
			posTop = texture(posTex, vec2(toTexPos.x, toTexPos.y + gridOffs) ).xyz; \n
			posBottom = texture(posTex, vec2(toTexPos.x, toTexPos.y - gridOffs)).xyz; \n
			posCenter = texture(posTex, toTexPos).xyz; \n
			posLeft = texture(posTex, vec2(toTexPos.x - gridOffs, toTexPos.y)).xyz;\n
			posRight = texture(posTex, vec2(toTexPos.x + gridOffs, toTexPos.y)).xyz;\n
			\n
			norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter)));\n
			norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));\n
			\n
			for(int i=0;i<2;i++)\n
				norms[i] = norms[i].z > 0.0 ? norms[i] : norms[i] * -1.0;\n
			\n
			norms[0] = normalize((norms[0] + norms[1]) * 0.5);
			norm_tex = vec4(norms[0], 1.0);\n
		});
	frag = "// AudioTunnelGPU norm tex shader\n" + shdr_Header + frag;

	normTexShdr = shCol->addCheckShaderText("gamMusicaFondoNormTex", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNGam_Musica_Fondo::initRenderShdr()
{
	std::string shdr_Header = shCol->getShaderHeader() + "\n uniform vec4 scCol[" + std::to_string(nrBaseCols) + "];\n";

	std::string stdVert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec4 normal;\n
		layout( location = 2 ) in vec2 texCoord;\n
		layout( location = 3 ) in vec4 color;\n

		uniform sampler2D posTex;\n
		uniform sampler2D normTex;\n

		uniform int useInstancing;\n
		uniform int vertPerChan;\n
		uniform int texNr;\n
		uniform float time;\n
		uniform float texGridSize;\n
		uniform float heightScale;

		mat4 normCylMatrix;\n

		uniform mat4 m_vm;\n
		uniform mat4 m_pvm;\n
		uniform mat3 normalMatrix;\n

		vec2 modTexCoord;\n
		vec4 pos;\n
		vec4 norm;\n
		vec4 column0;\n
		vec4 column1;\n
		vec4 column2;\n
		vec4 column3;\n

		float pi = 3.1415926535897932384626433832795;\n
		float rad;\n
		float angle;\n
		int chanInd;\n

		out TO_FS {\n
			vec4 tex_coord;\n
			vec3 eye_pos;\n
			vec3 normal;\n
			vec4 col;
		} vertex_out;\n

		void main()\n {\n
			chanInd = gl_VertexID / vertPerChan;\n
			modTexCoord = texCoord * ((texGridSize - 1.0) / texGridSize) + vec2(0.5 / texGridSize);\n

			pos = texture(posTex, modTexCoord);\n
			pos = vec4(pos.xy * ((texGridSize + 1.0) / texGridSize) * 1.0005, // noetig, vermutlich rundungsfehler, kreis wird sonst nicht ganz geschlossen
					pos.z, 1.0);\n
			angle = pos.x * pi;\n
			rad = pos.z * heightScale + 1.5;\n// ops.z -1 to 1

			pos = vec4(sin(angle) * rad, cos(angle) * rad, (pos.y - 0.5) * 16.0, 1.0);\n
			norm = texture(normTex, modTexCoord);\n

			column0 = vec4(cos(angle), 0.0, -sin(angle), 0.0);\n
			column1 = vec4(0.0, 1.0, 0.0, 0.0);\n
			column2 = vec4(sin(angle), 0.0, cos(angle), 0.0);\n
			column3 = vec4(0.0, 0.0, 0.0, 1.0);\n
			normCylMatrix = mat4(column0, column1, column2, column3);\n

			vertex_out.normal = normalize(normalMatrix * (normCylMatrix * norm).xyz);\n

			// interpolate between the four base colors by texCoord.x
			float offPhase = mod(texCoord.x - 0.375, 1.0);
			vertex_out.col = scCol[0];
			for (int i=0;i<4;i++)
				vertex_out.col = mix(vertex_out.col, scCol[(i+1)%4], min(max(offPhase - float(i) * 0.25, 0.0) * 4.0, 1.0));\n

			vertex_out.tex_coord = vec4(vec2(texCoord.x, texCoord.y - time * 0.1), float(texNr), 0.0);\n
			vertex_out.eye_pos = normalize( vec3( m_vm * pos ) );\n

			gl_Position = m_pvm * pos;\n
		});

	stdVert = "// SNGam_Musica_Fondo vertex shader\n" + shdr_Header + stdVert;

	std::string frag = STRINGIFY(
		uniform sampler2D litSphereTex;
		uniform samplerCube cubeMap;
		uniform mat3 m_normal;

		in TO_FS {
			vec4 tex_coord;
			vec3 eye_pos;
			vec3 normal;
			vec4 col;
		} vertex_in;

		vec4 litColor;
		const float Eta = 0.15; // Water

		uniform float alpha;
		uniform float reflAmt;
		uniform float brightAdj;
		uniform float time;\n
		uniform float ftime;\n
		uniform float lineThick;\n
		uniform float nrLines;\n
		uniform float ringFadeOffs;\n
		uniform float ringFade;\n

		layout (location = 0) out vec4 color;

		void main() {
			// litsphere
			vec3 reflection = reflect( vertex_in.eye_pos, vertex_in.normal );
			vec3 refraction = refract( vertex_in.eye_pos, vertex_in.normal, Eta );

			float m = 2.0 * sqrt( pow( reflection.x, 2.0 ) + pow( reflection.y, 2.0 ) + pow( reflection.z + 1.0, 2.0 ) );
			vec2 vN = reflection.xy / m + 0.5;

			litColor = texture(litSphereTex, vN);
			float litBright = litColor.r * litColor.g * litColor.b;

			vec4 reflectionCol = texture( cubeMap, reflection );
			vec4 refractionCol = texture( cubeMap, refraction );

			float fresnel = Eta + (1.0 - Eta) * pow(max(0.0, 1.0 - dot(-vertex_in.eye_pos, vertex_in.normal)), 5.0);
			vec4 reflCol = reflAmt * mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0));

			float ring = 1.0 - sin(vertex_in.eye_pos.z * nrLines);
			color = max(0.0, ring + lineThick) < 0.9 ? vertex_in.col : vec4(0.0);
//			color = vertex_in.col * litBright * brightAdj + reflCol;
			color.a = alpha * (1.0 - sqrt(-vertex_in.eye_pos.z * ringFade + ringFadeOffs));
		});

	frag = "// SNGam_Musica_Fondo fragment shader\n" + shCol->getShaderHeader() + frag;

	renderShdr = shCol->addCheckShaderText("SNGam_Musica_Fondo_rndr", stdVert.c_str(), frag.c_str());
}

//--------------------------------------------------------------------------------

void SNGam_Musica_Fondo::loadCalib()
{
//	printf("loading calibration \n");
	cv::FileStorage fs(calibFileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		fs["aux1"] >> aux1;
		fs["aux2"] >> aux2;
		fs["alpha"] >> alpha;
		fs["reflAmt"] >> reflAmt;
		fs["brightAdj"] >> brightAdj;
		fs["heightScale"] >> heightScale;
		fs["distAmp"] >> distAmp;
		fs["audioDistMed"] >> audioDistMed;
		fs["lineThick"] >> lineThick;
		fs["nrLines"] >> nrLines;
		fs["ringFadeOffs"] >> ringFadeOffs;
		fs["ringFade"] >> ringFade;
	}

	_hasNewOscValues = true;
	update(0.0, 0.0);
}

//--------------------------------------------------------------------------------

void SNGam_Musica_Fondo::saveCalib()
{
	cv::FileStorage fs(calibFileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		fs << "aux1" << aux1;
		fs << "aux2" << aux2;
		fs << "alpha" << alpha;
		fs << "reflAmt" << reflAmt;
		fs << "brightAdj" << brightAdj;
		fs << "heightScale" << heightScale;
		fs << "distAmp" << distAmp;
		fs << "audioDistMed" << audioDistMed;
		fs << "lineThick" << lineThick;
		fs << "nrLines" << nrLines;
		fs << "ringFadeOffs" << ringFadeOffs;
		fs << "ringFade" << ringFade;
	}
}

//----------------------------------------------------

SNGam_Musica_Fondo::~SNGam_Musica_Fondo()
{
	delete posTex;
	delete normTex;
	delete quadAr;
	delete normTexShdr;
	delete posTexShdr;
}
}
