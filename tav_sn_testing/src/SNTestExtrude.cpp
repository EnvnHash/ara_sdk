//
// SNTestExtrude.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//

#include "SNTestExtrude.h"

#define STRINGIFY(A) #A

namespace tav
{
SNTestExtrude::SNTestExtrude(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), baseFormNrPoints(8), baseFormRad(0.1f), nrBaseLinePoints(128)
{
#ifdef HAVE_AUDIO
	pa = (PAudio*) scd->pa;
#endif

	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// -------------- basis form ---------------------------

	// quadrat 8 punkte wegen normalen
	GLfloat pos[] = {
			baseFormRad,  baseFormRad, 0.f, 1.f, // re oben
			baseFormRad,  baseFormRad, 0.f, 1.f, // re oben
			-baseFormRad,  baseFormRad, 0.f, 1.f, // li oben
			-baseFormRad,  baseFormRad, 0.f, 1.f, // li oben
			-baseFormRad, -baseFormRad, 0.f, 1.f, // li unten
			-baseFormRad, -baseFormRad, 0.f, 1.f, // li unten
			baseFormRad, -baseFormRad, 0.f, 1.f, // re unten
			baseFormRad, -baseFormRad, 0.f, 1.f  // re unten
	};

	GLfloat norm[] = {
			1.f,  0.f, 0.f, 0.f, // re
			0.f,  1.f, 0.f, 0.f, // oben
			0.f,  1.f, 0.f, 0.f, // oben
			-1.f,  0.f, 0.f, 0.f, // li
			-1.f,  0.f, 0.f, 0.f, // li
			0.f, -1.f, 0.f, 0.f, // unten
			0.f, -1.f, 0.f, 0.f, // unten
			1.f,  0.f, 0.f, 0.f  // re
	};


	/*
     	GLfloat* pos = new GLfloat[baseFormNrPoints * 4];
    	GLfloat* norm = new GLfloat[baseFormNrPoints * 4];

 	 	// kreis
    	for (unsigned int i=0;i<baseFormNrPoints;i++)
    	{
    		float angle = static_cast<float>(i) / static_cast<float>(baseFormNrPoints -1) * M_PI * 2.f;
    		glm::vec2 cP = glm::vec2(std::cos(angle), std::sin(angle));

    		pos[i *4] = cP.x * baseFormRad * 0.1;
    		pos[i *4 +1] = cP.y * baseFormRad;
    		pos[i *4 +2] = 0.f;
    		pos[i *4 +3] = 1.f;

    		// normale = Kreisbasis Punkt
    		norm[i *4] = cP.x;
    		norm[i *4 +1] = cP.y;
    		norm[i *4 +2] = 0.f;
    		norm[i *4 +3] = 0.f;
    	}
	 */

	baseFormPosBuf = new TextureBuffer(baseFormNrPoints, 4, pos);
	baseFormNormBuf = new TextureBuffer(baseFormNrPoints, 4, norm);

	// ------------- trigger vao -------------------------------

	GLfloat* trig = new GLfloat[nrBaseLinePoints * 4];
	GLfloat halfPixOffset = 1.f / static_cast<float>(nrBaseLinePoints);

	for (unsigned int i=0;i<nrBaseLinePoints;i++)
	{
		float fInd = static_cast<float>(i) / static_cast<float>(nrBaseLinePoints);
		trig[i *4] = fInd * 2.f - 1.f + halfPixOffset;
		trig[i *4 +1] = 0.f;
		trig[i *4 +2] = 0.f;
		trig[i *4 +3] = 1.f;
	}

	trigVao = new VAO("position:4f", GL_STATIC_DRAW, nullptr, 1, true);
	trigVao->upload(POSITION, trig, nrBaseLinePoints);

	// ------------------------------------------------------

	// bau ein Vao mit Dummy Punkten und Indices
	GLfloat* baseObj = new GLfloat[nrBaseLinePoints * baseFormNrPoints *4];
	//memset(baseObj, 0, nrBaseLinePoints * baseFormNrPoints *4);
	for (unsigned int segNr=0; segNr<nrBaseLinePoints; segNr++)
	{
		float xPos = static_cast<float>(segNr) / static_cast<float>(nrBaseLinePoints -1);
		for (unsigned int bp=0; bp<baseFormNrPoints; bp++)
		{
			int ind = (segNr *baseFormNrPoints +bp) *4;
			float bpF = static_cast<float>(bp) / static_cast<float>(std::max(baseFormNrPoints -1, static_cast<unsigned int>(2)));

			baseObj[ind] = xPos * 2.f - 1.f;
			baseObj[ind +1] = std::sin(bpF * M_PI * 2.f) * baseFormRad;
			baseObj[ind +2] = std::cos(bpF * M_PI * 2.f) * baseFormRad - baseFormRad;
			baseObj[ind +3] = 1.f;
		}
	}

	GLuint* baseObjInd = new GLuint[(nrBaseLinePoints -1) * baseFormNrPoints *6];
	// prototyp fuer ein quad zwischen jeweils zwei Punkten zweier Segmente
	GLuint oneQuadInd[6] = { 0, 0, 1,  1, 0, 1 };
	GLuint oneQuadIndOffs[6] = { 0, baseFormNrPoints, 0,  0, baseFormNrPoints, baseFormNrPoints };
	//GLuint oneQuadInd[6] = { 0, baseFormNrPoints, 1,  1, baseFormNrPoints +1, baseFormNrPoints };

	for(unsigned int segNr=0; segNr<(nrBaseLinePoints -1); segNr++)
		for(unsigned int baseFormPointInd=0; baseFormPointInd<baseFormNrPoints; baseFormPointInd++)
			for(unsigned int quadInd=0; quadInd<6; quadInd++)
			{
				baseObjInd[(segNr *baseFormNrPoints + baseFormPointInd) *6 + quadInd] =
						((oneQuadInd[quadInd] + baseFormPointInd) % baseFormNrPoints) + oneQuadIndOffs[quadInd] + (segNr * baseFormNrPoints);

				//std::cout <<  "baseObjInd[" << (segNr *baseFormNrPoints + baseFormPointInd) *6 + quadInd << "]: " << baseObjInd[(segNr *baseFormNrPoints + baseFormPointInd) *6 + quadInd] << std::endl;
			}

	const char* format = "position:4f";
	baseVao = new VAO(format, GL_STATIC_DRAW, nullptr, 1, true);
	baseVao->upload(POSITION, &baseObj[0], nrBaseLinePoints * baseFormNrPoints );
	baseVao->setElemIndices((nrBaseLinePoints -1) * baseFormNrPoints *6, &baseObjInd[0]);

	// ------------------------------------------------------

	initBaseLineShdr();
	initMatrShdr();
	initTex1DShdr();
	initExtrShdr();

	// attachment 0: pos, 1: scale (x,y,z), 2: rotate (axisX, axisY, axisZ, angle) = quaternion
	baseLineFbo = new FBO(shCol, nrBaseLinePoints, 1, GL_RGBA32F, GL_TEXTURE_1D,
			false, 3, 1, 1, GL_REPEAT, false);
	matrFbo = new FBO(shCol, nrBaseLinePoints, 1, GL_RGBA32F, GL_TEXTURE_1D,
			false, 1, 1, 1, GL_REPEAT, false);

	//texShader = shCol->getStdTex();
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	litTex = new TextureManager();
	litTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/ikin_logo_lit.jpeg");

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube( ((*scd->dataPath)+"textures/skybox_camch.png").c_str() );

	ssao = new SSAO(_scd, SSAO::ALGORITHM_HBAO_CACHEAWARE, true,  4.f, 10.f);

}

// ------------------------------------------------------

SNTestExtrude::~SNTestExtrude()
{}

// ------------------------------------------------------

void SNTestExtrude::initTex1DShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(layout (location=0) in vec4 position;
	layout (location=2) in vec2 texCoord;
	out vec2 tex_coord;
	void main(void) {
		tex_coord = texCoord;
		gl_Position = position;
	});
	vert = "// SNAudioOptics  audio smooth vertex shader\n" +shdr_Header +vert;

