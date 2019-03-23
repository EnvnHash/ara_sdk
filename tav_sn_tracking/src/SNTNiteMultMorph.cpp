/*
 * SNTNiteMultMorph.cpp
 *
 *  Created on: 13.03.2016
 *      Copyright by Sven Hahne
 */

#include "SNTNiteMultMorph.h"

namespace tav
{

SNTNiteMultMorph::SNTNiteMultMorph(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
											SceneNode(_scd, _sceneArgs),  maxNrUsers(10),
											posSameThres(600.f), velSameThres(20.f), timeToDeact(1.0),
											objSize(1.f),
											timeToDeactSingle(1.0),
											onTimeToAdd(0.1), 	// time a new user has to exist until it is added to the single users
											fadeInTime(3.0),	// time to fade in the phone
											fadeOutTime(4.0)	// time to fade out the phone
{
    kin = static_cast<KinectInput*>(scd->kin);
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	mapConf = static_cast<SNFreenect2Motion::mapConfig*>(scd->fnMapConf);

	nrDevices = kin->getNrDevices();
	nisFrameNr = new int[nrDevices];
	nis = new NISkeleton*[nrDevices];
	userDat = new userData*[nrDevices];

	testFade = new AnimVal<float>(RAMP_LIN_UP, nullptr);

	objTex = new TextureManager();
	objTex->loadTexture2D((*scd->dataPath)+"/textures/samsung/g7.png");

	objTexInv = new TextureManager();
	objTexInv->loadTexture2D((*scd->dataPath)+"/textures/samsung/g7.png");

	objTexSombra = new TextureManager();
	objTexSombra->loadTexture2D((*scd->dataPath)+"/textures/samsung/g7_sombra.png");

	for (int i=0; i<nrDevices; i++)
	{
		nis[i] = kin->getNis(i);
		nisFrameNr[i] = -1;
		userDat[i] = new userData[maxNrUsers];

		for (int j=0; j<maxNrUsers; j++)
		{
			userDat[i][j].kFilt = new CvKalmanFilter(4, 2, 0.00001f);
			//			userDat[i][j].transMat = glm::mat4(1.f);
		}
	}

	quadAr = new QuadArray(40, 40, -1.f, -1.f,
			1.f,
			//			float(scd->screenWidth) / float(scd->screenHeight) * (objTex->getHeightF() / objTex->getWidthF()),
			1.f,
			1.f, 1.f, 1.f, 1.f);
}

//----------------------------------------------------

void SNTNiteMultMorph::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(!inited)
	{
		initShdr(cp);
		noiseTex = new Noise3DTexGen(shCol,
				true, 8,
				256, 256, 64,
				4.f, 4.f, 16.f);

		// normalize room dimensions
		// alle werte werden in den norm cubus skaliert
		normRoomDim.x = scd->roomDim->x;     // referenz = 2.f, x wert
		normRoomDim.y = scd->roomDim->y;    // y groesse
		normRoomDim.z = scd->roomDim->z;      // z groesse

		// skaliere tunnel dimensionen auf normkubus
		// bestimme den skalierungsfaktor
		roomScaleF = 2.f / normRoomDim.x;

		// skaliere
		normRoomDim.y = normRoomDim.y * roomScaleF;	// 0,628
		normRoomDim.z = normRoomDim.z* roomScaleF;	// 0,628
		normRoomDim.x = 2.f;

		testFade->start(0.f, 1.f, 6.0, time, true );
		inited = true;
	}

	glDisable(GL_CULL_FACE);


	explShdr->begin();
	explShdr->setUniform1i("tex", 0);
	explShdr->setUniform1i("tex2", 2);
	explShdr->setUniform1i("perlNoise", 1);
	explShdr->setUniform1f("texFade", osc->alpha);

	//        explShdr->setUniform1f("modu", explSlid->getVal());

	explShdr->setUniformMatrix4fv("projection_matrix_g", cp->multicam_projection_matrix, cp->nrCams);
	explShdr->setUniformMatrix4fv("view_matrix_g", cp->multicam_view_matrix, cp->nrCams);

	glActiveTexture(GL_TEXTURE0);
	objTex->bind(0);

	glActiveTexture(GL_TEXTURE2);
	objTexSombra->bind(2);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	//explShdr->setUniform1f("noiseOffsX", 0.14f );


	for (short i=0;i<nrDevices;i++)
	{
		for (short j=0;j<maxNrUsers;j++ )
		{
			if (userDat[i][j].active && userDat[i][j].onTime > onTimeToAdd)
			{
				explShdr->setUniform1f("moduAmt",
						float(1.0 - std::fmin( userDat[i][j].onTime - onTimeToAdd, fadeInTime ) / fadeInTime)
						+ userDat[i][j].fadeOut->getVal()
				);

				explShdr->setUniformMatrix4fv("model_matrix", &userDat[i][j].transMat[0][0]);
				explShdr->setUniform3f("noiseOffs", 0.1f, 0.2f, 0.2f);
				//				explShdr->setUniform3f("noiseOffs", userDat[i][j].noiseOffs.x,
				//						userDat[i][j].noiseOffs.y, userDat[i][j].noiseOffs.z);
				quadAr->draw();

				glActiveTexture(GL_TEXTURE0);
				objTexInv->bind(0);


				glm::mat4 mirrorMat = glm::translate( userDat[i][j].transMat, glm::vec3(0.f, 0.f, 2.f) );
				explShdr->setUniformMatrix4fv("model_matrix", &mirrorMat[0][0]);
				quadAr->draw();

				if( userDat[i][j].isFading && userDat[i][j].fadeOut->stopped() )
				{
				//	std::cout <<  "stopped fading " << std::endl;

					userDat[i][j].active = false;
					userDat[i][j].isFading = false;
				}
			}
		}
	}

	/*
	glm::mat4 mirrorMat;
	for(std::vector<singleData*>::iterator it = singleUserDat.begin(); it != singleUserDat.end(); it++)
	{
		explShdr->setUniform1f("moduAmt",
				(*it)->active ?
						float(1.0 - std::fmin( (*it)->onTime, fadeInTime ) / fadeInTime) :
						(*it)->fadeOut->getVal()
		);

		glm::mat4 posMatr = glm::mat4(1.f);
		posMatr = glm::scale( glm::vec3(0.3f, 0.8f, 1.f) );

		//std::cout <<  testFade->getVal() << std::endl;

//		explShdr->setUniform1f("moduAmt", 0.f );
//		explShdr->setUniform1f("moduAmt", testFade->getVal() );
//		explShdr->setUniformMatrix4fv("model_matrix", &posMatr[0][0]);

		explShdr->setUniformMatrix4fv("model_matrix", &(*it)->transMat[0][0]);
		explShdr->setUniform3f("noiseOffs", 0.1f, 0.2f, 0.2f);
		quadAr->draw();

		//			explShdr->setUniformMatrix4fv("model_matrix", &(*it)->transMat2[0][0]);
		//			quadAr->draw();
	}
	 */
}

//----------------------------------------------------

void SNTNiteMultMorph::update(double time, double dt)
{
    //testFade->update(time);

	std::chrono::duration<double> diff;
	float diffPos;
	float diffVel;

	if (kin->isReady())
	{
		for (short i=0;i<nrDevices;i++)
		{
			if(kin->isDeviceReady(i) && nisFrameNr[i] != nis[i]->getFrameNr())
			{
				// check for new Users and update values
				const nite::Array<nite::UserData>& users = nis[i]->getUserTrackerFrame()->getUsers();

				for (short j=0;j<users.getSize();j++ )
				{
					glm::vec4 newPos;

					newPos.x = users[j].getCenterOfMass().x;
					newPos.y = users[j].getCenterOfMass().y;
					newPos.z = users[j].getCenterOfMass().z;
					newPos.w = 1.f;


					// apply rotation matrices
					applyMapping(&newPos, i);

					//std::cout << glm::to_string(newPos) << std::endl;

					if(newPos.z < 2100.f && newPos.z > -2500.f)
					{
						if(!userDat[i][j].active)
						{
							userDat[i][j].active = true;
							userDat[i][j].startTime = std::chrono::high_resolution_clock::now();
							userDat[i][j].pos = newPos;

							// set initPos for kalman filter
							userDat[i][j].kFilt->initPos( userDat[i][j].pos.y, userDat[i][j].pos.z);
							userDat[i][j].fadeOut = new AnimVal<float>(tav::RAMP_LIN_UP, nullptr);
							userDat[i][j].fadeOut->setInitVal(0.f);
							userDat[i][j].noiseOffs = glm::vec3( getRandF(0.f, 1.f), getRandF(0.f, 1.f), getRandF(0.f, 1.f));

						} else
						{
							diff = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - userDat[i][j].lastUpdt);

							// calculate velocity, with limit
							//	if(diff.count() < 0.08 && glm::length(userDat[i][j].pos - userDat[i][j].lastPos) < 300.f)
							userDat[i][j].vel = userDat[i][j].pos - userDat[i][j].lastPos;

							// check active time
							diff = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - userDat[i][j].startTime);
							userDat[i][j].onTime = diff.count();

							//std::cout << "got user [" << i << "][" << j << "] onTIme : " << userDat[i][j].onTime << std::endl;

							// save last Updt Time
							userDat[i][j].lastUpdt = std::chrono::high_resolution_clock::now();
							userDat[i][j].pos = newPos;
							userDat[i][j].kFilt->update( userDat[i][j].pos.y, userDat[i][j].pos.z);
						}

						//std::cout <<  glm::length(userDat[i][j].pos - userDat[i][j].lastPos) << std::endl;

						// if user "jumps" put him inactive
						//					if( glm::length(userDat[i][j].pos - userDat[i][j].lastPos) > 100.f)
						//					{
						//						userDat[i][j].active = false;
						//						userDat[i][j].fadeOut->start(0.f, 1.f, fadeOutTime, time, false);
						//					}

						// reset duplicate flag
						userDat[i][j].isDupl = false;
						userDat[i][j].duplCamId = -1;
						userDat[i][j].duplUserId = -1;
						userDat[i][j].isFading = false;

						// save lastPos
						userDat[i][j].lastPos = userDat[i][j].pos;

						// kalman predict
						userDat[i][j].kFilt->predict();

					} else
					{
						if(userDat[i][j].active && !userDat[i][j].isFading)
						{
							userDat[i][j].isFading = true;
							userDat[i][j].fadeOut->start(0.f, 1.f, fadeOutTime, time, false);
							//std::cout <<  "start fade out " << std::endl;
						}

						if(userDat[i][j].isFading)
						{
							if( glm::length(newPos -userDat[i][j].pos ) < 100.f  )
							{
								userDat[i][j].pos = newPos;
								userDat[i][j].kFilt->update( userDat[i][j].pos.y, userDat[i][j].pos.z);
							}
							userDat[i][j].fadeOut->update(time);
							//std::cout <<  "userDat[i][j].fadeOut "<< userDat[i][j].fadeOut->getVal() << std::endl;
						}
					}
				}
				nisFrameNr[i] = nis[i]->getFrameNr();
			}
		}

