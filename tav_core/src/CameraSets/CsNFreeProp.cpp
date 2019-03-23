//
//  CsNFree.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  Generisches Screen Setup für rechteckige Leinwände, die senkrecht zum Boden stehen
//  p0 "links unten" p1 "rechts oben"
//
//  Je nachdem wie viele <screen> tags im setup xml gesetzt sind, werden
//  entsprechend cameras generiert
//  die cameras werden über die Koordinaten der Projektionsflächen im realen Raum
//  definiert. Es werden die Grenzen aller Flächen gesucht und der resultierende
//  Kubus auf einen Einheitskubus gemappt (der virtuelle Raum, in den gerendert wird)
//  Anhand der Skalierung und der Translation, die nötig ist, um den einen Raum auf
//  den anderen zu mappen wird eine Transformationsmatrix berechnet, die noch parametrisch
//  angepasst werden kann (skalierung, translation). Die Anpassung erfolgt proportional -
//  die proportionen der realen Leinwände bleiben erhalten. Das ganze Setup kann per Angabe
//  des Mittelpunkte einer der Seiten des einheitskubus zu dieser bündig geschoben werden
//
//  die kameraposition und Blickrichtung resultiert aus der dist-variable aus der
//  xml-datei und der orthogonalen
//  aus der Mitte der Projektionsfläche in Richtung des Mittelpunktes
//
//  Near ist 0.1f und Far 100.0f
//  scrWidth und scrHeight sollten sich auf die Groesse beider Bildschirme
//  zusammen beziehen
//
//  die z-Koordinate in negativer Richtung bedeutet in den Bildschirm hinein
//
//  fehlt noch overlap. müsste so gehen:
//  überprüfe ob zwei screens auf derselben eben liegen, dazu den up-vector auf die
//  Ebene berechnen und die verbindung zwischen punkt1 p0 und punkt2 p0 berechnen
//  wenn beide orthogonal sind, liegen die screens auf derselben ebene
//  wenn dem so ist, projezieren die oberkante der screens auf eine ebene
//  (zwei linien) und rechne die überschneidung aus
//
//  vielleicht macht es aber mehr sinn, das overlap manuell anzugeben...

#include "CsNFreeProp.h"

