//
// SNVektorTunnel.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNVektorTunnel.h"

#define STRINGIFY(A) #A

namespace tav
{
SNVektorTunnel::SNVektorTunnel(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"LitSphere")
{
	osc = static_cast<OSCData*>(scd->osc);

	// setup chanCols
getChanCols(&chanCols);

	nrCircSegments = 20; 	// pro Kreis
	nrZSegments = 60;		// Z Segmente
	circRadius = 1.f;
	tunelLength = 140.f;
//	tunelLength = 30.f;
	typoScale = 0.1f;
	tunnelXDist = 10.f;
	tunnelYDist = 5.f;

	tunnelRenderDepth = 10.f; // per osc

	keyWordCircMod1 = 5;
	keyWordCircMod2 = 6;
	keyWordZMod = 1;

	propo = _scd->roomDim->x / _scd->roomDim->y;
	totemAndCenter = _scd->roomDim->x / (_scd->mark->at("lcTR3").x -_scd->mark->at("lcLeftBeam").x);

	// skalierung fuer den Tunnel
	modelScale = glm::scale(glm::mat4(1.f), glm::vec3(propo, 1.5f, 1.f));
	//    modelScale = glm::scale(glm::mat4(1.f), glm::vec3(totemAndCenter * 2.5f, 1.5f, 1.f));


	// add osc Parameter for control of camera
//	tunnelRenderDepth = 10.f;
	addPar("tunRenderDpth", &tunRenderDpth);
	addPar("alpha", &alpha);
	addPar("pathPos", &pathPos);
	addPar("typoBlend", &typoBlend);
	addPar("typoPosMin", &typoPosMin);	// index relativ auf die Laenge des Tunnels, ab wo keywords auftauchen sollen
	addPar("typoPosMax", &typoPosMax);	// index relativ auf die Laenge des Tunnels, bis wohin keywords auftauchen sollen
	addPar("typoAlpha", &typoAlpha);
	addPar("typoScale", &typoScale);
	addPar("typoBlendDist", &typoBlendDist);
	addPar("typoStayAmt", &typoStayAmt);
	addPar("typoFadeOut", &typoFadeOut);
	addPar("robotAlpha", &robotAlpha);
	addPar("robotPathPos", &robotPathPos);
	addPar("robotPathToCam", &robotPathToCam);
	addPar("robotXOffs", &robotXOffs);
	addPar("robotYOffs", &robotYOffs);
	addPar("robotSize", &robotSize);
	addPar("closeCamAngle", &closeCamAngle);
	addPar("closeCamDist", &closeCamDist);
	addPar("camBlend", &camBlend);
	addPar("axis1", &axis1);
	addPar("axis2", &axis2);
	addPar("axis3", &axis3);
	addPar("axis4", &axis4);
	addPar("zange", &zange);
	addPar("verbinder", &verbinder);

	litTex = new TextureManager();
	litTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/ikin_logo_lit.jpeg");

	robotLitTex = new TextureManager();
	robotLitTex->loadTexture2D( ((*scd->dataPath)+"textures/litspheres/kuka.jpeg").c_str() );

	robotLitBlackTex = new TextureManager();
	robotLitBlackTex->loadTexture2D( ((*scd->dataPath)+"textures/litspheres/Unknown-30_kl.jpeg").c_str() );

	normalTexture = new TextureManager();
	normalTexture->loadTexture2D( ((*scd->dataPath)+"textures/bump_maps/Unknown-20.jpeg").c_str() );

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);
	stdNormal = glm::vec3(0.f, 0.f, -1.f);

	camNormal = new Median<glm::vec3>(3.f);

	buildTunel();
	buildKeyWords();
	buildRobot();

	initPointShader();
	initLineShader();
	initTypoShader();
	initLitSphereShader();
	//initLitSphereAllVertShader();

	stdTextShader = shCol->getStdTexAlpha();
	stdDirLight = shCol->getStdDirLight();


	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);


	// setup custom matrix
	float globalScale = 16.f;
	view = glm::lookAt(
			glm::vec3(0.f, 0.f, globalScale),	// camPos
			glm::vec3(0.f), 			// lookAt
			glm::vec3(0.f, 1.f, 0.f));	// upVec

	float near = globalScale * 0.1f;
	float far = globalScale * 10.f;
	proj = glm::perspective(0.2f, propo, near, far);


	delete [] lines;
	delete [] linesColors;
	delete [] linesNormals;
}

//----------------------------------------------------

void SNVektorTunnel::calcCircNormals()
{
	glm::vec3 normMinusOneToOne;
	glm::vec3 normOneToPlusOne;
	glm::vec3 normFin;
	glm::vec3 basePoint;

	// gehe durch den Basis Pfad von z = 0 bis z = -1.0f;
	// berechne die normalen, nimm immer einen Punkt von naechsten Kreisabschnitt
	// und einen vom letzten, berechne zwei normalen n1 [-1][0] und n2 [0][1]
	// und berechnen deren mittelwert -> normale des Kreissegments
	for (auto i=0;i<nrZSegments;i++)
	{
		float fInd = static_cast<float>(i) / static_cast<float>(nrZSegments -1);
		float fIndPlusOne = 0;
		float fIndMinusOne = 0;

		basePoint = tunelBasePath.sampleAt(fInd);
		circBasePoints.push_back(basePoint);

		// for the first point just take the next point to calculate the normal
		if (i==0)
		{
			fIndPlusOne = static_cast<float>(i+1) / static_cast<float>(nrZSegments -1);
			normOneToPlusOne = tunelBasePath.sampleAt(fIndPlusOne) - basePoint;
			normFin = glm::normalize(normOneToPlusOne); // scale z with tunnel length

			circNormals.push_back(normFin);
			tunelNormalPath.push_back(normFin);
		}

		// beim letzten punkt kann keine Richtungsnormale mehr gebilder werden
		if (i > 0 && (i < nrZSegments-1))
		{
			fIndMinusOne = static_cast<float>(i-1) / static_cast<float>(nrZSegments -1);
			fIndPlusOne = static_cast<float>(i+1) / static_cast<float>(nrZSegments -1);

			normMinusOneToOne = basePoint - tunelBasePath.sampleAt(fIndMinusOne);
			normOneToPlusOne = tunelBasePath.sampleAt(fIndPlusOne) - basePoint;

			normFin = (normMinusOneToOne + normOneToPlusOne) / 2.f;
			normFin = glm::normalize(normFin); // scale z with tunnel length

			circNormals.push_back(normFin);
			tunelNormalPath.push_back(normFin);
		}

		// don't calculate the last normal, just take the last result
		if (i == (nrZSegments-1))
		{
			circNormals.push_back(circNormals.back());
			tunelNormalPath.push_back(circNormals.back());
		}
	}
}

