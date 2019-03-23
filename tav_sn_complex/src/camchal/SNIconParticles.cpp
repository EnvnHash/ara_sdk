//
// SNIconParticles.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//	der osc befehl zum "klicken" eines icons ist "click", iconNr
//	in der update-methode wird gecheckt ob ein update gefordert wurde
//
//	im vao der partikel gibt es POSITION, AUX0 und AUX1
//  AUX0: x: clickTime + TimeDur (<0 when inactive) y: clickDuration z: icon Nr a: alpha


#define STRINGIFY(A) #A

#include "SNIconParticles.h"

namespace tav
{
SNIconParticles::SNIconParticles(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
{
	// setup chanCols
	getChanCols(&chanCols);

	nrIcons = glm::ivec2(4, 3);
	icons = new TextureManager();
	icons->loadTexture2D((*_scd->dataPath)+"/textures/camchal/iconos2.tga");

    propo = _scd->roomDim->x / _scd->roomDim->y;
    perlCoordScale = 1.f / propo;
    netAdjust = glm::scale(glm::mat4(1.f), glm::vec3(propo, 1.f, 1.f));

	timeVar = 0.15f;		// wie schnell der perlin noise wechselt
	iconCountDiv = 2;
	clickDur = 0.6f;
	zModDepth = 2.f;
	fotoZoomDur = 4.f;
	fotoZoomInitZ = -6.f;

	// net parameter
	nrRows = 5;
	nrCols = 15;
	nrZRows = 10;

	addPar("netDistAmt", &netDistAmt);
	addPar("zModDepth", &zModDepth);
	addPar("clickIcon", &clickIcon);
	addPar("circSize", &circSize);
	addPar("iconSize", &iconSize);

	addPar("foto3dZOffs", &foto3dZOffs);
	addPar("foto3dYScale", &foto3dYScale);
	addPar("foto3dXScale", &foto3dXScale);
	//addPar("foto3dZPos", & 0.f);

	addPar("netAlpha", &netAlpha);
	addPar("voluNetAlpha", &voluNetAlpha);
	addPar("voluNetZOffs", &voluNetZOffs);
	addPar("voluDrawDepth", &voluDrawDepth);
	addPar("voluNetDepth", &voluNetDepth);
	addPar("voluNetSizeX", &voluNetSizeX);
	addPar("voluNetSizeY", &voluNetSizeY);
	addPar("voluColBlend", &voluColBlend);
	addPar("perlTimeModu", &perlTimeModu);
	addPar("alpha", &alpha);
	addPar("lightAlpha", &lightAlpha);
	addPar("godLineCol", &godLineCol);
	addPar("recPointTY", &recPointTY);
	addPar("recPointSY", &recPointSY);

	//------------------------------------------------------------

	// 3d fotos
	std::vector<std::string> pathList;

	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_1_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_2_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_4_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_5_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_6_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_7_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_8_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_9_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_11_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_12_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_13_");
	pathList.push_back((*_scd->dataPath)+"/textures/camchal/kv_14_");

	nrFotos3d = static_cast<int>(pathList.size());
	std::cout << "nrFotos3d: " << nrFotos3d << std::endl;

	nrLayersFotos3d = 3;

	fotos3dFboSize = glm::ivec2(512, 512);
	fotos3d = new FBO(shCol, fotos3dFboSize.x, fotos3dFboSize.y,
				    	GL_RGBA8, GL_TEXTURE_2D, false, 1, 0, 1, GL_CLAMP_TO_EDGE, false);

	fotos3dTexs = new TextureManager**[nrFotos3d];
	for (int i=0;i<nrFotos3d;i++)
	{
		fotos3dTexs[i] = new TextureManager*[nrLayersFotos3d];

		for (int j=0;j<nrLayersFotos3d;j++){
			fotos3dTexs[i][j] = new TextureManager();
			fotos3dTexs[i][j]->loadTexture2D(pathList[i]+std::to_string(j)+".tga");
			//std::cout << "loading " << pathList[i]+std::to_string(j)+".tga" << "to [" << i << "][" << j << "]" <<std::endl;
		}
		//genLayerTex(fotos3dTexs[i], 3, &fotos3dLayerTexs[i]);
	}

	//----------------------------------------------------------


	pointRecModMat = glm::mat4(1.f);

	initCircles(_scd);
	initNet();
	initVoluNet2();


	stdColShdr = shCol->getStdCol();
	stdTexShdr = shCol->getStdTex();
	stdTexAlpaShdr = shCol->getStdTexAlpha();

	initVertShdr();

	init3dFotoShdr();
	initCircsShdr();
	initClickedShdr();
	initDrawShdr();
	initIconsShdr();
	initVoluShdr();
	initPointShdr();
	initGodRayShdr();

	// TFO recording
	initPointRecShdr();

    std::vector<std::string> names;
    names.push_back(stdRecAttribNames[0]); // POSITION
    names.push_back(stdRecAttribNames[6]); // AUX0
    names.push_back(stdRecAttribNames[7]); // AUX1

	pointTfo = new TFO(rawPart->getNrVertices(), names);
	pointTfo->setVaryingsToRecord(&names, recPointShdr->getProgram());
    recPointShdr->link();

	singleVert = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat sVert[] = { 0.f, 0.f, 0.f };
	singleVert->upload(POSITION, sVert, 1);


	blur = new FastBlurMem(0.48f, shCol, 1024, 125, GL_RGBA8);

    int fboWidth = std::min(_scd->screenWidth, 1024) / 2;
    int fboHeight = int(float(_scd->screenHeight) * float(fboWidth) / float(_scd->screenWidth));

	godRaysFbo = new FBO(shCol, fboWidth, fboHeight,
						GL_RGBA8, GL_TEXTURE_2D, false, 1, 0, 1, GL_CLAMP_TO_EDGE, false);

	lightCircle = new Circle(20, 0.1f, 0.f);
    lightPos = glm::vec3(0.8f, 0.4f, 0.f);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	noiseTex = new Noise3DTexGen(shCol,
			true, 4,
			256, 256, 64,
			4.f, 4.f, 16.f);
}



void SNIconParticles::initCircles(sceneData* _scd)
{
	nrCircus = glm::ivec2(4, 4);

	nrCircs = 5;
	circPars = new circPar[nrCircs];
	for (int i=0;i<nrCircs;i++) circPars[i].tex = new TextureManager();

	circPars[0].tex->loadTexture2D((*_scd->dataPath)+"/textures/camchal/wedges_fill.jpg");
	circPars[0].circSize = 1.2f;
	circPars[0].rotSpeed = -0.8453f;
	circPars[0].color = glm::vec4(0.4f, 0.4f, 0.4f, 0.5f);

	circPars[1].tex->loadTexture2D((*_scd->dataPath)+"/textures/camchal/wedges_fill3.jpg");
	circPars[1].circSize = 1.3f;
	circPars[1].rotSpeed = 0.64f;
	circPars[1].color = glm::vec4(0.4f, 0.4f, 0.4f, 0.5f);

	circPars[2].tex->loadTexture2D((*_scd->dataPath)+"/textures/camchal/wedges_fill4.jpg");
	circPars[2].circSize = 1.1f;
	circPars[2].rotSpeed = 1.3f;
	circPars[2].color = glm::vec4(0.4f, 0.4f, 0.4f, 0.8f);

	circPars[3].tex->loadTexture2D((*_scd->dataPath)+"/textures/camchal/wedges_lines1.jpg");
	circPars[3].circSize = 1.f;
	circPars[3].rotSpeed = 0.53f;
	circPars[3].color = glm::vec4(1.f, 1.f, 1.f, 1.f);

	circPars[4].tex->loadTexture2D((*_scd->dataPath)+"/textures/camchal/wedges_stroke2.jpg");
	circPars[4].circSize = 1.1f;
	circPars[4].rotSpeed = -0.74f;
	circPars[4].color = glm::vec4(1.f, 0.4f, 0.4f, 1.f);

}



void SNIconParticles::initNet()
{
	// von links nach recht und von unten nach oben, ein gitter von punkten
	GLfloat* positions = new GLfloat[(nrRows+1) * (nrCols+1) * 3];
	for (int row=0;row<nrRows+1;row++)
	{
		float rowInd = static_cast<float>(row) / static_cast<float>(nrRows);

		for (int col=0;col<nrCols+1;col++)
		{
			float colInd = static_cast<float>(col) / static_cast<float>(nrCols);
			int ind = (row * (nrCols+1) + col) * 3;

			glm::vec4 pos = glm::vec4(
					colInd * 2.f - 1.f,
					rowInd * 2.f - 1.f,
					0.f, 1.f);
			pos = netAdjust * pos;

			positions[ind] = pos.x;
			positions[ind +1] = pos.y;
			positions[ind +2] = pos.z;
		}
	}

	//circleParameter x=clickstartTime, clickDur, iconNr, alpha,
	int iconNr = 0;
	GLfloat* aux0 = new GLfloat[(nrRows+1) * (nrCols+1) * 4];
	for (int row=0;row<nrRows+1;row++)
	{
		float rowInd = static_cast<float>(row) / static_cast<float>(nrRows);

		for (int col=0;col<nrCols+1;col++)
		{
			float colInd = static_cast<float>(col) / static_cast<float>(nrCols);
			int ind = (row * (nrCols+1) + col) * 4;

			aux0[ind] = 0.f;
			aux0[ind +1] = 0.f;
			if ((ind/4) % iconCountDiv == 0)
			{
				aux0[ind +2] = float(iconNr) + 0.001f; // iconNr +0.001 = avoid rounding errors
				aux0[ind +3] = 1.f;
				iconNr = (iconNr+1) % nrFotos3d;
			} else
			{
				aux0[ind +2] = 0; // iconNr +0.001 = avoid rounding errors
				aux0[ind +3] = 0.f;
			}
		}
	}

	iconNr = 0;

	//image Parameter x=imgstartTime, imgDur, imgNr, alpha,
	GLfloat* aux1 = new GLfloat[(nrRows+1) * (nrCols+1) * 4];
	for (int row=0;row<nrRows+1;row++)
	{
		float rowInd = static_cast<float>(row) / static_cast<float>(nrRows);

		for (int col=0;col<nrCols+1;col++)
		{
			float colInd = static_cast<float>(col) / static_cast<float>(nrCols);
			int ind = (row * (nrCols+1) + col) * 4;

			aux1[ind] = 1.f;
			aux1[ind +1] = 1.f;

			if ((ind/4) % iconCountDiv == 0)
			{
				aux1[ind +2] = float(iconNr) + 0.001f; // iconNr +0.001 = avoid rounding errors
				aux1[ind +3] = 1.f;
				iconNr = (iconNr+1) % nrFotos3d;
			} else
			{
				aux1[ind +2] = 0; // iconNr +0.001 = avoid rounding errors
				aux1[ind +3] = 0.f;
			}
		}
	}

	// base indices, triangles defined ccw
	GLuint nrColsP = nrCols +1;
	GLuint nrColsP2 = nrCols +2;
	GLuint baseInd[6] = { 0, 1, nrColsP, nrColsP, 1, nrColsP2 };

	GLuint* indices = new GLuint[nrRows * nrCols * 6];
	int ind = 0;

	for (int row=0;row<nrRows;row++)
		for (int col=0;col<nrCols;col++)
			for (int indiInd=0;indiInd<6;indiInd++)
				indices[ind++] = baseInd[indiInd] + col + row * nrColsP;


	// fuer drawLines
	part = new VAO("position:3f,aux0:4f,aux1:4f", GL_STATIC_DRAW);
	part->upload(POSITION, positions, (nrRows +1) * (nrCols +1));
	part->upload(AUX0, aux0, (nrRows +1) * (nrCols+1));
	part->upload(AUX1, aux1, (nrRows +1) * (nrCols+1));
	part->setElemIndices(nrRows *nrCols *6, indices);

	rawPart = new VAO("position:3f,aux0:4f,aux1:4f", GL_DYNAMIC_DRAW);
	rawPart->upload(POSITION, positions, (nrRows +1) * (nrCols +1));
	rawPart->upload(AUX0, aux0, (nrRows +1) * (nrCols+1));
	rawPart->upload(AUX1, aux1, (nrRows +1) * (nrCols+1));

	fotoPart = new VAO("position:3f,aux0:4f,aux1:4f", GL_DYNAMIC_DRAW);
	fotoPart->upload(POSITION, positions, (nrRows +1) * (nrCols +1));
	fotoPart->upload(AUX0, aux0, (nrRows +1) * (nrCols+1));
	fotoPart->upload(AUX1, aux1, (nrRows +1) * (nrCols+1));

	maxNrFotos3d = (nrRows+1) * (nrCols+1) / iconCountDiv;
	fotoPars = new fotoPar[maxNrFotos3d];
	for (int i=0;i<maxNrFotos3d;i++)
	{
		fotoPars[i].active = false;
		fotoPars[i].index = 0;
		fotoPars[i].initPos = glm::vec4(0.f);
		fotoPars[i].initTime = 0.0;
	}
}



void SNIconParticles::initVoluNet()
{
	float rowHeight = 1.f / static_cast<float>(nrRows);
	glm::vec4 pos;

	// von links nach recht und von unten nach oben, ein gitter von punkten
	GLfloat* positions = new GLfloat[(nrRows+1) * (nrZRows+1) * (nrCols+1) * 3];

	int ind = 0;
	for (int z=0;z<nrZRows+1;z++)
	{
		float zRowInd = static_cast<float>(z) / static_cast<float>(nrZRows);

		for (int row=0;row<nrRows+1;row++)
		{
			float rowInd = static_cast<float>(row) / static_cast<float>(nrRows);

			for (int col=0;col<nrCols+1;col++)
			{
				float colInd = static_cast<float>(col) / static_cast<float>(nrCols);

				pos = glm::vec4(
						colInd * 2.f - 1.f,
						(row * rowHeight) * 2.f - 1.f,
						zRowInd * -1.f, 1.f);
				pos = netAdjust * pos;

				positions[ind++] = pos.x;
				positions[ind++] = pos.y;
				positions[ind++] = pos.z;
			}
		}
	}


	GLuint* indices = new GLuint[(nrZRows * nrRows * nrCols  +  nrZRows * (nrRows+1) * nrCols)* 6];

	ind = 0;
	GLuint nrColsP = nrCols+1;
	GLuint nrColsP2 = nrCols+2;
	GLuint nrRowsP = nrRows+1;

	GLuint baseIndXY[6] = { 0, nrColsP, 1, 1, nrColsP, nrColsP2 };
	GLuint baseIndXZ[6] = { 0, nrColsP * nrRowsP, 1, 1, nrColsP * nrRowsP, nrColsP * nrRowsP +1 };

	for (int z=0;z<nrZRows;z++)
	{
		// zuerst die ebene x,y
		for (int row=0;row<nrRows;row++)
		{
			for (int col=0;col<nrCols;col++)
			{
				for (int triInd=0;triInd<6;triInd++)
				{
					indices[ind] = baseIndXY[triInd];	// triangle and corner offset
					indices[ind] += col; 				// column offset
					indices[ind] += row * (nrCols +1); 	// row offset
					indices[ind] += z * (nrCols +1) * (nrRows +1); 	// z offset

					ind++;
				}
			}
		}

		// dann die verbindungen in der ebene x,z Ebene
		for (int row=0;row<nrRows+1;row++)
		{
			for (int col=0;col<nrCols;col++)
			{
				for (int triInd=0;triInd<6;triInd++)
				{
					indices[ind] = baseIndXZ[triInd];	// triangle and corner offset
					indices[ind] += col; 							// column offset
					indices[ind] += row * (nrCols +1); 						// row offset
					indices[ind] += z * (nrCols +1) * (nrRows +1); 	// z offset

					ind++;
				}
			}
		}
	}

	voluNet = new VAO("position:3f", GL_STATIC_DRAW);
	voluNet->upload(POSITION, positions, (nrZRows +1) * (nrRows +1) * (nrCols+1));
	voluNet->setElemIndices((nrZRows * nrRows * nrCols + nrZRows * (nrRows+1) * nrCols) *6, indices);
}



void SNIconParticles::initVoluNet2()
{
	nrRows = 5;
	nrCols = 10;
	nrZRows = 10;

	glm::vec4 pos, modPos;
	int nrVertices = ((nrZRows +1) * (nrRows +1) * (nrCols)
					+ (nrZRows +1) * (nrRows   ) * (nrCols+1)
					+ (nrZRows   ) * (nrRows +1) * (nrCols+1)) * 2;

	unitCubeSize = glm::vec3(1.f / static_cast<float>(nrCols),
							 1.f / static_cast<float>(nrRows),
							-1.f / static_cast<float>(nrZRows) ); // z offset must be negative

	// von links nach recht und von unten nach oben, ein gitter von punkten
	GLfloat* positions = new GLfloat[nrVertices * 3];

	int ind = 0;
	for (int z=0;z<nrZRows+1;z++)
	{
		float zInd = static_cast<float>(z) / static_cast<float>(nrZRows);

		for (int y=0;y<nrRows+1;y++)
		{
			float yInd = static_cast<float>(y) / static_cast<float>(nrRows);

			for (int x=0;x<nrCols+1;x++)
			{
				float xInd = static_cast<float>(x) / static_cast<float>(nrCols);

				pos = glm::vec4(xInd, yInd, -zInd, 1.f);

				// linie von x[n] nach x[n+1]
				if (x < nrCols)	addLine(0, &pos, unitCubeSize, positions, &ind);

				// linie von y[n] -> y[n+1]
				if (y < nrRows) addLine(1, &pos, unitCubeSize, positions, &ind);

				// linie von z[n] -> z[n+1]
				if (z < nrZRows) addLine(2, &pos, unitCubeSize, positions, &ind);
			}
		}
	}

	voluNet2 = new VAO("position:3f", GL_STATIC_DRAW);
	voluNet2->upload(POSITION, positions, nrVertices);
}



void SNIconParticles::addLine(int ind, glm::vec4* pos, glm::vec3& unitCubeSize,
		GLfloat* positions, int* posInd)
{
	glm::vec4 offsPos[2];

	for (int i=0;i<2;i++) offsPos[i] = glm::vec4(pos->x, pos->y, pos->z, 1.f);	// make a copy for offsetting

	// offset to specific direction
	offsPos[1][ind] += unitCubeSize[ind];

	for (int i=0;i<2;i++)
	{
		// normalize to -1 | 1
		offsPos[i] = glm::vec4(offsPos[i].x * 2.f -1.f,
							   offsPos[i].y * 2.f -1.f,
							   offsPos[i].z,
							   1.f);
		offsPos[i] = netAdjust * offsPos[i];
	}

	for (int i=0;i<2;i++)
		for(int j=0;j<3;j++)
			positions[(*posInd)++] = offsPos[i][j];
}



void SNIconParticles::initVoluShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(
	layout(location = 0) in vec4 position;

	uniform sampler3D perlin;
	uniform float zOffs;
	uniform float timeVar;
	uniform float distAmt;
	uniform float voluDepth;
	uniform float perlCoordScale;
	uniform float perlTimeModu;

	out VS_GS_VERTEX
	{
	    vec4 rawPos;
	} vertex_out;


	void main() {
		vec3 perlCoord = vec3(position.x * perlCoordScale + timeVar * 0.2,
							  position.y * perlCoordScale + timeVar * 0.023,
							  0.3 * position.z + 0.3 + (sin(timeVar * 0.123) * 0.1 + 0.1));
		vec4 perlinCol = texture(perlin, perlCoord);	// 0 bis 0.5

		vec3 fixPerlCoord = vec3(position.x * perlCoordScale + 0.2,
								 position.y * perlCoordScale + 0.023,
								 0.3 * position.z + 0.3);
		vec4 fixPerlinCol = texture(perlin, fixPerlCoord);	// 0 bis 0.5

		perlinCol = mix(fixPerlinCol, perlinCol, perlTimeModu);

		perlinCol.xyz = perlinCol.xyz - vec3(0.25);		// -0.25 bis 0.25
		perlinCol.z -= 0.25;		// 0. bis -0.5
		float perlZOffs = perlinCol.z * -2.0; // nach 0 bis -1

		//vec4 modPos = position;
		vec4 modPos = vec4(position.xyz + distAmt * perlinCol.xyz, position.w);
		modPos.z = mod(position.z +1.0 + zOffs, 1.0); // position.z von 0 bis -1
		modPos.z -= 1.0; // 0 bis -1

		vertex_out.rawPos = modPos;

		modPos.z *= voluDepth;

		gl_Position = modPos;
	});

	vert = "// SNIconParticles Volumetric Net vertex shader\n" + shdr_Header
			+ vert;

	

	std::string geo = STRINGIFY(
	layout(lines, invocations=2) in;
	layout(line_strip, max_vertices=2) out;

	uniform mat4 m_pvm;
	uniform float drawDepth;
	uniform vec4 col;
	uniform float solidCol;
	uniform float unitSqrDpth;
	uniform float time;

	in VS_GS_VERTEX {
		vec4 rawPos;
	} vertex_in[];

	out GS_FS_VERTEX {
		vec4 color;
	    vec2 lineTexCoord;
	    float beamMult;
	    float timeOffs;
	} vertex_out;

	vec4 screenPos;

	void main() {
		// check if the line is longer than 0.5
		// if this is the case skip it
		float length = abs(vertex_in[1].rawPos.z - vertex_in[0].rawPos.z) > 0.5 ? 1.0 : 0.0;

		// depth check
		float depthCheck = (vertex_in[0].rawPos.z + 1.0) > drawDepth ? 0.0 : 1.0;
		float check = depthCheck + length;

		if( check < 1.0)
		{
			float depthAlpha = (1.0 - drawDepth) + vertex_in[0].rawPos.z; // most far away -> transparent
			depthAlpha *= min(-vertex_in[0].rawPos.z - unitSqrDpth, unitSqrDpth) / unitSqrDpth; // blend out to the front

			float timeOffs = mod( float(gl_PrimitiveIDIn) * 0.01, 1.0);
			float beamMult =  float( gl_PrimitiveIDIn % 12 == (int(time + timeOffs) % 12) );

			// gerad-zahlige tris
			// zeichne von index 2 nach 0 nach 1
			for (int i=0; i<gl_in.length(); i++)
			{
				vertex_out.lineTexCoord = vec2(float(i), 0.0);
				vertex_out.color = col;
				vertex_out.color.a *= mix(depthAlpha, 1.0, solidCol);
				vertex_out.timeOffs = timeOffs;
				vertex_out.beamMult = beamMult;
				gl_Position = m_pvm * gl_in[i].gl_Position;

				EmitVertex();
			}
			EndPrimitive();
		}
	});

	geo = "// SNIconParticles Volumetric Net geo shader\n" + shdr_Header
				+ geo;

	


	std::string frag = STRINGIFY(
			layout (location = 0) out vec4 color;
			uniform vec4 col;
			uniform float time;
			uniform float alpha;

			in GS_FS_VERTEX {
			    vec4 color;
			    vec2 lineTexCoord;
			    float beamMult;
			    float timeOffs;
			} vertex_in;

			void main() {
				color = vertex_in.color;
				color.r = (1.0 - min(vertex_in.lineTexCoord.x, 0.1) / 0.1);

				float beamPos = 1.0 - min( abs(vertex_in.lineTexCoord.x - mod(time, 1.0)), 0.05) / 0.05;
				color += vec4(1.0) * beamPos * vertex_in.beamMult;

				color.a *= alpha;
			});

	frag = "// SNIconParticles Volumetric Net frag shader\n" + shdr_Header
			+ frag;

	voluShdr = shCol->addCheckShaderText("iconVoluShader", vert.c_str(), geo.c_str(), frag.c_str());
}



void SNIconParticles::initVertShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	vertShdr = STRINGIFY(
	layout(location = 0) in vec4 position;
	layout(location = 6) in vec4 aux0;
	layout(location = 7) in vec4 aux1;

	uniform mat4 m_pvm;
	uniform mat4 modMat;

	uniform sampler3D perlin;

	uniform float timeVar;
	uniform float basePointSize;
	uniform float distAmt;
	uniform float zModDepth;
	uniform float perlCoordScale;

	out vec4 rec_position;
	out vec4 rec_aux0;
	out vec4 rec_aux1;

	out icon_inf
	{
	    vec4 paux0;
	    vec4 paux1;
	    vec4 rawPos;
	} vertex_out;


	void main() {
		vertex_out.paux0 = aux0;
		vertex_out.paux1 = aux1;

		vec3 perlCoord = vec3(position.x * perlCoordScale + (timeVar * 0.2),
							  position.y * perlCoordScale + (timeVar * 0.023),
							  0.3);
		vec4 perlinCol = texture(perlin, perlCoord);	// 0 bis 0.5
		perlinCol.xy = perlinCol.xy - vec2(0.25);		// -0.25 bis 0.25

		float zOffs = perlinCol.z * -2.0; // nach 0 bis -1
		gl_PointSize = basePointSize * (1.0 + zOffs);

		vec4 modPos = modMat * vec4(position.xy + perlinCol.xy * distAmt, zOffs, 1.0);
		vertex_out.rawPos = modPos;

		rec_position = modPos;
		rec_aux0 = aux0;
		rec_aux1 = aux1;

		modPos.z *= zModDepth;

		gl_Position = m_pvm * modPos;
	});

