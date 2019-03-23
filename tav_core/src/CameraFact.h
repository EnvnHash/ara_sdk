//
//  SceneNodeFact.h
//  tav_gl4
//
//  Created by Sven Hahne on 15.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <string>

#include "CameraSets/CameraSet.h"
#include "CameraSets/Cs1FboAspectQuad.h"
#include "CameraSets/Cs1Perspective.h"
#include "CameraSets/Cs1PerspBg.h"
#include "CameraSets/Cs1Perspective2.h"
#include "CameraSets/Cs2HeadHead.h"
#include "CameraSets/Cs2InRow.h"
#include "CameraSets/Cs3InCube.h"
#include "CameraSets/Cs3InCubeFbo.h"
#include "CameraSets/Cs3InCubeScaler.h"
#include "CameraSets/Cs3InRow.h"
#include "CameraSets/Cs4Cube.h"
#include "CameraSets/Cs5CubeNoFloor.h"
#include "CameraSets/CsNFreeProp.h"


#ifdef HAVE_OPENCV

#ifdef WITH_AUDIO
#include "CameraSets/Cs1PerspFboFft.h"
#endif

#include "CameraSets/Cs1Fbo.h"
#include "CameraSets/Cs1FboAspect.h"
#include "CameraSets/Cs1PerspFboConv.h"
#include "CameraSets/Cs1PerspFboBack.h"
#include "CameraSets/Cs1PerspFboBackVid.h"
#include "CameraSets/Cs2InRowVert.h"
#include "CameraSets/Cs2InRowVertFbo.h"
#include "CameraSets/Cs3FloorWall.h"
#include "CameraSets/Cs3FloorWallFbo.h"
#include "CameraSets/Cs3Tunnel.h"
#include "CameraSets/Cs5Tunnel.h"
#include "CameraSets/Cs6FloorTunnel.h"
#endif

#include "Communication/OSC/OSCHandler.h"

namespace tav
{
class CameraFact
{
public:
	CameraFact(const std::string &sClassName)
	{
		msClassName = sClassName;
	}
	~CameraFact();

	CameraSet* Create(sceneData* scd, OSCData* _osc, OSCHandler* _osc_handler,
			GWindowManager* _winMan, std::vector<fboView*>* _screens)
	{
		if (!msClassName.compare("1Perspective"))
		{
			return new Cs1Perspective(scd, _osc);
		}
		else if (!msClassName.compare("1PerspBg"))
		{
			return new Cs1PerspBg(scd, _osc);
		}
		else if (!msClassName.compare("1Perspective2"))
		{
			return new Cs1Perspective2(scd, _osc);
		}
#ifdef HAVE_OPENCV
		else if (!msClassName.compare("1FboAspectQuad"))
		{
			return new Cs1FboAspectQuad(scd, _osc, _screens, _winMan);
		}
#ifdef WITH_AUDIO
		else if (!msClassName.compare("1PerspFboFft"))
		{
			return new Cs1PerspFboFft(scd, _osc, _screens, _winMan, _osc_handler);
		}
#endif
		else if (!msClassName.compare("1Fbo"))
		{
			return new Cs1Fbo(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("1FboAspect"))
		{
			return new Cs1FboAspect(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("1PerspFboBack"))
		{
			return new Cs1PerspFboBack(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("1PerspFboBackVid"))
		{
			return new Cs1PerspFboBackVid(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("1PerspFboConv"))
		{
			return new Cs1PerspFboConv(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("2InRowVert"))
		{
			return new Cs2InRowVert(scd, _osc, _winMan);
		}
		else if (!msClassName.compare("2InRowVertFbo"))
		{
			return new Cs2InRowVertFbo(scd, _osc, _winMan);
		}
		else if (!msClassName.compare("3FloorWall"))
		{
			return new Cs3FloorWall(scd, _osc, _winMan);
		}
		else if (!msClassName.compare("3FloorWallFbo"))
		{
			return new Cs3FloorWallFbo(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("3Tunnel"))
		{
			return new Cs3Tunnel(scd, _osc, _winMan);
		}
		else if (!msClassName.compare("5Tunnel"))
		{
			return new Cs5Tunnel(scd, _osc, _winMan);
		}
		else if (!msClassName.compare("6FloorTunnel"))
		{
			return new Cs6FloorTunnel(scd, _osc, _winMan);
		}
#endif
		else if (!msClassName.compare("2HeadHead"))
		{
			return new Cs2HeadHead(scd, _osc);
		}
		else if (!msClassName.compare("2InRow"))
		{
			return new Cs2InRow(scd, _osc);
		}
		else if (!msClassName.compare("3InCube"))
		{
			return new Cs3InCube(scd, _osc);
		}
		else if (!msClassName.compare("3InCubeFbo"))
		{
			return new Cs3InCubeFbo(scd, _osc, _screens, _winMan);
		}
		else if (!msClassName.compare("3InCubeScaler"))
		{
			return new Cs3InCubeScaler(scd, _osc);
		}
		else if (!msClassName.compare("3InRow"))
		{
			return new Cs3InRow(scd, _osc);
		}
		else if (!msClassName.compare("4Cube"))
		{
			return new Cs4Cube(scd, _osc);
		}
		else if (!msClassName.compare("5CubeNoFloor"))
		{
			return new Cs5CubeNoFloor(scd, _osc);
		}

		else if (!msClassName.compare("NFreeProp"))
		{
			return new CsNFreeProp(scd, _osc, _screens);
		}
		else
		{
			std::cerr << "CameraFact error, Prototype: " << msClassName
					<< " not found" << std::endl;
		}
		return 0;
	}

private:
	std::string msClassName;
};
}