	std::string frag = STRINGIFY(layout(location = 0) out vec4 color;
	in vec2 tex_coord;
	uniform sampler1D tex;
	void main()
	{
		color = texture(tex, tex_coord.x);
	});
	frag = "// SNAudioOptics audio smooth tex shader\n"+shdr_Header+frag;

	texShader = shCol->addCheckShaderText("SNTestExtrude", vert.c_str(), frag.c_str());
}

// ------------------------------------------------------

void SNTestExtrude::initBaseLineShdr()
{
	//------ Position Shader ---------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout (location=0) in vec4 position;\n

			uniform int nrBaseLinePoints;\n
			uniform float time;\n
			float pi = 3.1415926535897932384626433832795;

			out TO_FS {
				vec4 pos;
				vec4 scale;
				vec4 rot;
			} vertex_out;
			\n
			void main()\n
			{\n
				float fInd = float(gl_VertexID) / float(nrBaseLinePoints -1);
			vertex_out.pos = vec4( fInd * 2.0 - 1.0,
					0.0,
					-0.4, 1.0);\n

					float fact = sin((time + fInd) * 10.0) * 0.5 + 1.0;
					fact = 1.0;
					vertex_out.scale = vec4(fact, fact, fact, 1.0);\n

					float half_angle = fInd * mod(time, 8.0);
					vec3 rotationAxis = vec3(1.0, 0.0, 0.0);
					vertex_out.rot = vec4(rotationAxis.x * sin(half_angle),
							rotationAxis.y * sin(half_angle),
							rotationAxis.z * sin(half_angle),
							cos(half_angle));
					gl_Position = position;\n
			});

	vert = "// SNTestExtrude baseLine vertex shader\n" +shdr_Header +vert;

	//------- Frag Shader --------------

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(
			layout(location = 0) out vec4 pos;\n
			layout(location = 1) out vec4 scale;\n
			layout(location = 2) out vec4 rot;\n

			in TO_FS {
				vec4 pos;
				vec4 scale;
				vec4 rot;
			} vertex_in;

			void main()\n {
				pos = vertex_in.pos;
				scale = vertex_in.scale;
				rot = vertex_in.rot;
			});
	frag = "// SNTestExtrude baseLine frag shader\n"+shdr_Header+frag;

	baseLineShdr = shCol->addCheckShaderText("SNTestExtrude_basel",
			vert.c_str(), frag.c_str());
}