	vertShdr = "// SNIconParticles Draw Point Shader vertex shader\n" + shdr_Header
			+ vertShdr;


	

	basicVert = STRINGIFY(
	layout(location = 0) in vec4 position;
	layout(location = 6) in vec4 aux0;
	layout(location = 7) in vec4 aux1;
	uniform mat4 m_pvm;
	uniform float basePointSize;
	uniform float zModDepth;

	out icon_inf {
	    vec4 paux0;
	    vec4 paux1;
	    vec4 rawPos;
	} vertex_out;

	void main() {
		vertex_out.paux0 = aux0;
		vertex_out.paux1 = aux1;
		vertex_out.rawPos = position;

		gl_PointSize = basePointSize * (1.0 + position.z);

		gl_Position = m_pvm * vec4(position.x, position.y, position.z * zModDepth, position.w);
	});

	basicVert = "// SNIconParticles base vertex Shader\n" + shdr_Header + basicVert;

}



void SNIconParticles::initCircsShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;
	uniform sampler2D circs;
	uniform vec4 col;
	uniform float alpha;

	in icon_inf {
	    vec4 paux0;
	    vec4 paux1;
	    vec4 rawPos;
	} vertex_in;

	uniform float rot;
	uniform float totNrIcons;
	uniform vec2 iconSize;
	vec2 rotPc;

	void main() {
		// get texture part relativ to index
		vec2 texQuadSize = vec2(1.0 / iconSize.x, 1.0 / iconSize.y);
		vec2 texOffs = vec2(mod(vertex_in.paux0.z, iconSize.x) / iconSize.x,
							floor(vertex_in.paux0.z / iconSize.x) / iconSize.y);

		// translate to normalized space
		vec2 relToCent = gl_PointCoord.xy * texQuadSize;
		// offset half quad size to 0|0
		relToCent -= texQuadSize * 0.5;

		// rotate
		float rad = sqrt(relToCent.x * relToCent.x + relToCent.y * relToCent.y);
		float radAlpha = atan(relToCent.y, relToCent.x);
		radAlpha += rot;
		rotPc.x = cos(radAlpha) * rad;
		rotPc.y = sin(radAlpha) * rad;

		// back to texture space but leave offset
		rotPc += vec2(texQuadSize.x * 0.5, texQuadSize.y * 0.5);
		rotPc += texOffs;

		vec4 iconTex = texture(circs, rotPc);
		float bright = (iconTex.r + iconTex.g + iconTex.b) * 0.333;

		// dist to cent
		vec2 temp = gl_PointCoord - vec2(0.5);
		float f = dot(temp, temp);

		color = iconTex * col;
		color.a *= vertex_in.paux0.a * bright * (f > 0.25 ? 0.0 : 1.0);
		color.a *= alpha;
	});

	frag = "// SNIconParticles Draw Icons Shader\n" + shdr_Header + frag;

	circsShdr = shCol->addCheckShaderText("iconioCircs", basicVert.c_str(), frag.c_str());
}



