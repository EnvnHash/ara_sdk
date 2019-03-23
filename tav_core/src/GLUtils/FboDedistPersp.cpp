/*
*	FboDedistPersp.cpp
*	tav_core
*
*	Created by Sven Hahne on 22/12/15.
*	Copyright © 2015 Sven Hahne. All rights reserved.
*
*	Ein FboDedistPersp Objekt sollte dem Ziel-FBO einer Camera entsprechen
*	das Objekt haelt genau einen FBO mit der Groesse, die im Setup.xml per <camera fboWidth und fboHeight> angegeben wird
*	dieser FBO kann in beliebig viele GLFW Contexte an beliebige Stellen mit beliebigen Ausschnitten des ZielFBO gerendert werden
*
*	Alles was sich in einem Context befindet sollte mit einem GeoShader
*	in einem Schritt gerendet werden
*
*/

#define STRINGIFY(A) #A

#include "pch.h"
#include "FboDedistPersp.h"

namespace tav
{
FboDedistPersp::FboDedistPersp(sceneData* _scd, GWindowManager* _winMan) :
	scd(_scd), winMan(_winMan)
{
	commonInit();
}

//---------------------------------------------------------------

FboDedistPersp::FboDedistPersp(sceneData* _scd, GWindowManager* _winMan, int _fboWidth,
		int _fboHeight) :
	scd(_scd), winMan(_winMan), fboWidth(_fboWidth), fboHeight(_fboHeight)
{
	fboWidthF = static_cast<float>(fboWidth);
	fboHeightF = static_cast<float>(fboHeight);
	fboSize = glm::vec2(fboWidthF, fboHeightF);

	// ----------- Groesse des internen FBO ---------------------

	destFbo = new FBO(_scd->shaderCollector, fboWidth, fboHeight, GL_RGBA8,
			GL_TEXTURE_2D, true, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	commonInit();
}

//---------------------------------------------------------------

void FboDedistPersp::commonInit()
{
	cornersSize = 30;
	actBoundFbo = 0;
	xOffs = 0.f;

	shCol = static_cast<ShaderCollector*>(scd->shaderCollector);

	cornersNames[0] = "bl";
	cornersNames[1] = "br";
	cornersNames[2] = "tr";
	cornersNames[3] = "tl";

	float col[2] = { 1.f, 0.f };

	// ----------- VAO --------------------------------------------

	// standard quad
	quad = scd->stdQuad;

	quadAr = new QuadArray(10, 10, -1.f, -1.f, 2.f, 2.f, 1.f, 1.f, 0.f, 1.f);
	// standard quad
	redQuad = new Quad(-0.05f, -0.05f, 0.1f, 0.1f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 0.f, 0.f, 1.f);

	cornerQuad = new Quad*[2];
	for (auto i=0; i<2; i++)
		cornerQuad[i] = new Quad(-1.0f, -1.0f, 2.f, 2.f,
								glm::vec3(0.f, 0.f, 1.f),
								col[i], col[(i + 1) % 2], 0.f, 1.f);

	quadSkel[0] = 0.f;
	quadSkel[1] = 0.f;
	quadSkel[2] = 1.f;
	quadSkel[3] = 0.f;
	quadSkel[4] = 1.f;
	quadSkel[5] = 1.f;
	quadSkel[6] = 0.f;
	quadSkel[7] = 1.f;

	// ----------- Shaders --------------------------------------------

	setupRenderFboShader();
	setupGradQuadShader();
	colShader = scd->shaderCollector->getStdCol();
	stdTexShader = scd->shaderCollector->getStdTex();

	// ------------------------------------------------------

	for (auto j=0; j<2; j++) showCorners[j] = false;

	noModTex = glm::vec4(1.f, 1.f, 0.f, 0.f);
	identMat = glm::mat4(1.f);
}

//---------------------------------------------------------------

void FboDedistPersp::add(GWindow* _win, GLMCamera* _cam, float width,
		float height, float offsX, float offsY)
{
	scrData.push_back(new fboScreenData());
	scrData.back()->win = _win;
	scrData.back()->cam = _cam;

	for (auto i = 0; i < 4; i++)
		scrData.back()->blendAmt[i] = 0.f;

	for (auto j = 0; j < 2; j++)
		for (auto i = 0; i < 4; i++)
			scrData.back()->corners[j][i] = glm::vec2(quadSkel[i * 2],
					quadSkel[i * 2 + 1]);

	// berechnen der BlendPunkte in Screen-Koordinaten -> nur fuer internes GUI
	calcBlendPoints(scrData.end() - 1);
}

//---------------------------------------------------------------

// FBOView get always rendered to the full surface of a glfw context,
// for this reason all values have to be glfw context size relative
void FboDedistPersp::updtFboView(std::vector<fboView*>::iterator it)
{
	std::vector<fboScreenData*>::iterator thisFboView;
	std::vector<GWindow*>* wins = winMan->getWindows();

	// check if ctxId is valid
	if ((*it)->ctxId < int(wins->size()))
	{
		// check if fboView Index exists
		bool exists = false;
		for (std::vector<fboScreenData*>::iterator scrIt = scrData.begin(); scrIt != scrData.end(); ++scrIt)
			if ((*scrIt)->id == (*it)->id) {
				exists = true;
				thisFboView = scrIt;
			}

		// if not exists add new fboView
		if (!exists)
		{
			scrData.push_back(new fboScreenData());
			thisFboView = scrData.end() - 1;
			(*thisFboView)->id = (*it)->id;
			(*thisFboView)->glfwCtxId = (*it)->ctxId;
			(*thisFboView)->glfwCtxSize = glm::vec2(wins->at((*it)->ctxId)->getScreenWidth(),
													wins->at((*it)->ctxId)->getScreenHeight());

			(*thisFboView)->quad = new QuadArray(10, 10, -1.f, -1.f, 2.f, 2.f, 1.f, 1.f, 0.f, 1.f);

			uBlock.push_back( new UniformBlock(texShader->getProgram(), "quadData"));
			uBlock.back()->addVarName("perspMat", &(*thisFboView)->perspMat[0][0], GL_FLOAT_MAT4);
			uBlock.back()->addVarName("texSize", &(*thisFboView)->texSize[0], GL_FLOAT_VEC2);
			uBlock.back()->addVarName("texOffs", &(*thisFboView)->texOffs[0], GL_FLOAT_VEC2);
			uBlock.back()->addVarName("blendAmt", &(*thisFboView)->blendAmt[0], GL_FLOAT_VEC4);
		}

		// update values
		(*thisFboView)->win = wins->at((*it)->ctxId);
		(*thisFboView)->texSize.x = (*it)->texSize.x / fboWidthF;
		(*thisFboView)->texSize.y = (*it)->texSize.y / fboHeightF;

		(*thisFboView)->texOffs.x = (*it)->texOffs.x / fboWidthF;
		(*thisFboView)->texOffs.y = (*it)->texOffs.y / fboHeightF;

		(*thisFboView)->blendAmt[0] = std::max( (*it)->lowBlend / fboHeightF, 0.000001f); // avoid division by zero
		(*thisFboView)->blendAmt[1] = std::max( (*it)->rightBlend / fboWidthF, 0.000001f);
		(*thisFboView)->blendAmt[2] = std::max( (*it)->upBlend / fboHeightF, 0.000001f);
		(*thisFboView)->blendAmt[3] = std::max( (*it)->leftBlend / fboWidthF, 0.000001f);

		// "basic" quad ohne blending, hypothetische Eckpunkte,
		// kommen als pixel rein, normalisiere auf -1 | 1
		(*thisFboView)->corners[0][0] = glm::vec2((*it)->lowLeft.x, (*it)->lowLeft.y);
		(*thisFboView)->corners[0][1] = glm::vec2((*it)->lowRight.x, (*it)->lowRight.y);
		(*thisFboView)->corners[0][2] = glm::vec2((*it)->upRight.x, (*it)->upRight.y);
		(*thisFboView)->corners[0][3] = glm::vec2((*it)->upLeft.x, (*it)->upLeft.y);

		// kommen als pixel rein, normalisiere auf 0 | 1, relativ to glfwContext size
		for (unsigned int i = 0; i < 4; i++)
			(*thisFboView)->corners[0][i] = glm::vec2(
					(*thisFboView)->corners[0][i].x / (*thisFboView)->glfwCtxSize.x,
					(*thisFboView)->corners[0][i].y / (*thisFboView)->glfwCtxSize.y);

		calcMidAndDist(thisFboView);
		getPerspTrans(thisFboView); // berechne die eckpunkte der gradiationsquads und die transformationsmatritzen

		uBlock[thisFboView - scrData.begin()]->update();

	}
}

//---------------------------------------------------------

void FboDedistPersp::calcMidAndDist(std::vector<fboScreenData*>::iterator it)
{
	// berechne steigungen und c für alle seiten des dedist-quad
	for (unsigned short j=0; j<1; j++)
	{
		for (unsigned short i=0; i<4; i++)
		{
			unsigned short upInd = (i + 1) % 4;

			// corners
			// spezial fall, wenn die verbindungslinie zweier Eckpunkte genau
			// vertikal ist, funktioniert die y=mx+c formel nicht mehr
			if (((*it)->corners[j][upInd].x - (*it)->corners[j][i].x) != 0.f)
			{
				(*it)->m[j][i] = ((*it)->corners[j][upInd].y
						- (*it)->corners[j][i].y)
						/ ((*it)->corners[j][upInd].x - (*it)->corners[j][i].x);

				(*it)->c[j][i] = (*it)->corners[j][i].y
						- (*it)->m[j][i] * (*it)->corners[j][i].x;

				(*it)->fixX[j][i] = -1.f;
			}
			else
			{
				(*it)->fixX[j][i] = (*it)->corners[j][i].x;
				(*it)->m[j][i] = 1.f; // avoid division by 0
			}
		}
	}
}

//---------------------------------------------------------

void FboDedistPersp::getPerspTrans(std::vector<fboScreenData*>::iterator it)
{
	// get perspective transformation in pixels
	// upper left corner is origin

	cv::Mat pMat;
	vector<cv::Point2f> dstTexPoint;
	vector<cv::Point2f> dstQuadPoint;
	vector<cv::Point2f> dstGradPoint;
	glm::vec4 dirVec[4];

	quadPoint.clear();

	for (auto i=0; i<4; i++)
	{
		quadPoint.push_back(cv::Point2f(quadSkel[i * 2], quadSkel[i * 2 + 1]));
		//     std::cout << "quadPoint.back(): " << quadPoint.back() << std::endl;
	}

	for (auto i=0; i<4; i++)
	{
		dstTexPoint.push_back(cv::Point2f((*it)->corners[0][i].x, (*it)->corners[0][i].y));
		dstQuadPoint.push_back(cv::Point2f((*it)->corners[0][i].x, (*it)->corners[0][i].y));
		dstGradPoint.push_back(cv::Point2f((*it)->corners[0][i].x, (*it)->corners[0][i].y));
	}

	// get direction vector of the quad sides
	for (auto i=0; i<4; i++)
		dirVec[i] = glm::vec4(
				(*it)->corners[0][(i + 1) % 4].x - (*it)->corners[0][i].x,
				(*it)->corners[0][(i + 1) % 4].y - (*it)->corners[0][i].y, 0.f,
				0.f);

	// offset the texture points about the blendAmt in the direction of the side
	// maintain the perspective distortion of the quad
	for (auto i = 0; i < 4; i++)
	{
		dstTexPoint[i] -= cv::Point2f(dirVec[i].x, dirVec[i].y) * (*it)->blendAmt[(i + 3) % 4];
		dstTexPoint[i] += cv::Point2f(dirVec[(i + 3) % 4].x, dirVec[(i + 3) % 4].y) * (*it)->blendAmt[i];
	}

	pMat = cv::getPerspectiveTransform(dstTexPoint, quadPoint);
	(*it)->invPerspMat = cvMat33ToGlm(pMat);

	// get direction vector of the quad sides
	for (auto i=0; i<4; i++)
		dirVec[i] = glm::normalize(dirVec[i]);

	// offset the corner points about the blendAmt in the direction of the side
	for (auto i = 0; i < 4; i++)
		for (auto j = 0; j < 2; j++)
			dstQuadPoint[i] += cv::Point2f(dirVec[(i + j * 3) % 4].x,
					dirVec[(i + j * 3) % 4].y)
					* (*it)->blendCornerOffs[(i + (1 - j) * 3) % 4]
					* (j == 0 ? -1.f : 1.f);

	// save result for frustum adjustment
	for (auto i=0; i<4; i++)
		(*it)->corners[2][i] = glm::vec2(dstQuadPoint[i].x, dstQuadPoint[i].y);

	pMat = cv::getPerspectiveTransform(quadPoint, dstQuadPoint);
	(*it)->perspMat = cvMat33ToGlm(pMat);

	// calculate the transformation for the blending quads

	// bottom side
	dstGradPoint.clear();
	dstGradPoint.push_back(dstQuadPoint[0]);
	dstGradPoint.push_back(dstQuadPoint[1]);
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][1].x, (*it)->corners[0][1].y));
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][0].x, (*it)->corners[0][0].y));
	pMat = cv::getPerspectiveTransform(quadPoint, dstGradPoint);
	(*it)->gradQuad[0] = cvMat33ToGlm(pMat);

	// right side
	dstGradPoint.clear();
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][1].x, (*it)->corners[0][1].y));
	dstGradPoint.push_back(dstQuadPoint[1]);
	dstGradPoint.push_back(dstQuadPoint[2]);
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][2].x, (*it)->corners[0][2].y));
	pMat = cv::getPerspectiveTransform(quadPoint, dstGradPoint);
	(*it)->gradQuad[1] = cvMat33ToGlm(pMat);

	// top side
	dstGradPoint.clear();
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][3].x, (*it)->corners[0][3].y));
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][2].x, (*it)->corners[0][2].y));
	dstGradPoint.push_back(dstQuadPoint[2]);
	dstGradPoint.push_back(dstQuadPoint[3]);
	pMat = cv::getPerspectiveTransform(quadPoint, dstGradPoint);
	(*it)->gradQuad[2] = cvMat33ToGlm(pMat);

	// left side
	dstGradPoint.clear();
	dstGradPoint.push_back(dstQuadPoint[0]);
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][0].x, (*it)->corners[0][0].y));
	dstGradPoint.push_back(cv::Point2f((*it)->corners[0][3].x, (*it)->corners[0][3].y));
	dstGradPoint.push_back(dstQuadPoint[3]);
	pMat = cv::getPerspectiveTransform(quadPoint, dstGradPoint);
	(*it)->gradQuad[3] = cvMat33ToGlm(pMat);
}

