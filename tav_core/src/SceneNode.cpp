//
//  SceneNode.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 17.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SceneNode.h"

namespace tav
{
SceneNode::SceneNode(sceneData* _scd, std::map<std::string, float>* _sceneArgs,
		std::string _protoName) :
		scd(_scd), sceneArgs(_sceneArgs), protoName(_protoName), hasBoundingBox(false), _cullFace(true),
		_depthTest(true), _drawIndexed(false), _blendSrc(GL_SRC_ALPHA), _blendDst(GL_ONE_MINUS_SRC_ALPHA),
		_vao(NULL), _hasNewOscValues(false), _hasNewModelMat(false), _rotAngle(0.f)
{
	if (scd)
	{
		_stdQuad = scd->stdQuad;
		_stdHFlipQuad = scd->stdHFlipQuad;
	}

	initCol = new float[4];
	initCol[0] = 1.f;
	initCol[1] = 1.f;
	initCol[2] = 1.f;
	initCol[3] = 1.f;

	_modelMat = glm::mat4(1.f);
	_transMat = glm::mat4(1.f);
	_rotMat = glm::mat4(1.f);
	_scaleMat = glm::mat4(1.f);

	_transVec = glm::vec3(0.f);
	_scaleVec = glm::vec3(1.f);
	_rotVec = glm::vec3(0.f, 1.f, 0.f);

	_dimension = glm::vec3(0.f);

	_medTrans = new Median<glm::vec3>(4.f);
	_medTrans->update(_transVec);
	_medRot = new Median<glm::vec3>(4.f);
	_medRot->update(_rotVec);
	_medScale = new Median<glm::vec3>(4.f);
	_medScale->update(_scaleVec);

	_parentModelMat = glm::mat4(1.f);

	_active = false;

	// dictionary zum direkten zugriff per oscCommando auf Funktionen des SceneNodes
	_transFuncMap["transX"] = [this](float val){ return this->setTransX(val);};
	_transFuncMap["transY"] = [this](float val){ return this->setTransY(val);};
	_transFuncMap["transZ"] = [this](float val){ return this->setTransZ(val);};

	_transFuncMap["rotX"] = [this](float val){ return this->setRotX(val);};
	_transFuncMap["rotY"] = [this](float val){ return this->setRotY(val);};
	_transFuncMap["rotZ"] = [this](float val){ return this->setRotZ(val);};

	_transFuncMap["scaleX"] = [this](float val){ return this->setScaleX(val);};
	_transFuncMap["scaleY"] = [this](float val){ return this->setScaleY(val);};
	_transFuncMap["scaleZ"] = [this](float val){ return this->setScaleZ(val);};

	name = "";

	// if there are is a nrChan Argument build the chan Map
	if(_sceneArgs != 0 && _sceneArgs->count( "nrChan" ) != 0)
	{
		int nrChan = int(_sceneArgs->at("nrChan"));
		scd->chanMap = new int[nrChan];

		for ( short i=0;i<nrChan; i++)
			if(_sceneArgs->count( "inChan"+std::to_string(i) ) != 0)
				scd->chanMap[i] = int(_sceneArgs->at("inChan"+std::to_string(i)));
	}
}

//---------------------------------------------------------------

SceneNode::~SceneNode()
{
	delete[] initCol;
	if (scd) delete[] scd->tex;
	if(_vao) delete _vao;
	for (std::vector< TextureManager* >::iterator it = textures.begin() ; it != textures.end(); ++it)
	    delete (*it);
}

//----------------------------------------------------

void SceneNode::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(_vao)
	{
		unsigned int texCnt = 0;
		glBlendFunc(_blendSrc, _blendDst);

		/*
		if (textures.size() > 0)
		{
			for (unsigned int t = 0; t < int(textures.size()); t++)
			{
				glActiveTexture(GL_TEXTURE0 + t);
				_shader->setUniform1i("tex" + std::to_string(t), t);
				textures[t]->bind();
				texCnt++;
			}

			_shader->setUniform1i("hasTexture", 1);

		} else _shader->setUniform1i("hasTexture", 0);
		*/

		if (_cullFace)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);