void SNIconParticles::initClickedShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;
	uniform vec4 col;
	uniform float time;

	in icon_inf {
	    vec4 paux0;
	    vec4 paux1;
	    vec4 rawPos;
	} vertex_in;

	void main() {
		// animTime
		float aTime = max(vertex_in.paux0.x - time, 0.0) / vertex_in.paux0.y;
		// convert to a tri with one interval
		aTime = 1.0 - abs(aTime * 2.0 - 1.0);
		//aTime = 1.0;

		// dist to cent
		vec2 temp = gl_PointCoord - vec2(0.5);
		float f = (1.0 - (min(dot(temp, temp), 1.0) * 4.0)) * aTime;

		color = col;
		color.a *= vertex_in.paux0.a * f;
	});

	frag = "// SNIconParticles Draw Icons Shader\n" + shdr_Header + frag;

	clickShdr = shCol->addCheckShaderText("iconClicks", basicVert.c_str(), frag.c_str());
}



void SNIconParticles::initIconsShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string frag = STRINGIFY(
			layout (location = 0) out vec4 color;
	uniform sampler2D icons;

	in icon_inf
	{
	    vec4 paux0;
	    vec4 paux1;
	    vec4 rawPos;
	} vertex_in;

	uniform float rot;
	uniform float totNrIcons;
	uniform float alpha;
	uniform vec2 iconSize;

	void main() {
			vec2 temp = gl_PointCoord - vec2(0.5);
			float f = dot(temp, temp);

			// if draw is set to on and radius smaller than unit circle
			if (float(f < 0.23) * vertex_in.paux0.a > 0.1)
			{
				vec2 texCoord = vec2(mod(vertex_in.paux0.z, iconSize.x) / iconSize.x,
									 floor(vertex_in.paux0.z / iconSize.x) / iconSize.y);
				texCoord = texCoord + gl_PointCoord * vec2(1.0 / iconSize.x, -1.0 / iconSize.y);

				color = texture(icons, texCoord);
				float border = (0.23 -f) / 0.23; // 0 -1, aussen 0 innen 1

				color.a *= min(border, 0.2) / 0.2;
				color.a *= alpha;
			} else {
				discard;
			}
	});

	frag = "// SNIconParticles Draw Icons Shader\n" + shdr_Header + frag;

	iconsShdr = shCol->addCheckShaderText("iconIcons", basicVert.c_str(), frag.c_str());
}



