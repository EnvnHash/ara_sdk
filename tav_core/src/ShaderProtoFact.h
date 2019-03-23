//
//  ShaderProtoFact.h
//  tav_gl4
//
//  Created by Sven Hahne on 15.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <string>

#include "ShaderPrototype/ShaderProto.h"
#include "ShaderPrototype/SPDirLight.h"
#include "ShaderPrototype/SPDirLightNoTex.h"
#include "ShaderPrototype/SPLitSphere.h"
#include "ShaderPrototype/SPNoLight.h"
#include "ShaderPrototype/SPPerlinAudioSphere.h"
#include "ShaderPrototype/SPPerlinTexCoord.h"
#include "ShaderPrototype/SPPointLight.h"
#include "ShaderPrototype/SPPointLightCheap.h"
#include "ShaderPrototype/SPShadow.h"
#include "ShaderPrototype/SPSphereHarmonics.h"
#include "ShaderPrototype/SPSpotLight.h"

namespace tav
{
class ShaderProtoFact
{
public:
	ShaderProtoFact(const std::string &sClassName)
	{
		msClassName = sClassName;
	}
	;
	~ShaderProtoFact();

	ShaderProto * Create(void* shdrProtoData, ShaderCollector* _shCol,
			sceneData* _scd)
	{
		spData* sData = (spData*) (shdrProtoData);

		if (!msClassName.compare("DirLight"))
		{
			return new SPDirLight(sData, _shCol);
		}
		else if (!msClassName.compare("DirLightNoTex"))
		{
			return new SPDirLightNoTex(sData, _shCol);
		}
		else if (!msClassName.compare("LitSphere"))
		{
			return new SPLitSphere(sData, _shCol, _scd);
		}
		else if (!msClassName.compare("NoLight"))
		{
			return new SPNoLight(sData, _shCol);
		}
		else if (!msClassName.compare("PerlinAudioSphere"))
		{
			return new SPPerlinAudioSphere(sData, _shCol);
		}
		else if (!msClassName.compare("PerlinTexCoord"))
		{
			return new SPPerlinTexCoord(sData, _shCol);
		}
		else if (!msClassName.compare("PointLight"))
		{
			return new SPPointLight(sData, _shCol);
		}
		else if (!msClassName.compare("PointLightCheap"))
		{
			return new SPPointLightCheap(sData, _shCol);
		}
		else if (!msClassName.compare("Shadow"))
		{
			return new SPShadow(sData, _shCol);
		}
		else if (!msClassName.compare("SphereHarmonics"))
		{
			return new SPSphereHarmonics(sData, _shCol);
		}
		else if (!msClassName.compare("SpotLight"))
		{
			return new SPSpotLight(sData, _shCol);
		}
		else
		{
			std::cerr << "ShaderProtoFact error, Prototype: " << msClassName
					<< " not found" << std::endl;
		}
		return 0;
	}
	;

private:
	std::string msClassName;
};
}