//----------------------------------------------------

float SNVektorTunnel::frand()
{
	return rand() / (float) RAND_MAX;
}

//----------------------------------------------------

float SNVektorTunnel::sfrand()
{
	return frand()*2.0f-1.0f;
}

//----------------------------------------------------

void SNVektorTunnel::calcCircs()
{
	glm::mat4 transMat, rotMat;
	unsigned int i=0;

	// calculate for each BasePathNormal a circle of Points
	for (std::vector<glm::vec3>::iterator it = circNormals.begin(); it != circNormals.end(); it++)
	{
		// push back new vector for this circle
		circlesAsPoints.push_back( std::vector<glm::vec3>() );

		// calculate the rotationmatrix for rotating the circle according to the basePathNormal
		glm::quat rot = RotationBetweenVectors(stdNormal, (*it));
		rotMat = glm::mat4_cast(rot);

		for(unsigned int j=0;j<nrCircSegments;j++)
		{
			// winkel um den kreis zu interieren
			double alpha = static_cast<double>(j) / static_cast<double>(nrCircSegments) * M_PI * 2.0;

			// berechne den Kreis als waere er im Ursprung und als waere seine Normale (0|0|-1)
			// und multiplizieren dann mit der rotationsMatrix
			glm::vec4 newPoint = glm::vec4(
					static_cast<float>(std::cos(alpha) * circRadius),
					static_cast<float>(std::sin(alpha) * circRadius),
					0.f,
					1.f);

			transMat = glm::translate(glm::mat4(1.f), circBasePoints[i]);
			newPoint = transMat * rotMat * modelScale * newPoint;

			for (unsigned int k=0;k<3;k++)
				circPointsFlat[(i*nrCircSegments +j)*3 +k] = newPoint[k];

			circlesAsPoints.back().push_back( glm::vec3(newPoint) );
		}
		i++;
	}
}

//----------------------------------------------------

void SNVektorTunnel::buildLines()
{
	unsigned int linePtr=0;
	glm::vec3 norm;

	// gehe durch die kreise durch
	for (unsigned int zSegNr=0; zSegNr<nrZSegments; zSegNr++)
	{
		// gehe durch die punkte des kreises durch
		for (unsigned int pNr=0; pNr<nrCircSegments; pNr++)
		{
			// verbinde zu den naechsten kreisen
			// z-Offsets, bei 0 anfangen, so werden elegant die basiskreise gezeichnet,
			for (unsigned int zcNr=0; zcNr<connectNrPointsZ; zcNr++)
			{
				// verbinden zu den Nachbarpunkte auf dem aktuellen Kreis
				for (unsigned int cpNr=0; cpNr<connectNrPointsCirc; cpNr++)
				{
					// vermeide die linie mit zwei identischen punkten am anfang der zaehlung
					if ( !(zcNr == 0 && cpNr == 0) )
					{
						// berechne die versetzten indices im bezug auf den kreis
						unsigned int zOffsInd = std::min(int(zSegNr + zcNr), nrZSegments -1);

						// das waer symetrisch, sieht aber langweilig aus
						//int cOffsInd = (cpNr%2 == 0) ? 1 : -1;  // 1, -1, 1, -1, etc...
						//cOffsInd = cOffsInd * ((cpNr +1) / 2);	// 0, -1, 1, -2, etc...
						int cOffsInd = (cpNr%2 == 0) ? 1 : -1;  // 1, -1, 1, -1, etc...
						cOffsInd = cOffsInd * (cpNr +1 / 2 );	// 0, -1, 1, -2, etc...
						unsigned int pOffsInd = (pNr + cOffsInd +nrCircSegments) % nrCircSegments;

						// speichere einmal den aktuellen Punkt
						for (unsigned int i=0;i<3;i++)
							lines[linePtr *6 +i] = circlesAsPoints[zSegNr][pNr][i];

						// speichere einmal den offset Punkt
						for (unsigned int i=0;i<3;i++)
							lines[linePtr *6 +3 +i] =
									circlesAsPoints[zOffsInd][pOffsInd][i];


						// normals
						// normalen ist immer mittelpunkt zu kreispunkt
						norm = glm::normalize(circlesAsPoints[zSegNr][pNr] - circBasePoints[zSegNr]);
						for (unsigned int i=0;i<3;i++)
							linesNormals[linePtr *6 +i] = norm[i];

						norm = glm::normalize(circlesAsPoints[zOffsInd][pOffsInd] - circBasePoints[zOffsInd]);
						for (unsigned int i=0;i<3;i++)
							linesNormals[linePtr *6 +3 +i] = norm[i];


						// color
						for (unsigned int i=0;i<2;i++)
						{
							unsigned int colInd = (cpNr + zcNr) % 3;
							linesColors[linePtr *8 + (i*4) +0] = chanCols[colInd].r;
							linesColors[linePtr *8 + (i*4) +1] = chanCols[colInd].g;
							linesColors[linePtr *8 + (i*4) +2] = chanCols[colInd].b;
							linesColors[linePtr *8 + (i*4) +3] = chanCols[colInd].a;
						}

						linePtr++;
					}
				}
			}
		}
	}

	//	std::cout << "nrLines: " << nrLines << " linePtr" << linePtr << std::endl;
	lineVao->upload(POSITION, lines, linePtr * 2);
	lineVao->upload(NORMAL, linesNormals, linePtr * 2);
	lineVao->upload(COLOR, linesColors, linePtr * 2);
}

//----------------------------------------------------