void SNIconParticles::initDrawShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string frag = STRINGIFY(// GLSLParticleSystemFBO Emit Shader
	layout (location = 0) out vec4 color;

	in icon_inf
	{
	    vec4 paux0;
	    vec4 paux1;
	    vec4 rawPos;
	} vertex_in;

	uniform float alpha;
	void main() {
		color = vec4(vec3(0.1), 1.0 + vertex_in.rawPos.z);
		color.a *= 0.4 * alpha;
	});

	frag = "// SNIconParticles Draw Point Shader\n" + shdr_Header + frag;

	drawShdr = shCol->addCheckShaderText("iconDrawLines", vertShdr.c_str(), frag.c_str());
}



void SNIconParticles::initPointShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;
	in vec4 fsColor;
	float f;
	float grey = 0.3;
	uniform float alpha;
	void main() {
		vec2 temp = gl_PointCoord - vec2(0.5);
		float f = 1.0 - min(dot(temp, temp) * 4.0, 1.0);
		f = pow(f, 0.4);
		color = vec4(grey, grey, grey, f * alpha);
	});

	frag = "// SNIconParticles Draw Point Shader\n" + shdr_Header + frag;

	pointShdr = shCol->addCheckShaderText("iconPointDraw", basicVert.c_str(), frag.c_str());
}