		/*
		// detect "dead" users
		for (short i=0;i<nrDevices;i++)
		{
			for (short j=0;j<maxNrUsers;j++ )
			{
				// check the time difference to the last update
				diff = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - userDat[i][j].lastUpdt);

				if(diff.count() > timeToDeact)
				{
					if(userDat[i][j].active)
					{
						userDat[i][j].isFading = true;
						userDat[i][j].fadeOut->start(0.f, 1.f, fadeOutTime, time, false);
					}
				//
				//					userDat[i][j].active = false;
				}
			}
		}
*/
		// update kalman
		for (short i=0;i<nrDevices;i++)
		{
			for (short j=0;j<maxNrUsers;j++ )
			{
				userDat[i][j].kFilt->predict();
				userDat[i][j].kFilt->update( userDat[i][j].pos.y, userDat[i][j].pos.z);

				if (userDat[i][j].active)
				{
					if(userDat[i][j].fadeOut && userDat[i][j].isFading)
						userDat[i][j].fadeOut->update(time);

					//	printf("updating matrix %d %d %f\n", i, j, userDat[i][j].pos.z);

					userDat[i][j].transMat = userDat[i][j].transMat =
							glm::translate(glm::mat4(1.f),
									glm::vec3(
											userDat[i][j].kFilt->get(1) * -roomScaleF * 3.f,
											//userDat[i][j].pos.z * -roomScaleF * 3.f,
											0.f,
											-1.f) );

					userDat[i][j].transMat =
							glm::scale(
									userDat[i][j].transMat,
									glm::vec3(float(scd->screenHeight) / float(scd->screenWidth) * 1.f
											* (objTex->getWidthF() / objTex->getHeightF())
											* objSize,
											objSize, 1.f) );
				}
			}
		}