void SNVektorTunnel::buildTunel()
{
	circPointsFlat = new float[nrZSegments * nrCircSegments * 3];
	connectNrPointsZ = 3;		// heisst zum selben [0],  +1 +2, etc
	connectNrPointsCirc = 3;	// heisst 0, -1  +1, -2, 2 ...
	nrLines = connectNrPointsZ * connectNrPointsCirc * nrZSegments * nrCircSegments;
	lines = new float[nrLines *2 *3];
	linesColors = new float[nrLines *2 *4];
	linesNormals = new float[nrLines *2 *3];

	pointVao = new VAO("position:3f", GL_STATIC_DRAW);
	lineVao = new VAO("position:3f,normal:3f,color:4f", GL_STATIC_DRAW);

	tunelBasePathRefPoints.push_back( glm::vec3(0.f,   0.f,    0.f)  );
	tunelBasePathRefPoints.push_back( glm::vec3(-1.f,	0.f,  	-0.1f) );
	tunelBasePathRefPoints.push_back( glm::vec3(-1.f, 	0.f, 	-0.2f) );
	tunelBasePathRefPoints.push_back( glm::vec3(-0.5f,  0.2f,	-0.3f) );
	tunelBasePathRefPoints.push_back( glm::vec3(-0.2f, 	0.3f,	-0.4f) );
	tunelBasePathRefPoints.push_back( glm::vec3(-0.15f, 0.1f,	-0.5f) );
	tunelBasePathRefPoints.push_back( glm::vec3(0.f, 	0.15f,	-0.6f) );
	tunelBasePathRefPoints.push_back( glm::vec3(0.2f, 	0.1f,	-0.7f) );
	tunelBasePathRefPoints.push_back( glm::vec3(0.1f,	0.2f,	-0.8f) );
	tunelBasePathRefPoints.push_back( glm::vec3(0.2f, 	0.f,	-0.9f) );
	tunelBasePathRefPoints.push_back( glm::vec3(0.1f, 	-0.05f,	-1.f) );

	// scale basePath and build Spline
	for (std::vector<glm::vec3>::iterator it = tunelBasePathRefPoints.begin(); it != tunelBasePathRefPoints.end(); ++it)
	{
		glm::vec3 newVec = glm::vec3(
				(*it).x * tunnelXDist,
				(*it).y * tunnelYDist,
				(*it).z * tunelLength);
		tunelBasePath.push_back( newVec );
		//std::cout << glm::to_string( newVec ) << std::endl;
	}

	calcCircNormals();
	calcCircs();

	// upload vao
	pointVao->upload(POSITION, circPointsFlat, nrCircSegments* nrZSegments);

	buildLines();
}

//----------------------------------------------------

void SNVektorTunnel::buildKeyWords()
{
	glm::vec3 normVec;
	glm::quat quat;
	glm::vec3 scVec = glm::vec3(1.f, 0.f, 0.f);

	keyWords.push_back("Industry 4.0");
	keyWords.push_back("Smart Mining");
	keyWords.push_back("Human Capital");
	keyWords.push_back("Digitalization");
	keyWords.push_back("Automation");
	keyWords.push_back("Robotics");

	for (std::vector<std::string>::iterator it=keyWords.begin(); it != keyWords.end(); ++it)
	{
		types.push_back(new FreetypeTex(((*scd->dataPath)+"/fonts/camchal/FagoCo-Medium.otf").c_str(), 40));
		types.back()->setText((*it));
	}

	// jedes segment hat nrCircSegments punkte
	//	for (int i=0;i<nrZSegments-2;i++)
//	int startInd = int(float(nrZSegments-1) * typoPosMin"));
//	int endInd = int(float(nrZSegments-1) * typoPosMax"));

	for (int i=0;i<int(keyWords.size());i++)
	{
		float zPosInd = static_cast<float>(i) / static_cast<float>(keyWords.size()-1);

		kwPar.push_back(keywordPar());

		// berechne den Vektor vom aktuellen Kreis auf den naechsten Kreis,
		// an diesem Punkt zum entsprechenden naechsten Punkt
		// berechne die Transformationsmatrize die 1|0|0 auf diesen Vector Transformiert
//		normVec = glm::normalize(circlesAsPoints[i+1][cn] - circlesAsPoints[i][cn]);
//		quat = RotationBetweenVectors(scVec, normVec);
//		kwPar.back().rotMat = glm::mat4(quat);

		// berechne die Transformationsmatrize die die Typo auf den aktuellen Punkt des Kreises verschiebt
		//kwPar.back().transMat = glm::translate(glm::mat4(1.f), circlesAsPoints[i][cn]);

		kwPar.back().flipX = 0.f;

		// generiere eine Endposition auf dem Bildschirm als transformationsmatrize
		// safety z offs
		kwPar.back().scrPosTrans = glm::translate(glm::mat4(1.f), glm::vec3(0.f,  0.f, -0.01f));
		kwPar.back().texPropo = float( types[i]->getWidth() ) / float( types[i]->getHeight() );

		// weise eine der Typo Texture zu
		kwPar.back().type = types[i]->getTex();

		// berechne die SkalierungsMatritze die der Groesse der TypoTextur und der voreinstellung entspricht
		kwPar.back().scaleMat = glm::scale(glm::mat4(1.f), glm::vec3(typoScale * propo, typoScale, 1.f));

		// weise per zufall ein der 5 farben zu
		kwPar.back().color = chanCols[i%2];

		// offset auf die Blendposition, damit alle nacheinander und nicht zusammen rausfliegen
		kwPar.back().blendOffs = zPosInd;
		//std::cout << "zPosOffs: " << kwPar.back().blendOffs << std::endl;
	}
}

//----------------------------------------------------