void SNIconParticles::initPointRecShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;
	void main() {
		color = vec4(1.0);
	});

	frag = "// SNIconParticles Rec Point Shader\n" + shdr_Header + frag;

	recPointShdr = shCol->addCheckShaderTextNoLink("iconPointRecShader", vertShdr.c_str(), frag.c_str());
//	recPointShdr = shCol->addCheckShaderTextNoLink("iconPointRecShader", vert.c_str(), frag.c_str());
}



void SNIconParticles::init3dFotoShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(
	layout(location = 0) in vec4 position;
	layout(location = 1) in vec4 normal;
	layout(location = 2) in vec2 texCoord;

	uniform mat4 m_pvm;
	uniform vec2 screenTrans;
	uniform vec2 screenScale;
	uniform float zModDepth;

	out toFrag
	{
		vec2 texCoord;
	} vertex_out;

	void main() {
		//		gl_Position = vec4(position.xy + perlin.xy * distAmt, 0.0, 1.0);

		vec4 screenPos = m_pvm * position;
		screenPos /= screenPos.w;

		// transform back to 0
		screenPos.x += screenTrans.x;
		screenPos.y += screenTrans.y;

		// scale to whole screen
		screenPos.x *= screenScale.x;
		screenPos.y *= screenScale.y;

		vertex_out.texCoord = texCoord;
		gl_Position = screenPos;
//		gl_Position = m_pvm * position;
	});

	vert = "// SNIconParticles Foto 3d Shader\n" + shdr_Header + vert;

	

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;
	uniform sampler2D tex;

	in toFrag {
		vec2 texCoord;
	} vertex_in;

	void main()
	{
		vec4 iconTex = texture(tex, vertex_in.texCoord);
		color = iconTex;
	});

	frag = "// SNIconParticles Foto 3d Shader\n" + shdr_Header + frag;

	foto3dShdr = shCol->addCheckShaderText("iconFoto3d", vert.c_str(), frag.c_str());
}



