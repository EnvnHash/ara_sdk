//
//  ShaderProto.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "ShaderProto.h"

namespace tav
{
ShaderProto::ShaderProto(spData* _sData, ShaderCollector* _shCol) :
		sData(_sData), shCol(_shCol), debug(true)
{
	enableShdrType = new bool[SHTP_COUNT];
	for (int i = 0; i < SHTP_COUNT; i++)
		enableShdrType[i] = false;
	enableShdrType[VERTEX] = true;
	enableShdrType[FRAGMENT] = true;
}

ShaderProto::~ShaderProto()
{
	delete shader;
}

//--------------------------------------------------------------------------------

void ShaderProto::assemble()
{
	if (this->enableShdrType[GEOMETRY])
	{
		this->a.assemble(VERTEX, hasMultiCamGeo);
		if (this->debug)
			this->a.print();

		this->a.assemble(GEOMETRY, hasMultiCamGeo);
		if (this->debug)
			this->a.print();

		this->a.assemble(FRAGMENT, hasMultiCamGeo);
		if (this->debug)
			this->a.print();

		shader = new Shaders(a.getVertShader(), a.getGeomShader(),
				a.getFragShader(), false);
		glLinkProgram(shader->getProgram());

	}
	else
	{
		this->a.assemble(VERTEX, false);
		this->a.assemble(FRAGMENT, false);

		shader = new Shaders(a.getVertShader(), NULL, a.getFragShader(), false);
		glLinkProgram(shader->getProgram());
	}
}

//--------------------------------------------------------------------------------

// automatische erweiterung auf mehrere viewports und projektionsmatrizen
// nimmt auch die projections matrizen aus dem vertex shader und ersetzt
// sie durch instanzierbare im GeoShader

void ShaderProto::asmblMultiCamGeo()
{
	if (!hasMultiCamGeo)
		this->a.asmblMultiCamGeo(nrCams);
}

//--------------------------------------------------------------------------------

void ShaderProto::addHead2HeadEdgeBlend()
{
	this->a.setWorkShader(FRAGMENT);
	this->a.addUniform("float", "overlapV");
	this->a.addUniform("float", "scrHeight");
	this->a.addEdgeBlendCodeHead2Head();
}

//--------------------------------------------------------------------------------

void ShaderProto::add2RowsEdgeBlend()
{
	this->a.setWorkShader(FRAGMENT);
	this->a.addUniform("float", "overlapV");
	this->a.addUniform("float", "scrWidth");
	this->a.addEdgeBlendCode2();
}

//--------------------------------------------------------------------------------

void ShaderProto::add3RowsEdgeBlend()
{
	this->a.setWorkShader(FRAGMENT);
	this->a.addUniform("float", "overlapV");
	this->a.addUniform("float", "scrWidth");
	this->a.addEdgeBlendCode3();
}

//--------------------------------------------------------------------------------

void ShaderProto::addNRowsEdgeBlend(int nrCameras)
{
	this->a.setWorkShader(FRAGMENT);

	for (auto i = 0; i < nrCameras - 1; i++)
		this->a.addUniform("float", "overlapV" + std::to_string(i));

	this->a.addUniform("float", "scrWidth");
	this->a.addEdgeBlendCodeN(nrCameras);
}

//--------------------------------------------------------------------------------

void ShaderProto::setNrCams(int _nrCams)
{
	nrCams = _nrCams;
}

//--------------------------------------------------------------------------------

void ShaderProto::sendModelMatrix(camPar& cp)
{
	shader->setUniformMatrix4fv(stdMatrixNames[0], cp.model_matrix, 1);
}

//--------------------------------------------------------------------------------

void ShaderProto::sendViewMatrix(camPar& cp)
{
	shader->setUniformMatrix4fv(stdMatrixNames[1], cp.view_matrix, 1);
}

//--------------------------------------------------------------------------------

void ShaderProto::sendProjectionMatrix(camPar& cp)
{
	shader->setUniformMatrix4fv(stdMatrixNames[2], cp.projection_matrix, 1);
}

//--------------------------------------------------------------------------------

void ShaderProto::sendNormalMatrix(camPar& cp)
{
	shader->setUniformMatrix3fv(stdMatrixNames[3], cp.normal_matrix, 1);
}

//--------------------------------------------------------------------------------

void ShaderProto::sendOscPar(camPar& cp, OSCData* osc)
{
	// send total brightness
	shader->setUniform1f("brightness", osc->totalBrightness);
	shader->setUniform1f("oscAlpha", osc->alpha);

	if (!mCamModModInit)
	{
		mCamModModMatr = new glm::mat4[cp.nrCams];
		mCamModModInit = true;
		cp.multicam_model_matrix = &mCamModModMatr[0][0][0];
	}

	rotM = glm::rotate(osc->rotYAxis * static_cast<float>(M_PI) * 2.f,
			glm::vec3(0.f, 1.f, 0.f));
	cp.model_matrix_mat4 = rotM * glm::mat4(1.f);
	cp.model_matrix = &cp.model_matrix_mat4[0][0];

	// apply y-rotation transform
	for (auto i = 0; i < cp.nrCams; i++)
	{
		mCamModModMatr[i] = rotM * cp.multicam_modmat_src[i];
	}
}

//--------------------------------------------------------------------------------

Shaders* ShaderProto::getShader()
{
	return shader;
}
}