//--------------------------------------------------------------------------------

void FboDedistPersp::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_C:
				showCorners[0] = !showCorners[0];
				if (showCorners[0])
					showCorners[1] = false;
				for (auto i = 0; i < static_cast<int>(scrData.size()); i++)
					for (auto j = 0; j < 4; j++)
						scrData[i]->cornersState[1][j] = false;
				break;
			case GLFW_KEY_B:
				showCorners[1] = !showCorners[1];
				if (showCorners[1])
					showCorners[0] = false;
				for (auto i = 0; i < static_cast<int>(scrData.size()); i++)
					for (auto j = 0; j < 4; j++)
						scrData[i]->cornersState[0][j] = false;
				break;
			case GLFW_KEY_T:
				// show test pic
				showTestPic = !showTestPic;
				for (auto i = 0; i < static_cast<int>(scrData.size()); i++)
					changeUBlock[i] = true;
				break;
			case GLFW_KEY_S:
				saveCalib();
				printf("save Settings\n");
				break;
			}
		}
	}
}

//--------------------------------------------------------------------------------

void FboDedistPersp::mouseBut(GLFWwindow* window, int button, int action, int mods)
{
	float ref[4] = { 0.f, 0.f, 0.f, 0.f };

	if (action == GLFW_PRESS)
	{
		for (auto j = 0; j < 2; j++)
		{
			if (showCorners[j])
			{
				for (vector<fboScreenData*>::iterator it = scrData.begin(); it != scrData.end(); ++it)
				{
					// check which context was clicked
					if ((*it)->win->getWin() == window)
					{
						float xPos = (mouseX - float((*it)->offs.x)) / (*it)->size.x;
						float yPos = (mouseY - float((*it)->offs.y)) / (*it)->size.y;

						// check if one of the corners was clicked
						for (auto i = 0; i < 4; i++)
						{
							ref[0] = (*it)->corners[j][i].x;
							ref[1] = (*it)->corners[j][i].x + (*it)->cornerSize.x * (quadSkel[i * 2] == 0.f ? 1.f : -1.f);
							ref[2] = 1.f - (*it)->corners[j][i].y;
							ref[3] = 1.f - (*it)->corners[j][i].y + (*it)->cornerSize.y * (quadSkel[i * 2 + 1] == 0.f ? -1.f : 1.f);

							if ((i == 0 || i == 3 ?
									xPos > ref[0] : xPos < ref[0])
									&& (i == 0 || i == 3 ? xPos < ref[1] : xPos > ref[1])
									&& (i < 2 ? yPos < ref[2] : yPos > ref[2])
									&& (i < 2 ? yPos > ref[3] : yPos < ref[3]))
							{
								(*it)->cornersState[j][i] = true;
//								if (j == 0)
//								{
								lastMouseDown.pos = (*it)->corners[j][i];
								lastMouseDown.it = it;
								lastMouseDown.cornerInd = i;
//								}
							}
						}
					}
				}
			}
		}
	}
	else if (action == GLFW_RELEASE)
	{
		for (vector<fboScreenData*>::iterator it = scrData.begin();
				it != scrData.end(); ++it)
		{
			for (auto j = 0; j < 2; j++)
			{
				for (auto i = 0; i < 4; i++)
					(*it)->cornersState[j][i] = false;

				// move BlendPos proportional
				if (j == 0 && lastMouseDown.cornerInd != -1)
				{
					/*
					 float vecProp = std::min(
					 1.f - std::sqrt(
					 std::pow((*lastMouseDown.it)->corners[1][lastMouseDown.cornerInd].x - lastMouseDown.pos.x, 2.f)
					 + std::pow((*lastMouseDown.it)->corners[1][lastMouseDown.cornerInd].y - lastMouseDown.pos.y, 2.f)),
					 1.f);
					 
					 (*lastMouseDown.it)->corners[1][lastMouseDown.cornerInd] =
					 ((*lastMouseDown.it)->corners[0][lastMouseDown.cornerInd]
					 - lastMouseDown.pos) * vecProp
					 + (*lastMouseDown.it)->corners[1][lastMouseDown.cornerInd];
					 */

					calcBlendPoints(lastMouseDown.it);

					lastMouseDown.cornerInd = -1;

					getPerspTrans(lastMouseDown.it);
					calcMidAndDist(lastMouseDown.it);
					newCornerVals[lastMouseDown.cornerInd] = true;
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------

void FboDedistPersp::mouseCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos - xOffs;
	mouseY = ypos;

	unsigned int ind = 0;
	for (vector<fboScreenData*>::iterator it = scrData.begin();
			it != scrData.end(); ++it)
	{
		bool modified = false;

		// check which context was clicked
		if ((*it)->win->getWin() == window)
		{
			for (auto j = 0; j < 2; j++)
			{
				for (auto i = 0; i < 4; i++)
				{
					// if the corner is active move it
					if ((*it)->cornersState[j][i])
					{
						float xPos = (mouseX - float((*it)->offs.x))
								/ (*it)->size.x;
						float yPos = 1.0
								- (mouseY - float((*it)->offs.y))
										/ (*it)->size.y;

						if (j == 1)
						{
							// moved the blend point
							(*it)->corners[j][i].x = xPos;
							(*it)->corners[j][i].y = yPos;

							// correct it´s position
							float distNewPointMidPoint = distPointLine(
									(*it)->corners[1][i], (*it)->corners[0][i],
									(*it)->corners[0][(i + 1) % 4]);

							//  std::cout << "distNewPointMidPoint: " << distNewPointMidPoint << std::endl;

							// calculate the direction vector of the actual side
							glm::vec2 sideDirVec = (*it)->corners[0][(i + 1) % 4] - (*it)->corners[0][i];
							//  std::cout << "sideDirVec: " << glm::to_string(sideDirVec) << std::endl;

							// rotate it about 90 degrees
							//glm::vec2 orthoSideDirVec = glm::vec2(glm::rotate( float(M_PI) * -0.5f, glm::vec3(0.f, 0.f, 1.f) )
							//                                      * glm::vec4(glm::normalize(sideDirVec), 0.f, 0.f));
							//  std::cout << "orthoSideDirVec: " << glm::to_string(orthoSideDirVec) << std::endl;

							// calculate midPoint of the corresponding side
							glm::vec2 midPointThisSide = sideDirVec * 0.5f
									+ (*it)->corners[0][i];
							// std::cout << "midPointThisSide: " << glm::to_string(midPointThisSide) << std::endl;

							// now calculate the size of the quad for setting the correct proportion of the
							// resulting blendAmt (offset)

							// calculate midPoint of the opposite side
							glm::vec2 midPointOpSide =
									((*it)->corners[0][(i + 3) % 4]
											- (*it)->corners[0][(i + 2) % 4])
											* 0.5f
											+ (*it)->corners[0][(i + 2) % 4];
							// std::cout << "midPointOpSide: " << glm::to_string(midPointOpSide) << std::endl;

							// calculate the distance of these two points
							float distMid = glm::distance(midPointOpSide,
									midPointThisSide);
							// std::cout << "distMid: " << distMid << std::endl;

							// set it into relation to the total width/height of the quad
							(*it)->blendCornerOffs[i] = distNewPointMidPoint;
							//  std::cout << " (*it)->blendCornerOffs[i]: " <<  (*it)->blendCornerOffs[i] << std::endl;

							// set the blendamt for scaling the camfrustrum
							// offset des blendpunktes von der seite, wenn das quad die groesse 1 | 1 haette
							(*it)->blendAmt[i] = distNewPointMidPoint / distMid;
							//  std::cout << " (*it)->blendAmt[i]: " <<  (*it)->blendAmt[i] << std::endl;

							// std::cout << std::endl;

						}
						else
						{
							(*it)->corners[j][i].x = xPos;
							(*it)->corners[j][i].y = yPos;
						}

						modified = true;
					}
				}

				// if blend point move, adjust the frustrum
				if (modified && j == 1)
					adjustFrustrum(it);
			}

			if (modified)
			{
				getPerspTrans(it);
				calcMidAndDist(it);
				newCornerVals[ind] = true;
			}
		}

		ind++;
	}
}

//---------------------------------------------------------

void FboDedistPersp::adjustFrustrum(std::vector<fboScreenData*>::iterator it)
{
	float* frustMult;
	frustMult = new float[4];

	// berechne die ursprüngliche Groesse des Frustrums
	float orgFrustWidth = (*it)->cam->getRight() - (*it)->cam->getLeft();
	float orgFrustHeight = (*it)->cam->getTop() - (*it)->cam->getBottom();

	// berechne die neue Hoehe und Breite fuer die blendAmts fuer jede Seite (da unterschiedlich)
	// (blendAmt muss in relation zur Breite und Hoehe des Justierungs-Quads stehen)
	float dstWidthLeft = orgFrustWidth * (1.f + (*it)->blendAmt[LEFT]);
	float dstWidthRight = orgFrustWidth * (1.f + (*it)->blendAmt[RIGHT]);
	float dstHeightTop = orgFrustHeight * (1.f + (*it)->blendAmt[TOP]);
	float dstHeightBottom = orgFrustHeight * (1.f + (*it)->blendAmt[BOTTOM]);

	// berechne die zielpositionen des neuen frustrums
	float destTop = (*it)->cam->getBottom() + dstHeightTop;
	float destBottom = (*it)->cam->getTop() - dstHeightBottom;
	float destRight = (*it)->cam->getLeft() + dstWidthRight;
	float destLeft = (*it)->cam->getRight() - dstWidthLeft;

	// berechne mit den Zielpositionen die notwendigen Offsets
	frustMult[BOTTOM] = destBottom - (*it)->cam->getBottom();
	frustMult[TOP] = destTop - (*it)->cam->getTop();
	frustMult[RIGHT] = destRight - (*it)->cam->getRight();
	frustMult[LEFT] = destLeft - (*it)->cam->getLeft();

	if (inited)
	{
		(*it)->cam->setFrustMult(frustMult);

		// call camera setup in CameraSet Class
		if (camSetPtr != NULL) (*camSetPtr)();
	}
}

//--------------------------------------------------------------------------------

void FboDedistPersp::calcBlendPoints(std::vector<fboScreenData*>::iterator it)
{
	glm::vec4 dirVec[4];

	// berechne die Positionen der Blendpunkte fuer das GUI
	for (auto i = 0; i < 4; i++)
	{
		// bestimme die Mittelpunkte der Verbindungslinien der Eckpunkte
		(*it)->corners[1][i] = glm::vec2(
				((*it)->corners[0][(i + 1) % 4].x + (*it)->corners[0][i].x)
						* 0.5f,
				((*it)->corners[0][(i + 1) % 4].y + (*it)->corners[0][i].y)
						* 0.5f);

		// korrigiere um die groesse der Punkte

		// bestimme den Richtungsvektor der Verbindungslinien und normalisiere sie
		dirVec[i] = glm::normalize(
				glm::vec4(
						(*it)->corners[0][(i + 1) % 4].x
								- (*it)->corners[0][i].x,
						(*it)->corners[0][(i + 1) % 4].y
								- (*it)->corners[0][i].y, 0.f, 0.f));

		// rotiere sie um 90 grad im uhrzeigersinn
		dirVec[i] = glm::rotate(float(M_PI) * -0.5f, glm::vec3(0.f, 0.f, 1.f))
				* dirVec[i];

		// verschiebe die Blendpunkte um den blendAmt
		(*it)->corners[1][i] += glm::vec2(dirVec[i])
				* (*it)->blendCornerOffs[i];
	}
}

//--------------------------------------------------------------------------------

void FboDedistPersp::bindFbo()
{
	destFbo->bind();
}

//--------------------------------------------------------------------------------

void FboDedistPersp::clearFbo()
{
	destFbo->clear();
}

//--------------------------------------------------------------------------------

void FboDedistPersp::clearFboAlpha(float alpha, float col)
{
	destFbo->clearAlpha(alpha, col);
}

//--------------------------------------------------------------------------------

void FboDedistPersp::clearDepthFbo()
{
	destFbo->clearDepth();
}

//---------------------------------------------------------------

void FboDedistPersp::drawFboView(std::vector<fboScreenData*>::iterator it)
{
	unsigned int ind = it - scrData.begin();

	// make the requested context current
	(*it)->win->makeCurrent();

	//std::cout << "FboDedistPersp::drawFboView viewport: 0.f, 0.f, " << float((*it)->win->getScreenWidth());
	//std::cout << ", " << float((*it)->win->getScreenHeight()) << std::endl;

	glViewportIndexedf(0, 0.f, 0.f, (*it)->glfwCtxSize.x, (*it)->glfwCtxSize.y);

	// render the fbo to the requested glfw context
	texShader->begin();
	texShader->setUniform1i("tex", 0);
	uBlock[it - scrData.begin()]->bind();

	glActiveTexture(GL_TEXTURE0);
	if (!showTestPic)
		glBindTexture(GL_TEXTURE_2D, destFbo->getColorImg());

	quadAr->draw();

	uBlock[ind]->unbind();
}

//---------------------------------------------------------

void FboDedistPersp::drawAllFboViews()
{
	for (std::vector<fboScreenData*>::iterator scrIt = scrData.begin();
		scrIt != scrData.end(); ++scrIt)
		drawFboView(scrIt);
}

//---------------------------------------------------------------

void FboDedistPersp::drawTestPic()
{
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	showTestPic = true;

	for (std::vector<fboScreenData*>::iterator scrIt = scrData.begin();
			scrIt != scrData.end(); ++scrIt)
	{
		changeUBlock[scrIt - scrData.begin()] = true;
		drawFboView(scrIt);
	}
}

//---------------------------------------------------------------

void FboDedistPersp::drawCorners(unsigned short _ind)
{
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
	{
		// draw corners
		glViewportIndexedf(0, scrData[_ind]->offs.x + xOffs,
				scrData[_ind]->offs.y, scrData[_ind]->size.x,
				scrData[_ind]->size.y);

		for (auto j = 0; j < 2; j++)
		{
			if (showCorners[j])
			{
				glDisable(GL_DEPTH_TEST);
				colShader->begin();

				for (auto i = 0; i < 4; i++)
				{
					mat = glm::translate(glm::mat4(1.f),
							glm::vec3(
									(scrData[_ind]->corners[j][i].x * 2.f - 1.f)
											+ scrData[_ind]->cornerSize.x
													* (quadSkel[i * 2] == 0 ?
															1.f : -1.f),
									(scrData[_ind]->corners[j][i].y * 2.f - 1.f)
											+ scrData[_ind]->cornerSize.y
													* (quadSkel[i * 2 + 1]
															== 0 ? 1.f : -1.f),
									1.f));
					mat = glm::scale(mat,
							glm::vec3(scrData[_ind]->cornerSize, 1.f));

					colShader->setUniformMatrix4fv("m_pvm", &mat[0][0]);
					cornerQuad[j]->draw();
				}

				glEnable(GL_DEPTH_TEST);
			}
		}
	}
}

//---------------------------------------------------------------

std::vector<FboDedistPersp::fboScreenData*>* FboDedistPersp::getFboScreenData()
{
	return &scrData;
}

//---------------------------------------------------------------

GLuint FboDedistPersp::getFbo()
{
	return destFbo->getFbo();
}

//---------------------------------------------------------------

GLuint FboDedistPersp::getFboTex(unsigned short _ind)
{
	return destFbo->getColorImg();
}

//---------------------------------------------------------------

glm::vec2 FboDedistPersp::getFboSize(unsigned short _ind)
{
	glm::vec2 out;
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
	{
		out.x = ctxFbos[scrData[_ind]->win]->getWidth();
		out.y = ctxFbos[scrData[_ind]->win]->getHeight();
	}
	return out;
}

//---------------------------------------------------------------

glm::vec2 FboDedistPersp::getScreenSize(unsigned short _ind)
{
	glm::vec2 out;
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
	{
		out = scrData[_ind]->size;
	}
	return out;
}

//---------------------------------------------------------------

glm::vec2 FboDedistPersp::getScreenOffs(unsigned short _ind)
{
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
		return scrData[_ind]->offs;

	return glm::vec2(0.f);
}

//---------------------------------------------------------------

float* FboDedistPersp::getGradQuadMatPtr(unsigned short _ind,
		cornerNames corner)
{
	if ((static_cast<short>(scrData.size()) - 1) >= _ind)
		return &scrData[_ind]->gradQuad[corner][0][0];

	return 0;
}

//---------------------------------------------------------------

void FboDedistPersp::setupRenderFboShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;
		layout( location = 1 ) in vec4 normal;
		layout( location = 2 ) in vec2 texCoord;
		layout( location = 3 ) in vec4 color;

		uniform quadData {
			mat4 perspMat;
			vec2 texSize;
			vec2 texOffs;
			vec4 blendAmt;
		};

		out vec2 tex_coord;
		out vec2 raw_texCoord;
		float x; float y; float w;

		vec2 undist(vec2 inPoint, mat4 distMat) {
			x = inPoint.x;
			y = inPoint.y;
			w = 1.0/(x * distMat[0][2] + y * distMat[1][2] + distMat[2][2]);
			inPoint.x = (x * distMat[0][0] + y * distMat[1][0] + distMat[2][0]) *w;
			inPoint.y = (x * distMat[0][1] + y * distMat[1][1] + distMat[2][1]) *w;
			return inPoint;
		}

		void main() {
			// interpolate corner coordinates
			raw_texCoord = texCoord;
			tex_coord = texCoord * texSize + texOffs;
			gl_Position = vec4(undist(position.xy *0.5 +0.5, perspMat) *2.0 -1.0, 0, 1);
		});

	vert = "// FboDedistPersp, vert\n" + shdr_Header + vert;

	// zero is left top
	std::string frag = STRINGIFY(
		uniform sampler2D tex;
		uniform quadData {
			mat4 perspMat;
			vec2 texSize;
			vec2 texOffs;
			vec4 blendAmt;
		};

		in vec2 tex_coord;
		in vec2 raw_texCoord;

		layout (location = 0) out vec4 color;

		void main() {
			float blend = min(raw_texCoord.y, blendAmt.x) / blendAmt.x; // bottom
			blend *= min(1.0 - raw_texCoord.x, blendAmt.y) / blendAmt.y;// right
			blend *= min(1.0 - raw_texCoord.y, blendAmt.z) / blendAmt.z;// top
			blend *= min(raw_texCoord.x, blendAmt.w) / blendAmt.w;// left

			color = texture(tex, tex_coord);
			color.a = blend;
		});

	frag = "// FboDedistPersp shader, frag\n" + shdr_Header + frag;

	texShader = shCol->addCheckShaderText("FboDedistPersp", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void FboDedistPersp::setupGradQuadShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color;

					uniform mat4 perspMat; mat4 m_pvm = mat4(1.0);

					out vec2 tex_coord; out vec4 col; float x; float y; float w;

					vec2 undist(vec2 inPoint, mat4 distMat) { x = inPoint.x; y = inPoint.y; w = 1.0/(x * distMat[0][2] + y * distMat[1][2] + distMat[2][2]); inPoint.x = (x * distMat[0][0] + y * distMat[1][0] + distMat[2][0]) *w; inPoint.y = (x * distMat[0][1] + y * distMat[1][1] + distMat[2][1]) *w; return inPoint; }

					void main() {
					// interpolate corner coordinates
					col = color; tex_coord = texCoord; gl_Position = m_pvm * vec4(undist(position.xy *0.5 +0.5, perspMat) *2.0 -1.0, 0, 1); });

	vert = "// FboDedistPersp GradQuad Shader, vert\n" + shdr_Header + vert;

	// zero is left top
	std::string frag =
			STRINGIFY(uniform sampler2D tex;

			in vec2 tex_coord; in vec4 col;

			uniform int side;

			layout (location = 0) out vec4 color;

			void main() {
			// color = vec4(1.0, 0.0, 0.0, 1.0);
					color = vec4(0.0, 0.0, 0.0, 1.0 - tex_coord.y) * float(side == 0) + vec4(0.0, 0.0, 0.0, tex_coord.x) * float(side == 1) + vec4(0.0, 0.0, 0.0, tex_coord.y) * float(side == 2) + vec4(0.0, 0.0, 0.0, 1.0 - tex_coord.x) * float(side == 3);
//                      color = texture(tex, vec2(tex_coord));
					});

	frag = "// FboDedistPersp GradQuad Shader shader, frag\n" + shdr_Header
			+ frag;

	gradQuadShdr = shCol->addCheckShaderText("FboDedistPerspGradQuad",
			vert.c_str(), frag.c_str());
}

//---------------------------------------------------------

glm::mat4 FboDedistPersp::cvMat33ToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j = 0; j < 3; j++)
		for (short i = 0; i < 3; i++)
			out[i][j] = _mat.at<double>(j * 3 + i);

	return out;
}