void SNIconParticles::initGodRayShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(
	layout(location = 0) in vec4 position;
	layout(location = 1) in vec4 normal;
	layout(location = 2) in vec2 texCoord;

	uniform mat4 m_pvm;
	uniform vec2 lightPositionOnScreen;
	uniform float density;

	const int NUM_SAMPLES = 80;

	out toFrag
	{
		vec2 texCoord;
		vec2 deltaTextCoord;
	} vertex_out;

	void main() {
		vertex_out.deltaTextCoord = vec2(texCoord - lightPositionOnScreen.xy );
		vertex_out.deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;

		vertex_out.texCoord = texCoord;
		gl_Position = m_pvm * position;
	});

	vert = "// SNIconParticles GodRayShdr Shader\n" + shdr_Header + vert;

	

	std::string frag = STRINGIFY(
	layout (location = 0) out vec4 color;

	uniform sampler2D firstPass;
	uniform float exposure;
	uniform float decay;
	uniform float weight;
	uniform float alpha;

	const int NUM_SAMPLES = 80;

	in toFrag {
		vec2 texCoord;
		vec2 deltaTextCoord;
	} vertex_in;

	void main()
	{
		vec2 textCoo = vertex_in.texCoord;
		float illuminationDecay = 1.0;
		vec4 outCol = vec4(0.0);

		for(int i=0; i < NUM_SAMPLES ; i++)
		{
			 textCoo -= vertex_in.deltaTextCoord;
			 vec4 sampleCol = texture(firstPass, textCoo );
			 sampleCol *= illuminationDecay * weight;
			 outCol += sampleCol;
			 illuminationDecay *= decay;
		 }

		outCol *= exposure;

		float bright = (outCol.r + outCol.g + outCol.b) * 0.33333;
		outCol.a *= bright;
		outCol.a *= alpha;

		if (outCol.a < 0.02) discard;

		color = outCol;
	});

	frag = "// SNIconParticles GodRayShdr Shader\n" + shdr_Header + frag;

	godRayShdr =shCol->addCheckShaderText("godRayShdr", vert.c_str(), frag.c_str());;
}



void SNIconParticles::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	if(!inited)
	{

		inited = true;
	}

	netAlpha = netAlpha;
	iconAlpha = alpha;
	voluNetAlpha = voluNetAlpha;

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);

	drawMvp = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;

	drawVoluNet(time, dt, cp, _shader, _tfo);

	if(netAlpha > 0.001f || iconAlpha > 0.001f)
		recPoints(time, dt, cp, _shader, _tfo);

	if(netAlpha > 0.001f)
	{
		drawLines(time, dt, cp, _shader, _tfo);
		drawPoints(time, dt, cp, _shader, _tfo);
	}

	if(iconAlpha > 0.001f)
	{
		drawCircles(time, dt, cp, _shader, _tfo);
		drawClicks(time, dt, cp, _shader, _tfo);
		drawIcons(time, dt, cp, _shader, _tfo);
		drawFotos(time, dt, cp, _shader, _tfo);
	}
}



void SNIconParticles::recPoints(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
    glEnable(GL_RASTERIZER_DISCARD);

//    GLuint          g_transformFeedbackQuery = 0;
//    glGenQueries(1, &g_transformFeedbackQuery);
//    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, g_transformFeedbackQuery);

	// record points
	recPointShdr->begin();
	recPointShdr->setUniform1i("perlin", 0);
	recPointShdr->setUniform1f("timeVar", time * timeVar);	// per osc par
	recPointShdr->setUniform1f("distAmt", netDistAmt);	// per osc
	recPointShdr->setUniform1f("distZAmt", netZDistAmt);
	//recPointShdr->setUniformMatrix4fv("m_pvm", cp->mvp);	// punkte werden ohne mvp aufgenommen
	recPointShdr->setUniform1f("perlCoordScale", perlCoordScale);
	recPointShdr->setUniformMatrix4fv("modMat", &pointRecModMat[0][0]);

	pointTfo->bind();
	pointTfo->begin(GL_POINTS);

	// draw as points
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	rawPart->draw(GL_POINTS);

    pointTfo->end();
    pointTfo->unbind();

    // manually offset the counters, for perfromance reasons
    pointTfo->incCounters(part->getNrVertices());

//    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
//    GLuint pointsWritten;
//    glGetQueryObjectuiv(g_transformFeedbackQuery, GL_QUERY_RESULT, &pointsWritten);
//    std::cout << "pointsWritten: " << pointsWritten << std::endl;
//    std::cout << "rec Points: " << pointTfo->getTotalPrimWritten() << std::endl;

    glDisable(GL_RASTERIZER_DISCARD);
}



void SNIconParticles::drawLines(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw as lines
	drawShdr->begin();
	drawShdr->setUniform1i("perlin", 0);
	drawShdr->setUniform1f("timeVar", time * timeVar);	// per osc par
	drawShdr->setUniform1f("distAmt", netDistAmt);	// per osc
	drawShdr->setUniform1f("zModDepth", zModDepth);
	drawShdr->setUniform1f("alpha", netAlpha);	// per osc
	drawShdr->setUniformMatrix4fv("m_pvm", &drawMvp[0][0]);
	drawShdr->setUniformMatrix4fv("modMat", &pointRecModMat[0][0]);
	drawShdr->setUniform1f("perlCoordScale", perlCoordScale);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	part->drawElements(GL_TRIANGLES);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}