		// detect duplicates, quick and dirty, check everything
		for (short i=0;i<nrDevices;i++)
		{
			for (short j=0;j<maxNrUsers;j++ )
			{
				if (userDat[i][j].active)
				{
					std::map<float, userData*> similarUsers;

					// calculate the difference with all other users of the other cameras
					for (short k=(i+1);k<nrDevices;k++)
					{
						for (short l=0;l<maxNrUsers;l++ )
						{
							if (userDat[k][l].active)
							{
								diffPos = glm::length( userDat[i][j].pos - userDat[k][l].pos );
								diffVel = glm::length( userDat[i][j].vel - userDat[k][l].vel );

								// create a map and save all similar users
								if(diffPos < posSameThres && diffVel < velSameThres)
								{
									similarUsers[diffPos] = &userDat[k][l];
									similarUsers[diffPos]->assignedCamId = k;
									similarUsers[diffPos]->assignedUserId = l;
								}
								//	std::cout << "user["<< i << "][" << j << "] with user["<< k << "][" << l << "]  diffPos " << diffPos <<  " diffVel: " << diffVel << std::endl;
							}
						}
					}

					// if there are similar users now take the nearest one and add it to the actual one
					if( static_cast<short>(similarUsers.size()) > 0)
					{
						userData* dupUser = (*similarUsers.begin()).second;

						userDat[i][j].duplCamId = dupUser->assignedCamId;
						userDat[i][j].duplUserId = dupUser->assignedUserId;

						dupUser->isDupl = true;
					}
				}
			}
		}