		if (_depthTest)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);


		if (_drawIndexed)
			_vao->drawElements(GL_TRIANGLES);
		else
			_vao->draw(GL_TRIANGLES);
	}
}

//----------------------------------------------------

void SceneNode::postDraw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
}

//---------------------------------------------------------

void SceneNode::update(double time, double dt)
{}

//---------------------------------------------------------

void SceneNode::updateOscPar(double time, double dt)
{
	if (_updtTrans)
	{
		_medTrans->update(_transVec);
		translate();
		rebuildModelMat();
		_diff3 = glm::abs(_medTrans->get() - _transVec);
		if (std::abs(_diff3.x + _diff3.y + _diff3.z) < 0.000001f)
			_updtTrans = false;
	}

	if (_updtRot)
	{
		_medRot->update(_rotVec);
		rotate();
		rebuildModelMat();
		_diff3 = glm::abs(_medRot->get() - _rotVec);
		if (std::abs(_diff3.x + _diff3.y + _diff3.z) < 0.000001f)
			_updtRot = false;
	}

	if (_updtScale)
	{
		_medScale->update(_scaleVec);
		scale();
		rebuildModelMat();
		_diff3 = glm::abs(_medScale->get() - _scaleVec);
		if (std::abs(_diff3.x + _diff3.y + _diff3.z) < 0.000001f)
			_updtScale = false;
	}

	// see if the paramter have to be smoothed
	for (std::map<std::string, oscVal>::iterator it = oscPar.begin(); it != oscPar.end(); ++it)
	{
		// if the parameter wasn't updated for a long time don't median filter
		if (!(*it).second.doMedian)
		{
			(*it).second.med->clear();
			(*it).second.med->update((*it).second.rawVal);
		}
		else
		{
			// if the last sent osc parameter is not equal the actual median value, actualize the median filter
			if (std::fabs((*it).second.med->get() - (*it).second.rawVal) > 0.00001f)
				(*it).second.med->update((*it).second.rawVal);
		}
	}

	// update paramters from the osc parameters
	for (std::map<std::string, float*>::iterator it = par.begin(); it != par.end(); ++it)
		if (oscPar.find((*it).first) != oscPar.end())
			*((*it).second) = oscPar[(*it).first].med->get();

	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

void SceneNode::onKey(int key, int scancode, int action, int mods)
{
}

//---------------------------------------------------------------

void SceneNode::sendPvmMatrix(Shaders* _shader, camPar* cp)
{
	if (cp->nrCams == 1)
	{
		//_m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
		_shader->setUniformMatrix4fv("_modelMatrix", &_modelMat[0][0]);
	}
	else
	{
		_m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * _modelMat;
		_shader->setUniformMatrix4fv("_m_pvm", &_m_pvm[0][0]);
	}
}

//---------------------------------------------------------------

void SceneNode::sendStdShaderInit(Shaders* _shader)
{
	// disable instancing explicitly. necessery for scene blending
	_shader->setUniform1i("useInstancing", 0);

	// init the aux colors, necessary if nothing is set from the setuploader
	for (auto i = 0; i < MAX_NUM_COL_SCENE; i++)
		_shader->setUniform4fv("auxCol" + std::to_string(i), &initCol[0]);

	assignTexUnits(_shader);

	if (scd->scnStructMode == BLEND)
	{
		glActiveTexture(GL_TEXTURE0 + 4); // 0-1 sind die texturebuffer von den tfos
		// an unit 4 kommen die "richtigen" texturen
	}
	else
	{
		glActiveTexture(GL_TEXTURE0);
	}

	glBindTexture(GL_TEXTURE_2D, 0); // standardmaessig auf 0 heisst keine Texture
}

//---------------------------------------------------------------

void SceneNode::assignTexUnits(Shaders* _shader)
{
	// hier sw fallback
	if (scd->scnStructMode == BLEND)
	{
		//for (auto i=0;i<MAX_NUM_SIM_TEXS;i++) scd->texNrs[i] = 4+i;
		_shader->setUniform1i("pos_tbo", 0);
		_shader->setUniform1i("nor_tbo", 1);
		_shader->setUniform1i("tex_tbo", 2);
		_shader->setUniform1i("col_tbo", 3);
		//_shader->setUniform1iv("texs", scd->texNrs, MAX_NUM_SIM_TEXS);   // muss gesetzt werden, sonst glsl fehler

		for (std::vector<auxTexPar>::iterator it = auxTex.begin();
				it != auxTex.end(); ++it)
			_shader->setUniform1i((*it).name, 4 + (*it).unitNr); // muss gesetzt werden, sonst glsl fehler

	}
	else
	{
//		for (auto i=0;i<MAX_NUM_SIM_TEXS;i++) scd->texNrs[i] = i;
//		_shader->setUniform1iv("texs", scd->texNrs, MAX_NUM_SIM_TEXS);   // muss gesetzt werden, sonst glsl fehler

		for (std::vector<auxTexPar>::iterator it = auxTex.begin();
				it != auxTex.end(); ++it)
			_shader->setUniform1i((*it).name, (*it).unitNr); // muss gesetzt werden, sonst glsl fehler
	}
}

//---------------------------------------------------------------

void SceneNode::useTextureUnitInd(int _unit, int _ind, Shaders* _shader,
		TFO* _tfo)
{
	assignTexUnits(_shader);

	if (scd->scnStructMode == BLEND)
		glActiveTexture(GL_TEXTURE0 + 4 + _unit);
	else
		glActiveTexture(GL_TEXTURE0 + _unit);

	glBindTexture(GL_TEXTURE_2D, _ind);

	if (_tfo)
		_tfo->addTexture(_unit, _ind, GL_TEXTURE_2D, "texs");

	for (std::vector<auxTexPar>::iterator it = auxTex.begin();
			it != auxTex.end(); ++it)
	{
		if (scd->scnStructMode == BLEND)
			glActiveTexture(GL_TEXTURE0 + 4 + (*it).unitNr);
		else
			glActiveTexture(GL_TEXTURE0 + (*it).unitNr);

		glBindTexture((*it).target, (*it).texNr);
		if (_tfo)
			_tfo->addTexture((*it).unitNr, (*it).texNr, (*it).target,
					(*it).name);
	}
}

//---------------------------------------------------------------

SceneNode* SceneNode::addChild()
{
	children.push_back(new SceneNode());
	return children.back();
}

//---------------------------------------------------------------

SceneNode* SceneNode::addChild(SceneNode* newScene)
{
	children.push_back(newScene);
	return children.back();
}

//--------------------------------------------------------------------------------

inline void SceneNode::getChanCols(glm::vec4** _chanCols)
{
	//std::cout << "make an array with " << scd->nrChannels << " entries" << std::endl;
	*_chanCols = new glm::vec4[scd->nrChannels];

	for (int i = 0; i < scd->nrChannels; i++)
	{
		std::string key = "col" + std::to_string(i);

		if (sceneArgs->find(key) != sceneArgs->end())
		{
			//std::cout << "assing _chanCols[" << i << "]" << " scd->colors[" <<  static_cast<unsigned int>( sceneArgs->at(key) ) << "]" << std::endl;
			for (int j = 0; j < 4; j++)
				(*_chanCols)[i][j] =
						scd->colors[static_cast<unsigned int>(sceneArgs->at(key))][j];

			//std::cout << "_chanCols[" << i << "]" << glm::to_string((*_chanCols)[i]) << std::endl;

		}
		else
		{
			std::cerr << "SceneNode::getChanCols Error: no" << key
					<< " definition in Setup XML.file!!!" << std::endl;
		}
	}
}

//---------------------------------------------------------------

std::vector<SceneNode*>* SceneNode::getChildren()
{
	return &children;
}

//--------------------------------------------------------------------------------

inline glm::vec3* SceneNode::getCenter() {

	return &center;
}

//---------------------------------------------------------------

std::string* SceneNode::getShaderProtoName()
{
	return &protoName;
}

//--------------------------------------------------------------------------------

glm::vec3* SceneNode::getDimension(){
	return &_dimension;
}

//--------------------------------------------------------------------------------

float SceneNode::getRotAngle(){

	return _rotAngle;
}

//--------------------------------------------------------------------------------

glm::vec3* SceneNode::getRotAxis(){

	return &_rotAxis;
}

//--------------------------------------------------------------------------------

glm::vec3* SceneNode::getTransVec(){

	return &_transVec;
}

//--------------------------------------------------------------------------------

glm::vec3* SceneNode::getScalingVec(){

	return &_scaleVec;
}

//--------------------------------------------------------------------------------

MaterialProperties* SceneNode::getMaterial(){

	return &_material;
}

//--------------------------------------------------------------------------------

glm::mat4* SceneNode::getModelMat(){

	return &_modelMat;
}

//---------------------------------------------------------------

std::string SceneNode::getName()
{
	return name;
}

//--------------------------------------------------------------------------------

inline const float SceneNode::getRawOscPar(std::string _cmd)
{
	float out = 0.f;
	if (oscPar.find(_cmd) != oscPar.end())
	{
		out = oscPar[_cmd].rawVal;
	}
	return static_cast<const float>(out);
}

//--------------------------------------------------------------------------------

inline const float SceneNode::getOscPar(std::string _cmd)
{
	float out = 0.f;
	if (oscPar.find(_cmd) != oscPar.end())
	{
		out = oscPar[_cmd].med->get();
	}
	return static_cast<const float>(out);
}

//--------------------------------------------------------------------------------

inline std::string SceneNode::getOscStringPar(std::string _cmd)
{
	std::string out;
	if (oscStrPar.find(_cmd) != oscStrPar.end())
		out = oscStrPar[_cmd];
	return out;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setActive(bool _val)
{
	_active = _val;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setBlendMode(GLenum _src, GLenum _dst)
{
	_blendSrc = _src; _blendDst = _dst;
}

//--------------------------------------------------------------------------------

void SceneNode::setBoundingBox(float _minX, float _maxX, float _minY, float _maxY,
		float _minZ, float _maxZ) {

	boundingBoxMin = glm::vec3(_minX, _minY, _minZ);
	boundingBoxMax = glm::vec3(_maxX, _maxY, _maxZ);
	_dimension = boundingBoxMax - boundingBoxMin;
	hasBoundingBox = true;
}

//--------------------------------------------------------------------------------

void SceneNode::setBoundingBox(glm::vec3* _boundMin, glm::vec3* _boundMax){

	boundingBoxMin = *_boundMin;
	boundingBoxMax = *_boundMax;
	_dimension = *_boundMax - *_boundMin;
	hasBoundingBox = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setCenter(float _x, float _y, float _z) {

	center = glm::vec3(_x, _y, _z);
}

//--------------------------------------------------------------------------------

void SceneNode::setCenter(glm::vec3* in_center){

	center = *in_center;
		//printf("SceneNode::setParentNode node: \"%s\", _hasNewModelMat = true \n", getName());
	//_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setEnv(std::string* _cmd, lo_arg** argv, int* _cmdOffs,
		unsigned int nrEnvPoints)
{
	// if envelope doesn't exist, create it
	if (envs.find(*_cmd) != envs.end())
	{
		envs[*_cmd]->clear();

		// numer of envelope points comes as nrTimes + nrValues ([time, values])
		for (unsigned int i = 0; i < nrEnvPoints; i++)
		{
			envs[*_cmd]->addTime(argv[*_cmdOffs]->f);
			envs[*_cmd]->addValue(argv[*_cmdOffs + nrEnvPoints]->f);
			(*_cmdOffs)++;
		}

		//std::cout << "got envelope" << std::endl;
		//envs[*_cmd]->dump();

	}
	else
	{
		// if the envelopse doesn't exist, create it
		envs[*_cmd] = new Envelope();
	}
}

//--------------------------------------------------------------------------------

//inline void SceneNode::setAlpha(float _val)
//{
//	newAlpha = _val;
//	updateAlpha = true;
//}

//---------------------------------------------------------------

void SceneNode::setName(std::string _name)
{
	//std::cout << "SceneNode::setName: " << _name << std::endl;
	name = _name;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setTransX(float _val)
{
	_transVec.x = _val;
	_updtTrans = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setTransY(float _val)
{
	_transVec.y = _val;
	_updtTrans = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setTransZ(float _val)
{
	_transVec.z = _val;
	_updtTrans = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setRotX(float _val)
{
	_rotVec.x = _val;
	_updtRot = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setRotY(float _val)
{
	_rotVec.y = _val;
	_updtRot = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setRotZ(float _val)
{
	_rotVec.z = _val;
	_updtRot = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setScaleX(float _val)
{
	_scaleVec.x = _val;
	_updtScale = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setScaleY(float _val)
{
	_scaleVec.y = _val;
	_updtScale = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setScaleZ(float _val)
{
	_scaleVec.z = _val;
	_updtScale = true;
}

//---------------------------------------------------------------

void SceneNode::setShaderProtoName(std::string* _protoName)
{
	protoName = *_protoName;
}

//--------------------------------------------------------------------------------

inline void SceneNode::setPar(std::string* _cmd, float val)
{
	if (std::strcmp((*_cmd).c_str(), "active") == 0)
	{
		_active = bool(val);
	}
	else
	{
		if (_transFuncMap.find(*_cmd) != _transFuncMap.end())
		{
			_transFuncMap[(*_cmd)](val);
		}
		else
		{
			if (par.find(*_cmd) != par.end())
			{
				_hasNewOscValues = true;
				//std::cerr << "SceneNode::setPar set par " << *_cmd << " val: " << val << std::endl;
				*par[*_cmd] = val;
			} else {
			//	std::cerr << "SceneNode::setPar couldn't find par" << std::endl;
			}
		}
	}
}

//--------------------------------------------------------------------------------

inline void SceneNode::setOscStrPar(std::string* _cmd, char* val)
{
	if (oscStrPar.find(*_cmd) != oscStrPar.end())
	{
		// replace "\\n" with "\n"
		oscStrPar[*_cmd] = ReplaceString(static_cast<std::string>(val), "\\n", "\n");
	}
}

//--------------------------------------------------------------------------------

inline void SceneNode::setOscPar(std::string* _cmd, float val, bool doMedian)
{
	if (std::strcmp((*_cmd).c_str(), "active") == 0)
	{
		_active = bool(val);
	}
	else
	{
		if (_transFuncMap.find(*_cmd) != _transFuncMap.end())
		{
			std::cout << "is transFunc" << std::endl;
			_transFuncMap[(*_cmd)](val);
		}
		else
		{
			// if the command doesn't exist as oscPar but as par, create a oscPar entryc
			if (oscPar.find(*_cmd) != oscPar.end())
			{
				oscPar[*_cmd].rawVal = val;
				oscPar[*_cmd].doMedian = doMedian;

			}
			else if (par.find(*_cmd) != par.end())
			{
				oscPar[*_cmd] = oscVal();
				oscPar[*_cmd].med = new Median<float>(4);
				oscPar[*_cmd].med->update(val);
			}
		}
	}
}

//--------------------------------------------------------------------------------

inline void SceneNode::setParentModelMat(glm::mat4 _inMat)
{
	_parentModelMat = _inMat;
	rebuildModelMat();
}

//--------------------------------------------------------------------------------

inline void SceneNode::translate()
{
	_transMat = glm::translate(glm::mat4(1.f), _medTrans->get());
	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::translate(glm::vec3& transVec){

	_transVec = transVec;
	_transMat = glm::translate(glm::mat4(1.f), _transVec);
	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------
// for compatibility reasons left this function...

inline void SceneNode::rotate()
{
	//_rotMat = glm::eulerAngleXYZ(_medRot->get().x, _medRot->get().y, _medRot->get().z);
	// _hasNewModelMat = true;
	//glm::quat rot = glm::eulerAngleXYZ(_medRot->get().x, _medRot->get().y, _medRot->get().z);
	glm::quat rot = glm::quat(glm::vec3(_medRot->get().x, _medRot->get().y, _medRot->get().z));
	glm::vec3 ax = glm::axis(rot);
	rotate(glm::angle(rot), ax);
}


//--------------------------------------------------------------------------------

inline void SceneNode::rotate(float angle, float _x, float _y, float _z){

	_rotAngle = angle;
	_rotAxis = glm::vec3(_x, _y, _z);
	_rotMat = glm::rotate(_rotAngle, _rotAxis);
	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::rotate(float angle, glm::vec3& axis){

	_rotAngle = angle;
	_rotAxis = axis;
	_rotMat = glm::rotate(_rotAngle, _rotAxis);
	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::scale()
{
	_scaleMat = glm::scale(_medScale->get());
	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::scale(glm::vec3& scaleVec) {

	_scaleVec = scaleVec;
	_scaleMat = glm::scale(_scaleVec);
	_hasNewModelMat = true;
}

//--------------------------------------------------------------------------------

inline void SceneNode::rebuildModelMat()
{
	_modelMat = _parentModelMat * _transMat * _rotMat * _scaleMat;
}

//--------------------------------------------------------------------------------

inline void SceneNode::addPar(std::string _cmd, float* ptr)
{
	if (_transFuncMap.find(_cmd) != _transFuncMap.end())
	{
		std::cerr << "SceneNode::addPar Error: this Name is reserved!!! " << std::endl;
	}
	else
	{
		//std::cerr << "SceneNode::addPar added command " << _cmd << std::endl;
		par[_cmd] = ptr;
	}
}

//--------------------------------------------------------------------------------

inline void SceneNode::addOscStringPar(std::string _cmd, std::string val)
{
	oscStrPar[_cmd] = val;
}

//---------------------------------------------------------------

void SceneNode::dumpTreeIt(SceneNode* tNode, unsigned int level)
{
	unsigned int newLevel = level +1;

	std::vector<SceneNode*>* children = tNode->getChildren();
	for ( std::vector<SceneNode*>::iterator it = children->begin(); it != children->end(); ++it)
	{
		for (unsigned int i=0;i<level;i++) printf("\t");
		printf("[\"%s\"]\n", (*it)->getName().c_str());

		for (unsigned int i=0;i<level;i++) printf("\t");
		printf("trans: (%f|%f|%f), \n", (*it)->getTransVec()->x, (*it)->getTransVec()->y, (*it)->getTransVec()->z);

		for (unsigned int i=0;i<level;i++) printf("\t");
		printf("scale: (%f|%f|%f)\n", (*it)->getScalingVec()->x, (*it)->getScalingVec()->y, (*it)->getScalingVec()->z);

		for (unsigned int i=0;i<level;i++) printf("\t");
		printf("rotation: (a: %f|%f|%f|%f)\n", (*it)->getRotAngle(), (*it)->getRotAxis()->x, (*it)->getRotAxis()->y, (*it)->getRotAxis()->z);

		for (unsigned int i=0;i<level;i++) printf("\t");
		printf("dimension: (%f|%f|%f)\n", (*it)->getDimension()->x, (*it)->getDimension()->y, (*it)->getDimension()->z);

		dumpTreeIt((*it), newLevel);
	}
}

//---------------------------------------------------------------

void SceneNode::dumpTree(){

	printf("\n####################################\n");

	printf("SceneNode[%s] DUMP: \n", name.c_str());
	dumpTreeIt(this, 1);

	printf("####################################\n\n");

}
}