void SNVektorTunnel::buildRobot()
{
	// load robot arm
	robotNode = this->addChild();
	robotNode->setName(name);
	robotNode->setActive(true);

	AssimpImport* aImport = new AssimpImport(scd, true);
	aImport->load(((*scd->dataPath)+"models/robot_camchal.obj").c_str(), robotNode, [this](){
		//regScenes->dumpTree();
	});


	std::vector< std::vector<std::string> > objNames;

	// basis, unbeweglich
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(0.f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Axis_0.white"); //0-1
	objNames.back().push_back("Axis_0.black"); //0-1

	//------------------------------------------------------------


	// erste Achse, rotation um y-achse, Zentrum liegt genau auf Ursprung
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(0.f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Axis_1.white"); // 1-0

	//------------------------------------------------------------


	// zweite Achse, rotation um die x-achse,
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(0.00115f, 0.12208f, 0.00170f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Axis_2.white"); // 2-0
	objNames.back().push_back("Axis_2.black"); // 2-0

	//------------------------------------------------------------


	// zweite Achse, rotation um die x-achse,
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(0.00102f, 0.3004f, -0.13561f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Axis_3.white"); // 3-0
	objNames.back().push_back("Axis_3.black"); // 3-0

	//------------------------------------------------------------


	// no transformation, n	o origin needed
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(0.f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Axis_4.white"); // 4-0

	//------------------------------------------------------------


	// rotation x-axis
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(-0.00194f, 0.36129f, 0.26663f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Axis_5.black"); // 5-0

	//------------------------------------------------------------


	// greifzange, verbindungsstueck, keine Transformation
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(-0.00308f, 0.36104f, 0.32655f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("verbinder"); // 6-0

	//------------------------------------------------------------


	// greifzange, sicht von vorne -> links, rotation um die y-achse
	objNames.push_back(std::vector<std::string>());
	robotBaseNodeMats.push_back(glm::mat4(1.f));
	robotBaseNodeAxisCenters.push_back(glm::vec3(0.f));
	robotNodeAxisCenters.push_back( std::vector<glm::vec3>() );

	objNames.back().push_back("Zange_links.white"); // 7-0
	// hack, ist aus irgendeinem grund 3 meshes und nicht einer
	robotNodeAxisCenters.back().push_back(glm::vec3(-0.01698f, 0.3612f, 0.34663f));
	robotNodeAxisCenters.back().push_back(glm::vec3(-0.01698f, 0.3612f, 0.34663f));
	robotNodeAxisCenters.back().push_back(glm::vec3(-0.01698f, 0.3612f, 0.34663f));

	// greifzange, sicht von vorne -> rechts, rotation um die y-achse
	// hack, ist aus irgendeinem grund 3 meshes und nicht einer
	objNames.back().push_back("Zange_rechts.white"); // 7-1
	robotNodeAxisCenters.back().push_back(glm::vec3(0.01283f, 0.36117f, 0.34663f));
	robotNodeAxisCenters.back().push_back(glm::vec3(0.01283f, 0.36117f, 0.34663f));


	/*
	// get mesh names and build hierarchic SceneNodeGroups
	std::vector<std::string> meshNames = robotImport->getMeshNames();
	for (std::vector<std::string>::iterator it = meshNames.begin(); it != meshNames.end(); ++it) {
		//std::cout << "mesh names: [" << it - meshNames.begin() << "]" << (*it) << std::endl;
	}


	// loop through all name groups
	for (std::vector< std::vector<std::string> >::iterator it = objNames.begin(); it != objNames.end(); ++it)
	{
		//std::cout << "checking objGroup Nr: " << it - objNames.begin() << std::endl;
		//std::cout << "adding nrew VAO and Mat vector" << std::endl;

		// add a new vector fpt the results
		robotNodes.push_back( std::vector<VAO*>() );
		robotNodeMats.push_back( std::vector< glm::mat4 >() );
		robotMaterials.push_back( std::vector< MaterialProperties* >() );

		// loop through this group
		for (std::vector<std::string>::iterator subIt = (*it).begin(); subIt != (*it).end(); ++subIt)
		{
			//std::cout << "checking for " << (*subIt) << std::endl;

			// get all matches
			std::vector<std::string>::iterator searchPtr = meshNames.begin();
			while (searchPtr != meshNames.end())
			{
				std::vector<std::string>::iterator foundIt = std::find (searchPtr, meshNames.end(), (*subIt));
				if (foundIt != meshNames.end())
				{
					//	std::cout << "found " << (*subIt) << " at  " << foundIt - meshNames.begin() << std::endl;
					//std::cout << "adding " << (*subIt) << " to group nr " << it - objNames.begin() << std::endl;

					//std::cout << "found at index :" << foundIt - meshNames.begin() << std::endl;
					robotNodes.back().push_back( robotImport->getVao(foundIt - meshNames.begin()) );
					robotNodeMats.back().push_back( glm::mat4(1.f) );
					robotMaterials.back().push_back( robotImport->getMaterialForMesh(foundIt - meshNames.begin()) );

					//	std::cout << "robotNodeMats size " << robotNodeMats.back().size() << std::endl;

					searchPtr = foundIt;
				}
				searchPtr++;
			}
		}
	}
	*/
}

//----------------------------------------------------

void SNVektorTunnel::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}


	float pathPos = pathPos;
	float alpha = alpha;
	tunnelRenderDepth = tunRenderDpth;

	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//define "camera" position and viewing angle
	camPos = tunelBasePath.sampleAt( pathPos ) * -1.f;

	// define normal -> view angle
	camNormal->update( tunelNormalPath.sampleAt( pathPos ) );


	rot = RotationBetweenVectors(camNormal->get(), stdNormal);
	rotMat = glm::mat4_cast(rot);

	glm::mat4 tunnelCam = rotMat * _modelMat;
	tunnelCam = glm::translate(tunnelCam, camPos);


	// offset relativ zum Tunnel Basis Pfad
	glm::vec3 basePoint = tunelBasePath.sampleAt( robotPathPos );
	basePoint.z += robotPathToCam;
	robotPos = glm::translate(glm::mat4(1.f), glm::vec3(totemAndCenter * 2.5f + robotXOffs, robotYOffs, 0.f))
			 * glm::translate(glm::mat4(1.f), basePoint); // Offset zur Position auf dem Tunnel Basis Pfad

	// close up camera, bzw. model matrix
	basePoint = tunelBasePath.sampleAt( robotPathPos );
	float closeCamAngle = closeCamAngle;
	float closeCamDist = closeCamDist;
	closeUpCam = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, closeCamDist)) 	// move into screen
				* glm::rotate(glm::mat4(1.f), closeCamAngle, glm::vec3(0.f, 1.f, 0.f))	// rotate around y-axis
				* glm::translate(glm::mat4(1.f), -basePoint); // trans to Tunnel Base Pos


	camModMatr = glm::mix(tunnelCam, closeUpCam, camBlend);
	m_normal = glm::mat3( glm::transpose( glm::inverse( camModMatr ) ) );

	// draw one time directly to screen
	drawRobot(time, dt, cp, _shader, _tfo, &proj[0][0], &view[0][0]);
	//drawRobot(time, dt, cp, _shader, _tfo, cp->projection_matrix, cp->view_matrix);

	// draw a second time into FBO only with closeUpCam
	//drawRobotCloseUp(time, dt, cp, _shader, _tfo, sideCam->getProjMatrPtr(), sideCam->getViewMatrPtr());

	// ---
	// draw scene

	//m_view = cp->projection_matrix_mat4 * cp->view_matrix_mat4;
	m_view = proj * view;

	drawLines(time, dt, cp, _shader, _tfo, alpha);
	drawPoints(time, dt, cp, _shader, _tfo, alpha);
	drawTypo(time, dt, cp, _shader, _tfo, alpha);

	// close ups: draw fbos
	//drawRobotCloseUpFbo(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo);
}

//----------------------------------------------------

void SNVektorTunnel::drawLines(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
		float alpha)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);

	glm::mat4 linesMat = camModMatr * _modelMat;

	lineShader->begin();
	lineShader->setUniformMatrix4fv("m_pv", &m_view[0][0]);
	lineShader->setUniformMatrix4fv("m_model", &linesMat[0][0]);
	//lineShader->setUniformMatrix3fv("m_normal", &m_normal[0][0]);
	//lineShader->setUniform3fv("lightPos", &lighPos[0]);
	lineShader->setUniform1f("tunelLength", tunnelRenderDepth);
	lineShader->setUniform1f("lineWidth", 0.03f);
	lineShader->setUniform1f("time", time);
	lineShader->setUniform1f("alpha", alpha);
	//lineShader->setUniform1i("litTex", 0);
	//lineVao->draw();
	lineVao->draw(GL_LINES);
	//	lineVao->draw(GL_LINES, _tfo, GL_LINES);
}

//----------------------------------------------------

void SNVektorTunnel::drawPoints(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
		float alpha)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glm::mat4 linesMat = camModMatr * _modelMat;

	pointShader->begin();
	pointShader->setUniformMatrix4fv("m_pv", &m_view[0][0]);
	pointShader->setUniformMatrix4fv("m_model", &linesMat[0][0]);
	pointShader->setUniform1f("tunelLength", tunnelRenderDepth);
	pointShader->setUniform1f("alpha", alpha);
	pointShader->setUniform1f("pointSize", cp->actFboSize.y * 0.03f);
	pointVao->draw(GL_POINTS, _tfo, GL_POINTS);
}