// ------------------------------------------------------

void SNTestExtrude::initMatrShdr()
{
	//------ Position Shader ---------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout (location=0) in vec4 position;\n

			uniform int nrBaseLinePoints;\n
			uniform sampler1D baseLineTex;\n
			//    	uniform samplerBuffer baseLineTex;\n
			float pi = 3.1415926535897932384626433832795;

			out vec4 outRot;

			// vektoren muessen normalisiert sein
			vec4 rotationBetweenVectors(vec3 start, vec3 dest) {\n
				float cosTheta = dot(start, dest);\n
				vec3 rotationAxis;\n
				if (cosTheta < -0.999)
				{\n
					rotationAxis = cross( vec3(0.0, 0.0, 1.0), start );\n
					if (dot(rotationAxis, rotationAxis) < 0.01){\n
						rotationAxis = cross( vec3(1.0, 0.0, 0.0), start );\n
					}\n
					rotationAxis = normalize(rotationAxis);\n

					float half_angle = pi * 0.5;
					return vec4(rotationAxis.x * sin(half_angle),
							rotationAxis.y * sin(half_angle),
							rotationAxis.z * sin(half_angle),
							cos(half_angle));

					//return angleAxis(180.0, rotationAxis);\n // grad???
				}\n
				rotationAxis = cross(start, dest);\n
				float s = sqrt( (1.0 + cosTheta) *2.0 );\n
				float invs = 1.0 / s;\n
				return vec4(rotationAxis.x * invs, rotationAxis.y * invs, rotationAxis.z * invs, s * 0.5);\n
			}\n
			vec3 getSegNorm(vec3 p1p0, vec3 p1p2)
			{
				// spezial fall wenn p1p0 und p1p2 parallel
				float isParallel = abs( length(p1p0 + p1p2) );

				vec3 up = normalize( cross(p1p0, p1p2) );\n
				// Ebenen Vektor der Ebene definiert durch p1p0 p1p2
				vec3 e = normalize(p1p0 + p1p2);\n
				// ziel normale des segments, basis
				vec3 b = normalize( cross(up, e) );\n

				return isParallel < 0.001 ? p1p2 : b;
			}
			\n
			void main()\n
			{\n
				vec4 p1 = texelFetch(baseLineTex, gl_VertexID, 0);\n

				// beim ersten Punkt der Basis Linie, kann p1p0 nicht berechnet werden
				vec4 p0 = texelFetch(baseLineTex, gl_VertexID > 0 ? gl_VertexID -1 : gl_VertexID +1, 0);\n
				vec4 p1p0 = normalize(p0 - p1) * (gl_VertexID > 0 ? 1.0 : -1.0);\n

				// beim letzten Punkt der Basis Linie kann p1p2 nicht berechnet werden
				vec4 p2 = texelFetch(baseLineTex, gl_VertexID < (nrBaseLinePoints -1) ? gl_VertexID +1 : gl_VertexID -1, 0);\n
				vec4 p1p2 = normalize(p2 - p1) * (gl_VertexID < (nrBaseLinePoints -1) ? 1.0 : -1.0);\n

				// berechne die Zielnormale des Segmentes
				vec3 b = getSegNorm(p1p0.xyz, p1p2.xyz);

				// berechne die rotation der Normale der der BasisForm zur Zielnormale
				outRot = rotationBetweenVectors(vec3(0.0, 0.0, 1.0), b);\n
				gl_Position = position;\n
			});

	vert = "// SNTestExtrude calcMatr vertex shader\n" +shdr_Header +vert;


	//------- Frag Shader --------------

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(
			layout(location = 0) out vec4 rot;\n
			in vec4 outRot;
			void main()\n {
				rot = outRot;
				rot.a = 1.0;
			});
	frag = "// SNTestExtrude calcMatr frag shader\n"+shdr_Header+frag;

	calcMatrShdr = shCol->addCheckShaderText("SNTestExtrude_matr",
			vert.c_str(), frag.c_str());
}