//--------------------------------------------------------------------------------

void FboDedistPersp::loadCalib()
{
#ifdef HAVE_OPENCV

	printf("loading calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		for (auto j = 0; j < static_cast<int>(scrData.size()); j++)
		{
			for (auto k = 0; k < 2; k++)
			{
				for (auto i = 0; i < 4; i++)
				{
					fs["corner" + std::to_string(j) + "_" + std::to_string(k)
							+ "_" + std::to_string(i) + "x"]
							>> scrData[j]->corners[k][i].x;
					fs["corner" + std::to_string(j) + "_" + std::to_string(k)
							+ "_" + std::to_string(i) + "y"]
							>> scrData[j]->corners[k][i].y;
				}
			}

			for (auto i = 0; i < 4; i++)
			{
				fs["blendamt" + std::to_string(j) + "_" + std::to_string(i)]
						>> scrData[j]->blendAmt[i];
				fs["blendcorneroffs" + std::to_string(j) + "_"
						+ std::to_string(i)] >> scrData[j]->blendCornerOffs[i];
			}
		}
	}

	// init matrices
	for (vector<fboScreenData*>::iterator it = scrData.begin();
			it != scrData.end(); ++it)
	{
		getPerspTrans(it);
		calcMidAndDist(it);
		adjustFrustrum(it);
	}
#endif
}