//----------------------------------------------------

void SNVektorTunnel::drawTypo(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
		float alpha)
{
	float blend = typoBlend;
	float typoBlendDist = typoBlendDist;
	float globalTypoAlpha = typoAlpha * alpha;
	float typoScale = typoScale;
	float pathPos = pathPos;
	float stayTime = typoStayAmt;
	float fadeOutMult = typoFadeOut;

	float typoMinPos = typoPosMin;	// index relativ auf die Laenge des Tunnels, ab wo keywords auftauchen sollen
	float typoMaxPos = typoPosMax;	// index relativ auf die Laenge des Tunnels, bis wohin keywords auftauchen sollen
	float typoPathLength = typoMaxPos - typoMinPos;

	// draw keywords
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	typoShader->begin();
	typoShader->setUniform1i("tex", 0);
	typoShader->setUniform1i("litTex", 1);
	typoShader->setUniform1f("tunelLength", tunelLength);
	litTex->bind(1);

	glm::mat4 typoMatScr;
	glm::mat4 typoMat;
	glm::vec3 basePoint;
	glm::vec3 basePoint2;
	glm::vec3 normVec;
	glm::mat4 thisTransMatScr;
	glm::mat4 thisTransMat;
	glm::mat4 thisRotMat;
	glm::mat3 thisNormMat;
	glm::quat quat;

	for (std::vector<keywordPar>::iterator it=kwPar.begin(); it != kwPar.end(); ++it)
	{
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.f), glm::vec3(typoScale * (*it).texPropo, typoScale, 1.f));

		float thisTypoPos = 1.f - std::min( (*it).blendOffs, 1.f); // position normalisiert, blending von 1 nach 0
		thisTypoPos = thisTypoPos * typoPathLength + typoMinPos;

		float toPathDiff = std::max(1.f - (thisTypoPos - pathPos) * typoBlendDist, 0.f);
		float pathDiffMax1 = std::min(toPathDiff, 1.f);


		basePoint = tunelBasePath.sampleAt( thisTypoPos );

		// zur bestimmung der normale berechen einen weiteren Punkt
		basePoint2 = tunelBasePath.sampleAt( thisTypoPos + 0.02f );

		normVec = glm::normalize(basePoint2 - basePoint);
		quat = RotationBetweenVectors(stdNormal, normVec);
		rotMat = glm::mat4(quat);

		// matrize fuer den screenspace
		thisTransMatScr = (*it).scrPosTrans * scaleMat;

		// matrize fuer den world space
		thisTransMat = glm::translate(glm::mat4(1.f), basePoint);
		thisTransMat = camModMatr * thisTransMat * rotMat * scaleMat;

		// matritzen werden geblendet
		typoMat = glm::mix(thisTransMat, thisTransMatScr, pathDiffMax1);
		thisNormMat = glm::mat3( glm::transpose( glm::inverse( typoMat ) ) );

		typoMat = cp->view_matrix_mat4 * typoMat;

		typoShader->setUniformMatrix4fv("m_vm", &typoMat[0][0]);
		typoShader->setUniformMatrix4fv("m_proj",  cp->projection_matrix);
		typoShader->setUniformMatrix3fv("m_normal", &thisNormMat[0][0]);
		typoShader->setUniform1f("flipX", (*it).flipX );
		typoShader->setUniform4fv("typeCol", &(*it).color[0]);
		//typoShader->setUniform1f("alpha",  (pathDiffMax1 - std::max(toPathDiff -1.f -stayTime, 0.f) * fadeOutMult) );
		typoShader->setUniform1f("alpha", globalTypoAlpha * (pathDiffMax1 - std::max(toPathDiff -1.f -stayTime, 0.f) * fadeOutMult) );

		(*it).type->bind(0);
		quad->draw(_tfo);
	}
}