void SNIconParticles::drawVoluNet(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glm::mat4 mvp = cp->projection_matrix_mat4 * cp->view_matrix_mat4
 			* glm::translate(_modelMat, glm::vec3(0.f, 0.f, -unitCubeSize.z * voluNetDepth))
			* glm::scale(glm::mat4(1.f), glm::vec3(voluNetSizeX, voluNetSizeY, 1.f));
	// move one unit square to the front

	glm::vec4 lightCol = glm::mix(chanCols[0], glm::vec4(1.f), 0.5f);
	glm::vec4 black = glm::vec4( glm::vec3(godLineCol), 1.f);
	glm::mat4 circMvp = glm::translate(cp->projection_matrix_mat4 * cp->view_matrix_mat4, lightPos);
	glm::vec2 lightPosScreen = glm::vec2(lightPos.x / propo * 0.5f +0.5f, lightPos.y * 0.5f +0.5f);

	glm::vec4 volCol = glm::mix(glm::vec4(0.1, 0.1, 0.1, 0.8), chanCols[0], voluColBlend);
	volCol.a *= 0.8;



	// draw net to fbo
	godRaysFbo->bind();

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LINE_SMOOTH);

	stdColShdr->begin();
	stdColShdr->setUniformMatrix4fv("m_pvm", &circMvp[0][0]);
	lightCircle->setColor(lightCol.r, lightCol.g, lightCol.b, lightAlpha);
	lightCircle->draw();

	voluShdr->begin();
	voluShdr->setUniform1i("perlin", 0);
	voluShdr->setUniform1f("timeVar", time * timeVar);	// per osc par
	voluShdr->setUniform1f("perlTimeModu", perlTimeModu);
	voluShdr->setUniform1f("distAmt", netDistAmt);
	voluShdr->setUniform1f("alpha", 1.f);
	voluShdr->setUniform1f("perlCoordScale", perlCoordScale);

	voluShdr->setUniform1f("zOffs", voluNetZOffs);	// per osc par
	voluShdr->setUniform1f("voluDepth", voluNetDepth);
	voluShdr->setUniform1f("drawDepth", voluDrawDepth);
	//voluShdr->setUniform4fv("colGodRay", &black[0]);
	voluShdr->setUniform4fv("col", &black[0]);
	voluShdr->setUniform1f("solidCol", 1.f);
	voluShdr->setUniform1f("unitSqrDpth", -unitCubeSize.z);

	voluShdr->setUniform1f("time", time * 0.8f);

	voluShdr->setUniformMatrix4fv("m_pvm", &mvp[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	voluNet2->draw(GL_LINES);

	godRaysFbo->unbind();


//	blur->proc(godRaysFbo->getColorImg());


	// render result with godray shader
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	godRayShdr->begin();
	godRayShdr->setIdentMatrix4fv("m_pvm");
	godRayShdr->setUniform1i("firstPass", 0);

	godRayShdr->setUniform1f("exposure", 0.003f);
	godRayShdr->setUniform1f("decay", 0.9999f);
	godRayShdr->setUniform1f("density", 0.84f);
	godRayShdr->setUniform1f("alpha", voluNetAlpha);
	godRayShdr->setUniform1f("weight", 5.65f);
	godRayShdr->setUniform2f("lightPositionOnScreen", lightPosScreen.x, lightPosScreen.y);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, blur->getLastResult());
	glBindTexture(GL_TEXTURE_2D, godRaysFbo->getColorImg());

	quad->draw();

	godRayShdr->end();



	// render volu net raw
	glEnable(GL_LINE_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);

	voluShdr->begin();
	voluShdr->setUniform1f("alpha", voluNetAlpha *2.f);
	voluShdr->setUniform4fv("col", &volCol[0]);
	voluShdr->setUniform1f("solidCol", 0.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	voluNet2->draw(GL_LINES);


//	voluNet2->setStaticColor(1.f, 1.f, 1.f, 1.f);
//	stdColShdr->begin();
//	stdColShdr->setUniformMatrix4fv("m_pvm", &mvp[0][0]);
//	voluNet2->draw(GL_LINES);


}



void SNIconParticles::drawPoints(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw as points
	pointShdr->begin();
	pointShdr->setUniformMatrix4fv("m_pvm", &drawMvp[0][0]);
	pointShdr->setUniform1f("basePointSize", cp->actFboSize.y * 0.03f);
	pointShdr->setUniform1f("zModDepth", zModDepth);
	pointShdr->setUniform1f("alpha", netAlpha);
	pointShdr->setUniform1f("perlCoordScale", perlCoordScale);

	pointTfo->draw(GL_POINTS, nullptr, GL_POINTS, 1);
}



void SNIconParticles::drawCircles(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw circles
	circsShdr->begin();
	circsShdr->setUniform1i("circs", 0);
	circsShdr->setUniform2f("iconSize", float(nrCircus.x), float(nrCircus.y));	// per osc
	circsShdr->setUniform1f("basePointSize", circSize * cp->actFboSize.y);
	circsShdr->setUniform1f("zModDepth", zModDepth);
	circsShdr->setUniform1f("alpha", iconAlpha);
	circsShdr->setUniformMatrix4fv("m_pvm", &drawMvp[0][0]);

	for (int i=0;i<nrCircs;i++)
	{
		circsShdr->setUniform1f("rot", time * circPars[i].rotSpeed);	// per osc par
		circsShdr->setUniform4fv("col", &circPars[i].color[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, circPars[i].tex->getId());

		pointTfo->draw(GL_POINTS, nullptr, GL_POINTS, 1);
	}
}



void SNIconParticles::drawClicks(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// draw circles
	clickShdr->begin();
	clickShdr->setUniformMatrix4fv("m_pvm", &drawMvp[0][0]);
	clickShdr->setUniform1f("basePointSize", 0.5f * cp->actFboSize.y);
	clickShdr->setUniform1f("time", time);
	clickShdr->setUniform4fv("col", &chanCols[1][0]);
	clickShdr->setUniform1f("zModDepth", zModDepth);

	pointTfo->draw(GL_POINTS, nullptr, GL_POINTS, 1);
}



void SNIconParticles::drawIcons(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// draw icons
	iconsShdr->begin();
	iconsShdr->setUniform1i("icons", 0);
	iconsShdr->setUniform1f("basePointSize", iconSize * cp->actFboSize.y);
	iconsShdr->setUniform1f("totNrIcons", nrIcons.x * nrIcons.y);
	iconsShdr->setUniform2f("iconSize", float(nrIcons.x), float(nrIcons.y));	// per osc
	iconsShdr->setUniform1f("zModDepth", zModDepth);
	iconsShdr->setUniform1f("alpha", iconAlpha);	// per osc
	iconsShdr->setUniformMatrix4fv("m_pvm", &drawMvp[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, icons->getId());

	pointTfo->draw(GL_POINTS, nullptr, GL_POINTS, 1);
}



void SNIconParticles::drawFotos(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	glm::mat4 m_pvm;
	glm::mat4 model;
	glm::vec4* quadCoord = new glm::vec4[3];
	glm::vec2 size;

	const float fotoZOffs = foto3dZOffs;
	const float fotoXScale = foto3dXScale;
	const float fotoYScale = foto3dYScale;
	//const float zPos = foto3dZPos");
	const float zModDepth = zModDepth;

	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// make an ordered list with the fotos to draw, for avoiding the depth_test and blending problem
	std::map<double, fotoPar*> orderedList;

	// check if a foto has to be rendered
	for (int i=0;i<maxNrFotos3d;i++)
	{
		if ( fotoPars[i].active )
		{
			fotoPars[i].offsetPos = fotoPars[i].initPos;
			fotoPars[i].offsetPos.x *= 1.f / propo;
			//std::cout << glm::to_string(fotoPars[i].offsetPos) << std::endl;

			// from 0 -1, mit der zeit von 0 bis 1
			fotoPars[i].offsetTime = std::fmin( time - fotoPars[i].initTime, fotoZoomDur ) / fotoZoomDur;
			orderedList[ fotoPars[i].offsetTime ] = &fotoPars[i];
		}
	}

	// draw the ordered list
	for (std::map<double, fotoPar*>::iterator it = orderedList.begin(); it!= orderedList.end(); ++it)
	{
		// -- berechne den FBO----
		(*it).second->offsetPos.z = (1.f - (*it).second->offsetTime) * fotoZoomInitZ -3.f;

		if ((*it).second->offsetTime >= 1.f)
			(*it).second->active = false;

		
		// transform the images in a FBO, as if they were rendered to the screen

		model = glm::translate(glm::mat4(1.f), glm::vec3((*it).second->offsetPos));
		m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * model;

		// berechne die position des fotos in bildschirm koordinaten
		getScreenTrans(&m_pvm, quadCoord, &size);

		

		// bind fbo
		fotos3d->bind();
		fotos3d->clear();

		foto3dShdr->begin();
		foto3dShdr->setUniform1i("tex", 0);
		foto3dShdr->setUniform2f("screenTrans", -quadCoord[1].x, -quadCoord[1].y);
		foto3dShdr->setUniform2f("screenScale", 2.f / size.x, 2.f / size.y);
		foto3dShdr->setUniform1f("zModDepth", zModDepth);

		int saveIndex = std::min((*it).second->index, nrFotos3d-1);

		// draw in reverse order
		for (int j=0;j<nrLayersFotos3d;j++)
		{
			// transform image to position in space
			// referenz ist bild nr 0 = hintergrund
			model = glm::translate(
					glm::mat4(1.f),
					glm::vec3((*it).second->offsetPos.x,
							  (*it).second->offsetPos.y,
							  (*it).second->offsetPos.z + float(j) * fotoZOffs) );

			// "fake perspective" scale brutally the base foto [0] horizontally
			float scaleAmt = (*it).second->offsetTime * float(j) * fotoXScale;
			model = glm::scale(model, glm::vec3( (1.f - scaleAmt * 0.5f) + scaleAmt, 1.f, 1.f) );

			m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * model;
			foto3dShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
			//foto3dShdr->setUniformMatrix4fv("m_pvm", &drawMvp[0][0]);

			fotos3dTexs[saveIndex][j]->bind(0);

			quad->draw();
		}

		fotos3d->unbind();

		
		// zeichne das ergebnis
		model = glm::translate(_modelMat, glm::vec3((*it).second->initPos));

	//	std::cout << (*it).second->index << std::endl;

		// proportionales zeichnen, skalieren mit dem screenproportionen und  bildproportionen
		model = glm::scale(model,
				glm::vec3(fotos3dTexs[saveIndex][0]->getWidthF() / fotos3dTexs[saveIndex][0]->getHeightF(), 1.f, 1.f));

		float scaleTime = (*it).second->offsetTime;
//		float scaleTime = std::pow((*it).second->offsetTime, 2.f);
		model = glm::scale(model, glm::vec3( scaleTime, scaleTime, 1.f));

		m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * model;


		float thisAlpha = std::fmin(1.0 - (*it).second->offsetTime, 0.3) / 0.3;

		// draw FBO
		stdTexAlpaShdr->begin();
		stdTexAlpaShdr->setUniformMatrix4fv("m_pvm", &m_pvm[0][0]);
		stdTexAlpaShdr->setUniform1i("tex", 0);
		stdTexAlpaShdr->setUniform1f("alpha", thisAlpha);	// fade out, bischen frueher schluss

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fotos3d->getColorImg());

		quad->draw();
	}
}



void SNIconParticles::update(double time, double dt)
{
	int clickIconNr = static_cast<int>(getRawOscPar("clickIcon"));

	pointRecModMat = glm::translate(glm::mat4(1.f), glm::vec3(0.f, recPointTY, 0.f))
		* glm::scale(glm::mat4(1.f), glm::vec3(1.f, recPointSY, 1.f));

	// check if an icon was clicked
	if (clickIconNr != -1)
	{
		fotoPars[clickIconNr].initTime = time;

		VAO* vaos[2] = { part, rawPart };

		for (int i=0;i<2;i++)
		{
			// get a pointer to the aux0 buffer
			GLfloat* ptr = (GLfloat*) vaos[i]->getMapBuffer(AUX0);
			// offset to the requested icon
			// set the actual time + the click duration (in the shader this gets add subtracted by time
			ptr[clickIconNr * iconCountDiv *4] = time + clickDur;
			// set the actual time
			ptr[clickIconNr * iconCountDiv *4 +1] = clickDur;

			// get icon Nr
			if (i==1){
				fotoPars[clickIconNr].active = true;
				fotoPars[clickIconNr].index = static_cast<int>(ptr[clickIconNr * iconCountDiv *4 +2]);
			}

			// unmap the buffer
			vaos[i]->unMapBuffer();
		}

		

		// get th eposition of the clicked icon
		//GLfloat* ptr = (GLfloat*) rawPart->getMapBuffer(POSITION);
		//GLfloat* ptr = (GLfloat*) pointTfo->getTFOBuf(POSITION);
        glBindBuffer(GL_ARRAY_BUFFER, pointTfo->getTFOBuf(POSITION));

            // get a pointer to the buffer
        GLfloat* ptr = (GLfloat*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

		// get icon Nr
		fotoPars[clickIconNr].initPos = glm::vec4(
				ptr[clickIconNr * iconCountDiv *4],
				ptr[clickIconNr * iconCountDiv *4 +1],
				ptr[clickIconNr * iconCountDiv *4 +2] * zModDepth,
				ptr[clickIconNr * iconCountDiv *4 +3]);


		// unmap the buffer
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

		

		// reset the value
//		oscPar["clickIcon"].second = -1.f;
//		oscPar["clickIcon"].first->clear();
//		oscPar["clickIcon"].first->update(-1.f);
	}

	// if from inactive to active
	if(time - lastUpdt > 0.2)
	{
//		oscPar["netAlpha"].second = 0.f;
//		oscPar["netAlpha"].first->clear();
//		oscPar["netAlpha"].first->update(0.f);
//
//		oscPar["voluNetAlpha"].second = 0.f;
//		oscPar["voluNetAlpha"].first->clear();
//		oscPar["voluNetAlpha"].first->update(0.f);
	}

	lastUpdt = time;
}



void SNIconParticles::getScreenTrans(glm::mat4* m_pvm, glm::vec4* quadCoord, glm::vec2* size)
{
	// berechne transformation von screenposition nach transformation mit m_pvm nach 0|0
	// get relative width in screenSpace
	quadCoord[0] = glm::vec4(-1.f, -1.f, 0.f, 1.f);
	quadCoord[1] = glm::vec4(0.f, 0.f, 0.f, 1.f);
	quadCoord[2] = glm::vec4(1.f,  1.f, 0.f, 1.f);

	for (int j=0;j<3;j++)
	{
		quadCoord[j] = (*m_pvm) * quadCoord[j];
		quadCoord[j] /= quadCoord[j].w;
	}

	*size = glm::vec2(std::abs(quadCoord[2].x - quadCoord[0].x),
					  std::abs(quadCoord[2].y - quadCoord[0].y));
}


// all layers must have same dimensions

void SNIconParticles::genLayerTex(TextureManager** texAr, int nrLayers, GLuint* texNr)
{
	GLsizei mipLevelCount = 1;

	glGenTextures(1, texNr);
	glBindTexture(GL_TEXTURE_2D_ARRAY, *texNr);

	//Allocate the storage.
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGBA8,
			texAr[0]->getWidth(), texAr[0]->getWidth(), nrLayers);

	//Upload pixel data.
	//The first 0 refers to the mipmap level (level 0, since there's only 1)
	//The following 2 zeroes refers to the x and y offsets in case you only want to specify a subrectangle.
	//The final 0 refers to the layer index offset (we start from index 0 and have 2 levels).
	//Altogether you can specify a 3D box subset of the overall texture, but only one mip level at a time.

	for (int i=0;i<nrLayers;i++)
	{
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
				0,	// level
				0, 0, // x and y offset
				i,	// z offset
				texAr[i]->getWidth(),  // width
				texAr[i]->getHeight(), // height
				1,	// depth
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				texAr[i]->getBits());
	}

	//Always set reasonable texture parameters
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}


SNIconParticles::~SNIconParticles()
{ }

}