// ------------------------------------------------------

void SNTestExtrude::initExtrShdr()
{
	//------ Position Shader ---------

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
			layout (location=0) in vec4 position;\n

			uniform int nrBaseLinePoints;\n
			uniform int baseFormNrPoints;\n

			uniform sampler1D baseLineTex;\n
			uniform sampler1D baseLineScale;\n
			uniform sampler1D baseLineRot;\n
			uniform samplerBuffer baseFormPos;\n
			uniform samplerBuffer baseFormNorm;\n
			uniform sampler1D rotQuats;\n

			uniform mat4 m_pvm;\n
			uniform mat4 m_vm;\n
			uniform mat3 m_normal;\n

			out TO_FS {\n
				vec3 eye_pos;\n
				vec3 normal;\n
				vec2 vN;\n
			} vertex_out;\n
			\n
			vec3 rotate_vector( vec4 quat, vec3 vec )\n
			{
				return vec + 2.0 * cross( cross( vec, quat.xyz ) + quat.w * vec, quat.xyz );\n
			}
			\n
			void main()\n
			{\n
				int segInd = int(gl_VertexID / baseFormNrPoints);\n // bestimme Segment Index, es kommen Indizes rein,
				// die sich auf die Punkte der Basisformen beziehen
				int baseFormPointInd = gl_VertexID % baseFormNrPoints;\n	// bestimme um welchen Punkt in der Basis Form es sich handelt

				// get base Point
				vec4 p1 = texelFetch(baseLineTex, segInd, 0);\n

				// get rotation quaternion
				vec4 rotQuat = texelFetch(rotQuats, segInd, 0);\n

				// get baseform point
				vec4 baseP = texelFetch(baseFormPos, baseFormPointInd);\n
				vec4 baseN = texelFetch(baseFormNorm, baseFormPointInd);\n

				// apply prerendered rotation
				vec3 transP = rotate_vector(rotQuat, baseP.xyz);\n
				vec3 transNormal = rotate_vector(rotQuat, baseN.xyz);\n

				// additional rotation per BaselinePoint
				vec4 addRot = texelFetch(baseLineRot, segInd, 0);

				// apply additional rotation
				transP = rotate_vector(addRot, transP.xyz);
				transNormal = rotate_vector(addRot, transNormal.xyz);

				// scale
				transP *= texelFetch(baseLineScale, segInd, 0).xyz;

				// apply Offset defined through baseLine
				vec4 tP = vec4(transP + p1.xyz, 1.0);

				// ---- litsphere ---
				vec3 eye_pos = normalize( vec3( m_vm * tP ) );
				vec3 n = normalize( m_normal * transNormal );

				vec3 r = reflect( eye_pos, n );
				float m = 2. * sqrt(
						pow( r.x, 2. ) +
						pow( r.y, 2. ) +
						pow( r.z + 1., 2. )
				);

				vertex_out.vN = r.xy / m + .5;
				vertex_out.eye_pos = eye_pos;
				vertex_out.normal = n;\n

				gl_Position = m_pvm * tP;\n
			});

	vert = "// SNTestExtrude extr vertex shader\n" +shdr_Header +vert;


	//------- Frag Shader --------------

	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(
			layout(location = 0) out vec4 fragCol;\n
			uniform sampler2D litTex;
			uniform samplerCube cubeMap;

			in TO_FS {
				vec3 eye_pos;\n
				vec3 normal;\n
				vec2 vN;\n
			} vertex_in;

			const float Eta = 0.15; // Water

			void main()\n {

				vec4 litCol = texture( litTex, vertex_in.vN );
				// float litBright = dot(litCol.rgb, vec3(0.2126, 0.7152, 0.0722)) * 1.5;
				//  float litBright = litCol.r * litCol.g * litCol.b *1.5;

				// litsphere
				vec3 reflection = reflect( vertex_in.eye_pos, vertex_in.normal );
				// vec3 refraction = refract( vertex_in.eye_pos, vertex_in.normal, Eta );

				vec4 reflectionCol = texture( cubeMap, reflection );
				//   vec4 refractionCol = texture( cubeMap, refraction );
				//   float fresnel = Eta + (1.0 - Eta) * pow(max(0.0, 1.0 - dot(-vertex_in.eye_pos, vertex_in.normal)), 4.0);
				//  vec4 reflCol = 0.8 * mix(reflectionCol, refractionCol, min(max(fresnel, 0.0), 1.0));

				fragCol = mix( litCol, reflectionCol, 0.1 );
				//fragCol.a = 0.8;
				// fragCol = litCol;
			});
	frag = "// SNTestExtrude extr frag shader\n"+shdr_Header+frag;

	extrShdr = shCol->addCheckShaderText("SNTestExtrude_extr",
			vert.c_str(), frag.c_str());
}