//--------------------------------------------------------------------------------

void FboDedistPersp::saveCalib()
{
#ifdef HAVE_OPENCV
	printf("saving calibration \n");

	cv::FileStorage fs(fileName, cv::FileStorage::WRITE);

	if (fs.isOpened())
	{
		for (int j = 0; j < static_cast<int>(scrData.size()); j++)
		{
			for (auto k = 0; k < 2; k++)
			{
				for (int i = 0; i < 4; i++)
				{
					fs
							<< "corner" + std::to_string(j) + "_"
									+ std::to_string(k) + "_"
									+ std::to_string(i) + "x"
							<< scrData[j]->corners[k][i].x;
					fs
							<< "corner" + std::to_string(j) + "_"
									+ std::to_string(k) + "_"
									+ std::to_string(i) + "y"
							<< scrData[j]->corners[k][i].y;
				}
			}

			for (int i = 0; i < 4; i++)
			{
				fs << "blendamt" + std::to_string(j) + "_" + std::to_string(i)
						<< scrData[j]->blendAmt[i];
				fs
						<< "blendcorneroffs" + std::to_string(j) + "_"
								+ std::to_string(i)
						<< scrData[j]->blendCornerOffs[i];
			}
		}
	}
#endif
}

//---------------------------------------------------------------

FboDedistPersp::~FboDedistPersp()
{
	delete cornerQuad;
}
}