namespace tav
{
CsNFreeProp::CsNFreeProp(sceneData* _scd, OSCData* _osc,
		std::vector<fboView*>* _screens) :
		CameraSet(int(_screens->size()), _scd, _osc), screens(_screens)
{
	id = _scd->id;
	nrCameras = (int) screens->size();
	cam = new GLMCamera*[nrCameras];

	overlap = 0.0f;

	// verschiebe die koordinaten des realen raumes in den virtuellen
	mapScreens();
	// intialisiere mit den gemappten Koordinaten die Cameras
	buildCams();

	setupCamPar();
}

CsNFreeProp::~CsNFreeProp()
{
}

//--------------------------------------------------------------------------------

void CsNFreeProp::initProto(ShaderProto* _proto)
{
	_proto->defineVert(false);
	if (!_proto->enableShdrType[GEOMETRY])
		_proto->defineGeom(false);
	_proto->defineFrag(false);

	if (nrCameras > 1)
	{
		_proto->asmblMultiCamGeo(); // add geo shader for multicam rendering
		_proto->addNRowsEdgeBlend(nrCameras); // add edge blending
		_proto->enableShdrType[GEOMETRY] = true;
	}

	_proto->assemble();
}

//--------------------------------------------------------------------------------

void CsNFreeProp::render(SceneNode* _scene, double time, double dt,
		unsigned int ctxNr)
{

	glScissor(0, 0, scrWidth, scrHeight);

	// call update method of the scene, if blending is enabled
	// this already gets called in the scene blender. the resulting morphscene
	// will have a empty update method
	_scene->updateOscPar(time, dt);
	_scene->update(time, dt);

	if (lightProto.find(_scene->protoName) != lightProto.end())
	{
		lightProto[_scene->protoName]->preRender(_scene, time, dt); // pre rendering e.g. for shadow maps
		lightProto[_scene->protoName]->shader->begin();
		lightProto[_scene->protoName]->sendPar(nrCameras, cp, osc, time); // 1 cameras
		lightProto[_scene->protoName]->shader->setUniform1f("scrWidth", scrWidth);

		// set overlap
		for (auto i=0; i<nrCameras - 1; i++)
		{
			float overL = screens->at(i)->overl <= 0.0001f ? 10000.f : 2.f / screens->at(i)->overl;
			lightProto[_scene->protoName]->shader->setUniform1f( "overlapV" + std::to_string(i), overL);
		}

		// set viewports
		const float wot = float(scrWidth) / static_cast<float>(nrCameras);
		for (auto i=0; i<nrCameras; i++)
		{
			float fInd = static_cast<float>(i);
			glViewportIndexedf(i, wot * fInd, 0.f, wot, scrHeight);
		}

		cp.camId = id;

		_scene->draw(time, dt, &cp, lightProto[_scene->protoName]->shader);
	}
	else
	{
		printf("CameraSet Error: couldn´t get LightProto \n");
	}
}

//--------------------------------------------------------------------------------

void CsNFreeProp::renderFbos(double time, double dt, unsigned int ctxNr)
{
}

//--------------------------------------------------------------------------------
/*
 // andere variante statt im fragment shader zu blenden, ist aber langsamer...
 void CsNFreeProp::textureEdgeBlend()
 {
 glViewportIndexedf(0, 0.f, 0.f, scrWidth, scrHeight);
 
 // wenn overlap, render noch weiche verläufe auf die kanten
 if (overlap > 0.f)
 {
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 texShader->begin();
 fullScrCam->sendIdentMtr(texShader->getProgram(), "m_pvm");

 glDisable(GL_DEPTH_TEST);

 tex.bind(0, 0, 0);
 for (auto i=0;i<nrCameras;i++) softEdgeQuads[i]->draw();
 tex.unbind();

 texShader->end();
 }
 //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
 }
 */
//--------------------------------------------------------------------------------
void CsNFreeProp::preDisp(double time, double dt)
{
}

//--------------------------------------------------------------------------------

void CsNFreeProp::postRender()
{
}

//--------------------------------------------------------------------------------

void CsNFreeProp::onKey(int key, int scancode, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void CsNFreeProp::mapScreens()
{
	// berechne die extreme der leinwände
	glm::vec3 rSpaceLimits[2] =
	{ glm::vec3(1000.f), glm::vec3(-1000.f) }; // min, max

	for (vector<fboView*>::iterator it = screens->begin(); it != screens->end();
			it++)
	{
		glm::vec3 points[2] =
		{ (*it)->p0, (*it)->p1 };
		getMinMax(rSpaceLimits, points);
	}

	// berechne die skalierungen für alle koordinaten
	float rSizes[3] =
	{ rSpaceLimits[1].x - rSpaceLimits[0].x, rSpaceLimits[1].y
			- rSpaceLimits[0].y, rSpaceLimits[1].z - rSpaceLimits[0].z };

	// suche nach max und minimum der skalierungen
	float rSizeLimits[2] =
	{ 1000.f, -1000.f }; // min, max
	for (auto i = 0; i < 3; i++)
	{
		if (rSizes[i] != 0.f && rSizes[i] < rSizeLimits[0])
			rSizeLimits[0] = rSizes[i];
		if (rSizes[i] != 0.f && rSizes[i] > rSizeLimits[1])
			rSizeLimits[1] = rSizes[i];
	}

	// wenn einer der werte nicht gesetzt wurde, stimmt was nicht
	if (rSizeLimits[0] == 1000.f || rSizeLimits[1] == -1000.f)
		std::cerr << "CsNFreeProp setup error, wrong Screen Definition!!!"
				<< std::endl;

	// wenn alle skalierungen > 2 sind skaliere runter, wenn nicht rauf
	float endScale = 1.f;
	endScale =
			rSizeLimits[1] > 2.f ?
					(2.f / rSizeLimits[1]) : (endScale = 2.f / rSizeLimits[0]);

	// baue eine transformations matrix mit der gefundenen Skalierung
	glm::mat4 transMat = glm::scale(glm::vec3(endScale));

	// wende die transMat an und überprüfe, ob die limits des einheitskubus
	// überschritten werden
	rSpaceLimits[0] = glm::vec3(1000.f);     // min
	rSpaceLimits[1] = glm::vec3(-1000.f);    // max

	for (vector<fboView*>::iterator it = screens->begin(); it != screens->end();
			it++)
	{
		mappedScreens.push_back(new fboView());

		// transformiere die Screen-Koordinaten
		glm::vec4 p0 = glm::vec4((*it)->p0.x, (*it)->p0.y, (*it)->p0.z, 1.0);
		p0 = transMat * p0;
		mappedScreens.back()->p0 = glm::vec3(p0.x, p0.y, p0.z);

		glm::vec4 p1 = glm::vec4((*it)->p1.x, (*it)->p1.y, (*it)->p1.z, 1.0);
		p1 = transMat * p1;
		mappedScreens.back()->p1 = glm::vec3(p1.x, p1.y, p1.z);

		mappedScreens.back()->dist = (*it)->dist;
		mappedScreens.back()->overl = (*it)->overl;

		glm::vec3 points[2] =
		{ mappedScreens.back()->p0, mappedScreens.back()->p1 };

		getMinMax(rSpaceLimits, points);
	}

	// wenn die grenzen überschritten werden, verschiebe
	glm::vec3 translate = glm::vec3(0.f);
	if (rSpaceLimits[0].x < -1.f)
		translate.x = -1.f - rSpaceLimits[0].x;
	if (rSpaceLimits[0].y < -1.f)
		translate.y = -1.f - rSpaceLimits[0].y;
	if (rSpaceLimits[0].z < -1.f)
		translate.z = -1.f - rSpaceLimits[0].z;

	if (rSpaceLimits[1].x > 1.f)
		translate.x = 1.f - rSpaceLimits[1].x;
	if (rSpaceLimits[1].y > 1.f)
		translate.y = 1.f - rSpaceLimits[1].y;
	if (rSpaceLimits[1].z > 1.f)
		translate.z = 1.f - rSpaceLimits[1].z;

	// wende das auf die transMat an
	transMat = glm::translate(translate) * transMat;

	// wende die matrix an
	applyMatrix(screens, mappedScreens, transMat);

	// wenn ein align angegeben wurde, verschiebe dementsprechend
	glm::vec3 alignToVec = glm::vec3(0.f);

	if (camAlign.x != 0.f || camAlign.y != 0.f || camAlign.z != 0.f)
	{
		// gehe durch die transformierten punkte
		for (vector<fboView*>::iterator it = mappedScreens.begin();
				it != mappedScreens.end(); it++)
		{
			glm::vec3 points[2] =
			{ (*it)->p0, (*it)->p1 };

			for (auto i = 0; i < 2; i++)
			{
				if (camAlign.x != 0.f && camAlign.x < 0.f
						&& points[i].x > camAlign.x)
					alignToVec.x = camAlign.x - points[i].x;

				if (camAlign.x != 0.f && camAlign.x > 0.f
						&& points[i].x < camAlign.x)
					alignToVec.x = camAlign.x - points[i].x;

				if (camAlign.y != 0.f && camAlign.y < 0.f
						&& points[i].y > camAlign.y)
					alignToVec.y = camAlign.y - points[i].y;
				if (camAlign.y != 0.f && camAlign.y > 0.f
						&& points[i].y < camAlign.y)
					alignToVec.y = camAlign.y - points[i].y;

				if (camAlign.z != 0.f && camAlign.z < 0.f
						&& points[i].z > camAlign.z)
					alignToVec.z = camAlign.z - points[i].z;

				if (camAlign.z != 0.f && camAlign.z > 0.f
						&& points[i].z < camAlign.z)
					alignToVec.z = camAlign.z - points[i].z;
			}
		}

		// wende das auf die transMat an
		transMat = glm::translate(alignToVec) * transMat;

		// wende die matrix an
		applyMatrix(screens, mappedScreens, transMat);
	}
}

//--------------------------------------------------------------------------------

void CsNFreeProp::getMinMax(glm::vec3* limits, glm::vec3* points)
{
	// get min
	for (auto i = 0; i < 2; i++)
	{
		if (points[i].x < limits[0].x)
			limits[0].x = points[i].x;
		if (points[i].y < limits[0].y)
			limits[0].y = points[i].y;
		if (points[i].z < limits[0].z)
			limits[0].z = points[i].z;
	}

	// get max
	for (auto i = 0; i < 2; i++)
	{
		if (points[i].x > limits[1].x)
			limits[1].x = points[i].x;
		if (points[i].y > limits[1].y)
			limits[1].y = points[i].y;
		if (points[i].z > limits[1].z)
			limits[1].z = points[i].z;
	}
}

//--------------------------------------------------------------------------------

void CsNFreeProp::applyMatrix(vector<fboView*>* inData,
		std::vector<fboView*>& outData, glm::mat4 transM)
{
	unsigned int ind = 0;
	for (vector<fboView*>::iterator it = inData->begin(); it != inData->end();
			it++)
	{
		glm::vec4 p0 = glm::vec4((*it)->p0.x, (*it)->p0.y, (*it)->p0.z, 1.0);
		p0 = transM * p0;
		outData[ind]->p0 = glm::vec3(p0.x, p0.y, p0.z);

		glm::vec4 p1 = glm::vec4((*it)->p1.x, (*it)->p1.y, (*it)->p1.z, 1.0);
		p1 = transM * p1;
		outData[ind]->p1 = glm::vec3(p1.x, p1.y, p1.z);

		ind++;
	}
}

//--------------------------------------------------------------------------------

void CsNFreeProp::buildCams()
{
	//float sWidth = 2.f / static_cast<float>(nrCameras);
	//float maxOverlap = 1.f / static_cast<float>(nrCameras);

	// berechne die camerapositionen und lookAts
	unsigned int ind = 0;
	for (vector<fboView*>::iterator it = mappedScreens.begin();
			it != mappedScreens.end(); it++)
	{
		// lookAt
		// berechne die Oberkante des rechtecks
		glm::vec4 edgeVec = glm::vec4((*it)->p1.x - (*it)->p0.x, 0.f,
				(*it)->p1.z - (*it)->p0.z, 0.f);

		// berechne eine Matrix, die diesen Vektor um 90 grad und die y-Achse dreht
		glm::mat4 rotMat = glm::rotate(static_cast<float>(M_PI),
				glm::vec3(0.f, 1.f, 0.f));

		// transformieren den Kanten Vektor und normalisiere ihn
		edgeVec = glm::normalize(edgeVec * rotMat);
		glm::vec3 screenNormal = glm::vec3(edgeVec.x, edgeVec.y, edgeVec.z);

		// schau ob der std::vector zum mittelpunkt zeigt, berechen dazu den vektor vom
		// Mittelpunkt des Screens zum Origin
		glm::vec3 screenMid = glm::vec3(((*it)->p1.x - (*it)->p0.x) * 0.5f,
				((*it)->p1.y - (*it)->p0.y) * 0.5f,
				((*it)->p1.z - (*it)->p0.z) * 0.5f) + (*it)->p0;

		float angleMidKante = glm::dot(glm::vec3(0.f) - screenMid,
				screenNormal);
		if (std::fabs(angleMidKante) > M_PI_2)
			screenNormal *= -1.f;

		// check for rounding errors
		if (screenNormal.x > -0.0000001 && screenNormal.x <= 0)
			screenNormal.x = 0;
		if (screenNormal.y > -0.0000001 && screenNormal.y <= 0)
			screenNormal.y = 0;
		if (screenNormal.z > -0.0000001 && screenNormal.z <= 0)
			screenNormal.z = 0;

		// lookAt ist Richtungsvektor der Camera, also umgedrehte normale des screens
		lookAt.push_back(screenNormal * -1.f);

		// Als Kamera Position wird immer der Ursprung angenommen
		// um die richtige Perspektive zu erhalten wird die Kamera
		// um die Variable dist aus dem ursprung bewegt
		camPos.push_back(glm::vec3(0.f) + screenNormal * (*it)->dist);

		// rotiere die screens in die x, y-Ebene um left, right, bottom, top
		// für das Frustrum auszurechnen
		glm::quat rotQuat = RotationBetweenVectors(screenNormal,
				glm::vec3(0.f, 0.f, 1.f));
		glm::mat4 rMat = glm::mat4_cast(rotQuat);

		glm::vec3 frustCoord0 = glm::vec3(
				rMat * glm::vec4((*it)->p0.x, (*it)->p0.y, (*it)->p0.z, 0.f));
		glm::vec3 frustCoord1 = glm::vec3(
				rMat * glm::vec4((*it)->p1.x, (*it)->p1.y, (*it)->p1.z, 0.f));

		// xPosition werden gleichmaessig verteilt => alle video ausgänge haben
		// dieselbe grösse
		// float fInd = static_cast<float>(ind);
		cam[ind] = new GLMCamera(GLMCamera::FRUSTUM, scrWidth / nrCameras,
				scrHeight, frustCoord0.x,
				frustCoord1.x,  // left, right
				frustCoord0.y, frustCoord1.y, camPos.back().x, camPos.back().y,
				camPos.back().z, // camPos
				lookAt.back().x, lookAt.back().y, lookAt.back().z);   // lookAt

		ind++;
	}
}

//--------------------------------------------------------------------------------

void CsNFreeProp::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
}

//--------------------------------------------------------------------------------

void CsNFreeProp::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
}

//--------------------------------------------------------------------------------

void CsNFreeProp::clearFbo()
{
}

}