//----------------------------------------------------

void SNVektorTunnel::drawRobot(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
		GLfloat* projM, GLfloat* viewM)
{
	glm::vec3 scVec = glm::vec3(1.f, 0.f, 0.f);
	glm::mat3 normalMatr = glm::mat3(1.f);
	glm::mat4 finalOffs  = camModMatr * robotPos;	//  aktuellen Camera Position
	glm::mat4 finalScale = glm::scale(glm::mat4(1.f), glm::vec3(robotNode->_scaleVec * robotSize));
	finalOffs *= finalScale;

	// draw robot
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	robotLitShader->begin();
	robotLitShader->setUniformMatrix4fv("m_view", viewM);
	robotLitShader->setUniformMatrix4fv("m_proj", projM);
	robotLitShader->setUniform1i("normalTex", 0);
	robotLitShader->setUniform1i("litsphereTexture", 1);
	robotLitShader->setUniform1f("alpha", robotAlpha);
	normalTexture->bind(0);
	robotLitTex->bind(1);
	robotLitBlackTex->bind(2);


	// define transformations
	float claw = zange;

	// define a rotate for the first axis
	robotBaseNodeMats[1] = glm::rotate(glm::mat4(1.f), axis1, glm::vec3(0.f, 1.f, 0.f));

	// define a rotation for the second axis
	// first move to center, than rotate around x axis, then translate back
	robotBaseNodeMats[2] = glm::translate(glm::mat4(1.f), robotBaseNodeAxisCenters[2])
	* glm::rotate(glm::mat4(1.f), axis2, glm::vec3(1.f, 0.f, 0.f))
	* glm::translate(glm::mat4(1.f), -robotBaseNodeAxisCenters[2]);

	// first move to center, than rotate around x axis, then translate back
	robotBaseNodeMats[3] = glm::translate(glm::mat4(1.f), robotBaseNodeAxisCenters[3])
	* glm::rotate(glm::mat4(1.f), axis3, glm::vec3(1.f, 0.f, 0.f))
	* glm::translate(glm::mat4(1.f), -robotBaseNodeAxisCenters[3]);

	// first move to center, than rotate around x axis, then translate back
	robotBaseNodeMats[5] = glm::translate(glm::mat4(1.f), robotBaseNodeAxisCenters[5])
	* glm::rotate(glm::mat4(1.f), axis4, glm::vec3(1.f, 0.f, 0.f))
	* glm::translate(glm::mat4(1.f), -robotBaseNodeAxisCenters[5]);

	// first move to center, than rotate around x axis, then translate back
	robotBaseNodeMats[6] = glm::translate(glm::mat4(1.f), robotBaseNodeAxisCenters[6])
	* glm::rotate(glm::mat4(1.f), verbinder, glm::vec3(0.f, 0.f, 1.f))
	* glm::translate(glm::mat4(1.f), -robotBaseNodeAxisCenters[6]);

	// zange links, rotier um y-achse
	// dirty hack---  aus irgendeinem Grund wird der Mesh 3 mal und nicht einmal geladen
	for (int i=0;i<3;i++)
		robotNodeMats[7][i] = glm::translate(glm::mat4(1.f), robotNodeAxisCenters[7][i])
	* glm::rotate(glm::mat4(1.f), -claw, glm::vec3(0.f, 1.f, 0.f))
	* glm::translate(glm::mat4(1.f), -robotNodeAxisCenters[7][i]);

	// zange rechts, rotier um y-achse
	// dirty hack---  aus irgendeinem Grund wird der Mesh 2 mal und nicht einmal geladen
	for (int i=3;i<5;i++)
		robotNodeMats[7][i] = glm::translate(glm::mat4(1.f), robotNodeAxisCenters[7][i])
	* glm::rotate(glm::mat4(1.f), claw, glm::vec3(0.f, 1.f, 0.f))
	* glm::translate(glm::mat4(1.f), -robotNodeAxisCenters[7][i]);

	hierachicalMat = glm::mat4(1.f);


	// loop through the nodes
	// hacking, the claw are not hierarchic, so render the last two objects not hierachically
	for (std::vector< std::vector<VAO*> >::iterator it = robotNodes.begin(); it != robotNodes.end(); ++it )
	{
		hierachicalMat *= robotBaseNodeMats[it - robotNodes.begin()];

		// loop through node group
		for (std::vector<VAO*>::iterator subIt = (*it).begin(); subIt != (*it).end(); ++subIt )
		{
			unsigned int ind = it -robotNodes.begin();
			unsigned int subInd = subIt - (*it).begin();

			thisModelMat = finalOffs * hierachicalMat;

			// brute force speed optimization
			//			if ( ind == 7  )
			if ( static_cast<unsigned int>(robotNodeMats[ind].size()) > subInd  )
			{
				thisModelMat *= robotNodeMats[ind][subInd];
				//				thisModelMat = finalOffs * hierachicalMat * robotNodeMats[ind][subInd];
			}

			thisNormMat = glm::mat3( glm::transpose( glm::inverse( thisModelMat ) ) );
			robotLitShader->setUniformMatrix3fv("m_normal", &thisNormMat[0][0]);
			robotLitShader->setUniformMatrix4fv("m_model", &thisModelMat[0][0]);


			if (glm::length( robotMaterials[ind][subInd]->getDiffuse() ) <= 1.f )
			{
				robotLitShader->setUniform1i("litsphereTexture", 2);
			} else {
				robotLitShader->setUniform1i("litsphereTexture", 1);
			}


			(*subIt)->drawElements(GL_TRIANGLES);
		}
	}
}