		int nrRealUsers=0;
		// count real users
		for (short i=0;i<nrDevices;i++)
			for (short j=0;j<maxNrUsers;j++ )
				if (userDat[i][j].active && !userDat[i][j].isDupl && userDat[i][j].onTime > onTimeToAdd)
					nrRealUsers++;

		//std::cout << "real users " << nrRealUsers  << std::endl;

		/*
		// now everything is clean, loop through all user only take the real ones
		for (short i=0;i<nrDevices;i++)
		{
			for (short j=0;j<maxNrUsers;j++ )
			{
				if (userDat[i][j].active && !userDat[i][j].isDupl && userDat[i][j].onTime > onTimeToAdd)
				{
					std::cout << "found real user["<< i << "][" << j << "] with: ";
					std::cout << glm::to_string(userDat[i][j].pos) << std::endl;
					//					std::cout << "kalman: " << userDat[i][j].kFilt->get(0) << ", " << userDat[i][j].kFilt->get(1) << std::endl;

					bool found = false;

					// wenn nicht gefunden schau ob es einen eintrag gibt, der nahe dran ist.
					// wenn umspringen von einer cam auf die andere
					if(!found)
					{
						std::map<float, singleData*> similarUsers;

						for(std::vector<singleData*>::iterator it = singleUserDat.begin(); it != singleUserDat.end(); it++)
						{
							diffPos = glm::length( (*it)->pos - userDat[i][j].pos );

							// create a map and save all similar users
							if(diffPos < posSameThres )
								similarUsers[diffPos] = (*it);
						}

						// if there are similar users now take the nearest one and update the actual one
						if( static_cast<short>(similarUsers.size()) > 0)
						{
							singleData* dupUser = (*similarUsers.begin()).second;

							dupUser->id[i] = j;
							dupUser->pos = glm::vec4(0.f, userDat[i][j].kFilt->get(0), userDat[i][j].kFilt->get(1), 1.f);
							dupUser->lastUpdt = std::chrono::high_resolution_clock::now();

							found = true;
						}
					}


					if (found == false && singleUserDat.size() <= nrRealUsers)
					{
						// wenn immer noch nichts gefunden mach einen neuen eintrag
						singleUserDat.push_back(new singleData());
						singleUserDat.back()->active = true;
						singleUserDat.back()->kFilt = new CvKalmanFilter(4, 2, 0.00001f);
						singleUserDat.back()->kFilt->initPos(userDat[i][j].pos.y, userDat[i][j].pos.z);
						singleUserDat.back()->fadeOut = new AnimVal<float>(tav::RAMP_LIN_UP, nullptr);
						singleUserDat.back()->noiseOffs = glm::vec3( getRandF(0.f, 1.f), getRandF(0.f, 1.f), getRandF(0.f, 1.f));

						singleUserDat.back()->pos = userDat[i][j].pos;
						singleUserDat.back()->vel = userDat[i][j].vel;
						singleUserDat.back()->id = new short[nrDevices];
						singleUserDat.back()->id[i] = j;
						singleUserDat.back()->startTime = std::chrono::high_resolution_clock::now();
						singleUserDat.back()->lastUpdt = std::chrono::high_resolution_clock::now();
						singleUserDat.back()->transMat = singleUserDat.back()->transMat =
								glm::translate(glm::mat4(1.f),
										glm::vec3(userDat[i][j].pos.z * roomScaleF,
												0.f,
												normRoomDim.z * -0.5f * roomScaleF) );

						singleUserDat.back()->transMat =
								glm::scale(
										singleUserDat.back()->transMat,
										glm::vec3(float(scd->screenHeight) / float(scd->screenWidth)
		 * (objTex->getWidthF() / objTex->getHeightF())
		 * objSize,
												objSize, 1.f) );
					}
				}
			}
		}
		 */
	}

	/*
	//std::cout << "found single  users "<< singleUserDat.size() << std::endl;

	// check for fadeOut entries
	short ind=0;

	for(std::vector<singleData*>::iterator it = singleUserDat.begin(); it != singleUserDat.end(); it++)
	{
		// check the time difference to the last update
		diff = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - (*it)->lastUpdt);

		if(diff.count() > timeToDeactSingle && (*it)->active == true)
		{
			(*it)->active = false;
			(*it)->fadeOut->start(0.f, 1.f, fadeOutTime, time, false);
		}

		ind++;
	}


	// update fadeOuts
	for(std::vector<singleData*>::iterator it = singleUserDat.begin(); it != singleUserDat.end(); it++)
		if(!(*it)->active)
			(*it)->fadeOut->update(time);


	// check for dead entries
	std::vector<short> toKill;
	ind=0;
	for(std::vector<singleData*>::iterator it = singleUserDat.begin(); it != singleUserDat.end(); it++)
	{
		if((*it)->active == false)
		{
			//std::cout << "user ind " << ind << " fadePerc " << (*it)->fadeOut->getPercentage() << std::endl;

			if( (*it)->fadeOut->getPercentage() >= 1.f)
				toKill.push_back(ind);
		}

		ind++;
	}


	// kill dead users
	for(std::vector<short>::iterator it = toKill.begin(); it != toKill.end(); it++)
	{
		singleUserDat[(*it)] = singleUserDat.back();
		singleUserDat.pop_back();
	}

	std::cout << "real  single  users "<< singleUserDat.size() << std::endl;


	// update active users
	// update matrices
	for(std::vector<singleData*>::iterator it = singleUserDat.begin(); it != singleUserDat.end(); it++)
	{
		if( (*it)->active )
		{
			(*it)->kFilt->update((*it)->pos.y, (*it)->pos.z);
			(*it)->kFilt->predict();

			//std::cout << glm::to_string(userDat[i][j].transMat) << std::endl;

			//std::cout << "real single User: " << (*it)->onTime << " : " ;
			//std::cout << (*it)->kFilt->get(0) << ", ";
			//std::cout << (*it)->kFilt->get(1);
			//		std::cout << userDat[i][j].kFilt->getPrediction(2) << " vel: ";
			//		std::cout << glm::length(userDat[i][j].vel) << std::endl;
			//std:cout << std::endl;

			diff = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - (*it)->startTime);
			(*it)->onTime = diff.count();

			(*it)->transMat = glm::translate(glm::mat4(1.f),
					glm::vec3((*it)->kFilt->get(1) * -roomScaleF * 2.5f,
							//(*it)->kFilt->get(0) * roomScaleF,
							0.f,
							-1.f) );

			(*it)->transMat = glm::scale(
					(*it)->transMat,
					glm::vec3(float(scd->screenHeight) / float(scd->screenWidth) * 8.f
	 * (objTex->getWidthF() / objTex->getHeightF())
	 * objSize,
							objSize, 1.f) );


			(*it)->transMat2 = glm::translate(glm::mat4(1.f),
					glm::vec3((*it)->kFilt->get(1) * -roomScaleF * 2.5f,
							//(*it)->kFilt->get(0) * roomScaleF,
							0.f,
							4.f) );

			(*it)->transMat2 = glm::scale(
					(*it)->transMat,
					glm::vec3(float(scd->screenHeight) / float(scd->screenWidth) * 8.f
	 * (objTex->getWidthF() / objTex->getHeightF())
	 * objSize,
							objSize, 1.f) );
		}
	}
	 */

	//std::cout << std::endl;
}