// ------------------------------------------------------

void SNTestExtrude::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);             // is needed for drawing cubes

	// debug, show quaternion preprocessing result
	/*
        texShader->begin();
        texShader->setIdentMatrix4fv("m_pvm");
        texShader->setUniform1i("tex", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, matrFbo->getColorImg(1));

        quad->draw();
	 */

	glm::mat3 m_normal = glm::mat3( glm::transpose( glm::inverse( glm::mat4(1.f) ) ) );

	extrShdr->begin();
	extrShdr->setUniformMatrix4fv("m_pvm", cp->mvp);
	extrShdr->setUniformMatrix4fv("m_vm", cp->view_matrix);
	extrShdr->setUniformMatrix3fv("m_normal", &m_normal[0][0]);

	extrShdr->setUniform1i("nrBaseLinePoints", nrBaseLinePoints);
	extrShdr->setUniform1i("baseFormNrPoints", baseFormNrPoints);

	extrShdr->setUniform1i("baseLineTex", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, baseLineFbo->getColorImg(0));

	extrShdr->setUniform1i("baseLineScale", 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, baseLineFbo->getColorImg(1));

	extrShdr->setUniform1i("baseLineRot", 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_1D, baseLineFbo->getColorImg(2));


	extrShdr->setUniform1i("baseFormPos", 3);
	baseFormPosBuf->bindTex(3);

	extrShdr->setUniform1i("baseFormNorm", 4);
	baseFormNormBuf->bindTex(4);

	extrShdr->setUniform1i("rotQuats", 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_1D, matrFbo->getColorImg());

	extrShdr->setUniform1i("litTex", 6);
	litTex->bind(6);

	extrShdr->setUniform1i("cubeMap", 7);
	cubeTex->bind(7);

	baseVao->drawElements(GL_TRIANGLES, _tfo, GL_TRIANGLES);

	/// --------------------------

	/*
        ssao->copyFbo(cp);
		ssao->bind();
        ssao->proc(cp);
        ssao->drawAlpha(cp, 1.f);
        ssao->unbind();
	 */
}

// ------------------------------------------------------

void SNTestExtrude::update(double time, double dt)
{
	glDisable(GL_BLEND);

	// update baseLinePoints
	baseLineFbo->bind();
	baseLineFbo->clear();

	baseLineShdr->begin();
	baseLineShdr->setUniform1i("nrBaseLinePoints", nrBaseLinePoints);
	baseLineShdr->setUniform1f("time", time);

	trigVao->draw(GL_POINTS);

	baseLineFbo->unbind();

	//----------------------------------------------

	// update normals, and transformation Matrices
	matrFbo->bind();
	matrFbo->clear();

	calcMatrShdr->begin();
	calcMatrShdr->setUniform1i("baseLineTex", 0);
	calcMatrShdr->setUniform1i("nrBaseLinePoints", nrBaseLinePoints);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, baseLineFbo->getColorImg(0));

	trigVao->draw(GL_POINTS);

	matrFbo->unbind();
}
}