//----------------------------------------------------

void SNVektorTunnel::drawRobotCloseUp(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo,
		GLfloat* projM, GLfloat* viewM)
{
	camModMatr = closeUpCam;
	m_normal = glm::mat3( glm::transpose( glm::inverse( camModMatr ) ) );

	closeUpFbo->bind();
	closeUpFbo->clear();
	drawRobot(time, dt, cp, _shader, _tfo, sideCam->getProjMatrPtr(), sideCam->getViewMatrPtr());
	closeUpFbo->unbind();
}

//----------------------------------------------------

void SNVektorTunnel::drawRobotCloseUpFbo(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	// draw left and right to screen
	float projWidth = scd->mark->at("lcLeftBeam").x / scd->roomDim->y;
	float projPos = propo - projWidth;

	m_view = cp->projection_matrix_mat4 * cp->view_matrix_mat4;

	glm::mat4 leftSide = m_view * glm::translate(glm::mat4(1.f), glm::vec3(projPos, 0.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(projWidth, 1.f, 1.f));
	glm::mat4 rightSide = m_view * glm::translate(glm::mat4(1.f), glm::vec3(-projPos, 0.f, 0.f)) * glm::scale(glm::mat4(1.f), glm::vec3(projWidth, 1.f, 1.f));

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	stdTextShader->begin();
	stdTextShader->setUniform1i("tex", 0);
	stdTextShader->setUniform1f("alpha", sideCamAlpha);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, closeUpFbo->getColorImg());
	//glBindTexture(GL_TEXTURE_2D, robotLitTex->getId());

	stdTextShader->setUniformMatrix4fv("m_pvm", &leftSide[0][0]);
	quad->draw();

	stdTextShader->setUniformMatrix4fv("m_pvm", &rightSide[0][0]);
	quad->draw();
}

//----------------------------------------------------

void SNVektorTunnel::update(double time, double dt)
{}

//----------------------------------------------------