//----------------------------------------------------

void SNTNiteMultMorph::applyMapping(glm::vec4* pos, unsigned short camNr)
{
	glm::mat4 rotMat;

	// flip vertical
	pos->y *= -1.f;

	rotMat = glm::mat4(1.f);

    
	rotMat = glm::translate(rotMat,
			mapConf->trans[ mapConf->xtionAssignMap[camNr] ] );

	rotMat = glm::scale(rotMat, mapConf->scale[ mapConf->xtionAssignMap[camNr] ]);

	rotMat = glm::rotate(rotMat,
			mapConf->rotAngleY[ mapConf->xtionAssignMap[camNr] ],
			glm::vec3(0.f, 1.f, 0.f));

	rotMat = glm::rotate(rotMat,
			mapConf->rotAngleX[ mapConf->xtionAssignMap[camNr] ],
			glm::vec3(1.f, 0.f, 0.f));

    (*pos) = rotMat * (*pos);
}

//----------------------------------------------------

void SNTNiteMultMorph::initShdr(camPar* cp)
{
	std::string nrCams = std::to_string(cp->nrCams);

	//- Position Shader ---

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout (location=0) in vec4 position;\n
	layout (location=1) in vec3 normal;\n
	layout (location=2) in vec2 texCoord;\n
	layout (location=3) in vec4 color;\n
	out vec2 tex_coord;\n
	void main() {\n
		tex_coord = texCoord;\n
		gl_Position = position;\n
	});
	vert = "// SNTNiteMultMorph pos tex vertex shader\n" +shdr_Header +vert;



	shdr_Header = "#version 410 core\n#pragma optimize(on)\n layout(triangles, invocations="+nrCams+") in;\n layout(triangle_strip, max_vertices=3) out;\n uniform mat4 view_matrix_g["+nrCams+"];\n uniform mat4 projection_matrix_g["+nrCams+"];\n";

	std::string geom = STRINGIFY(in vec2 tex_coord[];\n
	out vec2 gs_texCo;\n
	out vec4 gs_Col;\n

	uniform mat4 model_matrix;\n
	uniform sampler3D perlNoise;\n
	uniform float moduAmt;\n
	uniform float noiseOffsX;\n
	uniform vec3 noiseOffs; \n
	vec3 noiseModVec;\n
	vec2 normTexCoord;\n
	float rad;
	float alphaDeg;

	void main()
	{
		gl_ViewportIndex = gl_InvocationID;\n

		noiseModVec = texture(perlNoise,
				vec3(tex_coord[0] + float(gl_PrimitiveIDIn) * 0.01, moduAmt * 0.01)
				+ noiseOffs).xyz;

		noiseModVec -= vec3(0.14, 0.14, 0.0);
		noiseModVec *= vec3(40.0, 10.0, -1.0);

		normTexCoord = gl_in[0].gl_Position.xy;

		alphaDeg = atan( normTexCoord.y, normTexCoord.x );
		alphaDeg = min(max(alphaDeg, -4.0), 4.0);
		rad = moduAmt * 4.0;
		vec2 rev = vec2( cos(alphaDeg) * rad, sin(alphaDeg) * rad);

		for (int i=0; i<gl_in.length(); i++)
		{
			gs_texCo = tex_coord[i];\n
			gs_Col = vec4(1.0 - moduAmt);\n


			gl_Position = projection_matrix_g[gl_InvocationID] * \n
			view_matrix_g[gl_InvocationID] * \n
			model_matrix
			* \n
			(
					(moduAmt * vec4(rev, 0.0, 0.0))
					+ (vec4(noiseModVec, 0.0) * moduAmt * 0.3)
					+ gl_in[i].gl_Position
			);

			EmitVertex();
		}
		EndPrimitive();
	});

	geom = "// SNTNiteMultMorph pos tex geom shader\n" +shdr_Header +geom;


	shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
	in vec2 gs_texCo;\n
	in vec4 gs_Col;\n
	uniform sampler2D tex;\n
	uniform sampler2D tex2;\n
	vec4 col1;
	vec4 col2;
	uniform float texFade;\n

	void main()\n
	{\n
		col1 = texture(tex, gs_texCo);\n
		col1.a *= gs_Col.a;
		col2 = texture(tex2, gs_texCo);\n
		col2.a *= gs_Col.a;
		//fragColor = vec4(0.4, 0.4, 0.4, fragColor.a);\n
		fragColor = mix( col1, col2, texFade);
	});
	frag = "// SNTNiteMultMorph pos tex shader\n"+shdr_Header+frag;

	explShdr = shCol->addCheckShaderText("SNTNiteMultMorphShader",
			vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

SNTNiteMultMorph::~SNTNiteMultMorph()
{
}

}