void SNVektorTunnel::initPointShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	uniform mat4 m_pv;
	uniform mat4 m_model;
	uniform float tunelLength;
	uniform float pointSize;
	vec4 modPos;
	out vec4 col;

	void main ()
	{
		modPos = m_model * position;
		float depthAlpha = min(max(1.0 + (modPos.z / tunelLength), 0.0), 1.0);
		depthAlpha = pow(depthAlpha, 3.2);

		col = vec4(0.5, 0.5, 0.5, depthAlpha * 0.8);
		gl_PointSize = pointSize * depthAlpha;
		gl_Position = m_pv * modPos;
	});
	vert = "// SNVektorTunnel pointshader, vert\n"+shdr_Header+vert;



	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	uniform float alpha;
	in vec4 col;

	void main()
	{
		vec2 temp = gl_PointCoord - vec2(0.5);
		float f = 1.0 - min(dot(temp, temp) * 4.0, 1.0);
		f = pow(f, 0.4);
		color = vec4(col.rgb, f * col.a * alpha);
	});

	frag = "// SNVektorTunnel pointshader shader, frag\n"+shdr_Header+frag;

	pointShader = shCol->addCheckShaderText("SNVektorTunnel::pointShader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNVektorTunnel::initLineShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 3 ) in vec4 color;
	uniform mat4 m_pv;
	uniform mat4 m_model;
	uniform float tunelLength;

	out VS_FS
	{
		vec4 col;
		vec2 texCoord;
	} vertex_out;

	vec4 modPos;
	vec4 litCol;

	void main ()
	{
		modPos = m_model * position;
		float depthAlpha = min(max(1.0 + (modPos.z / tunelLength), 0.0), 1.0);
		depthAlpha = pow(depthAlpha, 3.2);

		vertex_out.col = vec4(0.6, 0.6, 0.6, depthAlpha * 0.8);
		vertex_out.texCoord = (gl_VertexID % 3) == 0 ? vec2(0.0, 0.0)
				: (gl_VertexID % 3) == 1 ? vec2(0.5, 1.0) : vec2(1.0, 0.0);

		gl_Position = m_pv * modPos;
	});
	vert = "// SNVektorTunnel lineshader, vert\n"+shdr_Header+vert;



	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	in VS_FS
	{
		vec4 col;
		vec2 texCoord;
	} vertex_in;

	uniform float alpha;
	uniform float lineWidth;

	void main()
	{
		vec3 texCol = vec3(vertex_in.texCoord, 1.0);

		// check border bottom, left and right
		//		float bottom = float(texCol.y < lineWidth);
		//		float left = float( abs(texCol.x * 2.0 - texCol.y) < lineWidth );
		//		float right = float( abs( (1.0 - texCol.x) * 2.0 - texCol.y ) < lineWidth );

		color = vec4(vertex_in.col.rgb, vertex_in.col.a * alpha);
		//		color = vec4(vertex_in.col.rgb, vertex_in.col.a * alpha * (left + right));
	});

	frag = "// SNVektorTunnel lineshader shader, frag\n"+shdr_Header+frag;

	lineShader = shCol->addCheckShaderText("SNVektorTunnel::lineshader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNVektorTunnel::initTypoShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";
	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;

	uniform mat4 m_vm;
	uniform mat4 m_proj;
	uniform mat3 m_normal;

	uniform vec4 typeCol;
	uniform float tunelLength;
	uniform float flipX;

	out VS_FS_DAT {
		vec4 pos;
		vec4 col;
		vec2 texCoord;
		vec2 vN;
	} vertex_out;

	void main ()
	{
	    vec3 e = normalize( vec3( m_vm * position ) );
	    vec3 n = normalize( m_normal * normal.xyz );

	    vec3 r = reflect( e, n );
	    float m = 2. * sqrt(
	        pow( r.x, 2. ) +
	        pow( r.y, 2. ) +
	        pow( r.z + 1., 2. )
	    );
	    vertex_out.vN = r.xy / m + .5;

		vertex_out.pos = position;
		vertex_out.texCoord = vec2(texCoord.x, 1.0 - texCoord.y);
		vertex_out.col = typeCol;

		gl_Position = m_proj * m_vm * position;
	});
	vert = "// SNVektorTunnel typo shader, vert\n"+shdr_Header+vert;

	//------------------------------------------------------------


	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;

	in VS_FS_DAT {
		vec4 pos;
		vec4 col;
		vec2 texCoord;
		vec2 vN;
	} vertex_in;

	uniform float alpha;
	uniform sampler2D tex;
	uniform sampler2D litTex;

	void main()
	{
	    vec4 litCol = texture( litTex, vertex_in.vN );
        float litBright = dot(litCol.rgb, vec3(0.2126, 0.7152, 0.0722)) * 1.5;

		color = texture(tex, vertex_in.texCoord) * vertex_in.col;
		color.rgb *= litBright;
		color.a *= alpha;
	});

	frag = "// SNVektorTunnel typo shader, frag\n"+shdr_Header+frag;

	typoShader = shCol->addCheckShaderText("SNVektorTunnelTypo", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNVektorTunnel::initLitSphereShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY( layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n

	out TO_FS {\n
		vec2 texCoord;\n
		vec3 normal;\n
		vec3 eye_pos;\n
		vec4 color;\n
	} vertex_out;\n

	uniform mat4 m_model;\n
	uniform mat4 m_view;\n
	uniform mat4 m_proj;\n

	void main ()
	{\n
		// litsphere tex_coords
		vertex_out.texCoord = position.xy + vec2(position.z); // hackish way of generating quick and dirty texture coordinate for random bump map
	vertex_out.color = color;
	vertex_out.normal = normal.xyz;
	vertex_out.eye_pos = normalize( vec3( m_view * m_model * position ) );
	gl_Position = m_proj * m_view * m_model * position;\n
	});

	vert = "// SNVektorTunnel litsphere shader, vert\n"+shdr_Header+vert;



	std::string frag = STRINGIFY(layout (location = 0) out vec4 fragColor;\n
	uniform sampler2D normalTex;\n
	uniform sampler2D litsphereTexture;\n
	uniform samplerCube cubeMap;\n
	uniform mat3 m_normal;\n
	uniform float alpha;\n
	uniform int texSel;\n

	in TO_FS {\n
		vec2 texCoord;\n
		vec3 normal;\n
		vec3 eye_pos;\n
		vec4 color;\n
	} vertex_in;\n

	void main() {\n

		vec3 modNorm = texture(normalTex, vertex_in.texCoord * 30.0).xyz;
	vec3 n = normalize( m_normal * normalize(vertex_in.normal.xyz + modNorm * 0.2));
	//vec3 n = normalize( m_normal * normalize(vertex_in.normal));

	vec3 r = reflect( vertex_in.eye_pos, n );
	float m = 2.0 * sqrt(
			pow( r.x, 2.0 ) +
			pow( r.y, 2.0 ) +
			pow( r.z + 1.0, 2.0 )
	);

	vec2 vN = r.xy / m + 0.5;\n
	vec3 base = texture( litsphereTexture, vN ).rgb;
	//  	vec4 reflCol = texture( cubeMap, r );

	fragColor = vec4(base * vertex_in.color.rgb, alpha);\n
	//	  	fragColor = vec4(base, 1.0) * vertex_in.color + reflCol * 0.06;\n
	});

	frag = "// SNVektorTunnel litsphere shader, frag\n"+shdr_Header+frag;

	robotLitShader = shCol->addCheckShaderText("SNVektorTunnel::robotLitShader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNVektorTunnel::initLitSphereAllVertShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY( layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n

	out TO_FS {\n
		vec2 vN;\n
	} vertex_out;\n

	uniform mat4 m_model;\n
	uniform mat4 m_view;\n
	uniform mat4 m_proj;\n
	uniform mat3 m_normal;\n
	uniform sampler2D normalTex;\n

	void main ()
	{\n
		// litsphere tex_coords
		vec2 modTexCoord = position.xy + vec2(position.z); // hackish way of generating quick and dirty texture coordinate for random bump map
	//		vertex_out.color = color;
	//		vertex_out.normal = normal.xyz;
	vec3 eye_pos = normalize( vec3( m_view * m_model * position ) );


	vec3 modNorm = texture(normalTex, modTexCoord * 30.0).xyz;
	vec3 n = normalize( m_normal * normalize(normal.xyz + modNorm * 0.2));
	//vec3 n = normalize( m_normal * normalize(vertex_in.normal));

	vec3 r = reflect( eye_pos, n );
	float m = 2.0 * sqrt(
			pow( r.x, 2.0 ) +
			pow( r.y, 2.0 ) +
			pow( r.z + 1.0, 2.0 )
	);

	vertex_out.vN = r.xy / m + 0.5;\n

	gl_Position = m_proj * m_view * m_model * position;\n
	});

	vert = "// SNVektorTunnel litsphere shader, vert\n"+shdr_Header+vert;



	std::string frag = STRINGIFY(layout (location = 0) out vec4 fragColor;\n
	uniform sampler2D litsphereTexture;\n
	//uniform samplerCube cubeMap;\n
	uniform float alpha;\n

	in TO_FS {\n
		vec2 vN;\n
	} vertex_in;\n

	void main() {\n

		vec3 base = texture( litsphereTexture, vertex_in.vN ).rgb;
	//  	vec4 reflCol = texture( cubeMap, r );

	fragColor = vec4(base, alpha);\n
	//	  	fragColor = vec4(base, 1.0) * vertex_in.color + reflCol * 0.06;\n
	});

	frag = "// SNVektorTunnel litsphere shader, frag\n"+shdr_Header+frag;

	robotLitShader = shCol->addCheckShaderText("SNVektorTunnel::robotLitShader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

SNVektorTunnel::~SNVektorTunnel()
{
	/*
        for (int i=0;i<mixDownToNrChans;i++)
        {
            delete quadArrays[i];

            for (int y=0;y<nrPllSnapShots;y++)
            {
                delete pllPositions[i][y];
            }

            for (int y=0;y<nrZSegments;y++)
            {
                delete position[i][y];
                delete pllNormals[i][y];
            }

            delete pllPositions[i];
        }
        delete quadArrays;
	 */
}
}
