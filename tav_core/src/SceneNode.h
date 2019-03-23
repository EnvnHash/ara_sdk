//
//  SceneNode.h
//  tav_gl4
//
//  Created by Sven Hahne on 17.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <vector>
#include <cmath>
#include <functional>
#include <lo/lo.h>

#include "GLUtils/glm_utils.h"
#include "GLUtils/TFO.h"
#include "Shaders/ShaderUtils/MaterialProperties.h"
#include "Shaders/Shaders.h"
#include "headers/sceneData.h"
#include "headers/tav_types.h"
#include "Envelope.h"
#include "Median.h"
#include "math_utils.h"
#include "string_utils.h"

namespace tav
{
class SceneNode
{
public:
	typedef std::function<void(float)> transCbk;
	typedef struct
	{
		Median<float>* med;
		float rawVal = 0;
		bool doMedian = true;
	} oscVal;

	SceneNode(sceneData* _scd = 0, std::map<std::string, float>* _sceneArgs = 0,
			std::string _protoName = "NoLight");
	~SceneNode();
	virtual void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = 0);
	virtual void postDraw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = 0);
	virtual void update(double time, double dt); // alles was nicht von tfos aufgenommen werden soll
	virtual void updateOscPar(double time, double dt);

	virtual void sendPvmMatrix(Shaders* _shader, camPar* cp);
	virtual void sendStdShaderInit(Shaders* _shader);
	virtual void assignTexUnits(Shaders* _shader);
	virtual void useTextureUnitInd(int _unit, int _ind, Shaders* _shader = nullptr, TFO* _tfo = nullptr);
	virtual void onKey(int key, int scancode, int action, int mods);

	virtual SceneNode*					addChild();
	virtual SceneNode* 					addChild(SceneNode* newScene);

	virtual glm::vec3*					getCenter();
	virtual void 						getChanCols(glm::vec4** _chanCols);
	virtual std::vector<SceneNode*>* 	getChildren();
	virtual glm::vec3*					getDimension();
	virtual float 						getRotAngle();
	virtual glm::vec3*					getRotAxis();
	virtual glm::vec3*					getTransVec();
	virtual glm::vec3*					getScalingVec();
	virtual MaterialProperties*			getMaterial();
	virtual glm::mat4*					getModelMat();
	virtual std::string 				getName();
	virtual const float 				getOscPar(std::string _cmd);
	virtual std::string 				getOscStringPar(std::string _cmd);
	virtual const float 				getRawOscPar(std::string _cmd);
	virtual std::string* 				getShaderProtoName();


	virtual void 						setActive(bool _val);
	virtual void						setBlendMode(GLenum _src, GLenum _dst);
	virtual void						setBoundingBox(float _minX, float _maxX, float _minY, float _maxY,
														float _minZ, float _maxZ);
	virtual void						setBoundingBox(glm::vec3* _boundMin, glm::vec3* _boundMax);
	virtual void						setCenter(float _x, float _y, float _z);
	virtual void						setCenter(glm::vec3* in_center);
	virtual void 						setEnv(std::string* _cmd, lo_arg** argv, int* _cmdOffs, unsigned int nrEnvPoints);
	virtual void 						setName(std::string _name);
	//virtual void setAlpha(float _val);
	virtual void 						setOscPar(std::string* _cmd, float val, bool doMedian = true);
	virtual void						setOscStrPar(std::string* _cmd, char* val);
	virtual void 						setPar(std::string* _cmd, float val);
	virtual void 						setParentModelMat(glm::mat4 _inMat);
	virtual void 						setRotX(float _val);
	virtual void 						setRotY(float _val);
	virtual void 						setRotZ(float _val);
	virtual void 						setScaleX(float _val);
	virtual void 						setScaleY(float _val);
	virtual void 						setScaleZ(float _val);
	virtual void 						setShaderProtoName(std::string* _protName);
	virtual void 						setTransX(float _val);
	virtual void 						setTransY(float _val);
	virtual void 						setTransZ(float _val);

	virtual void 						translate();
	virtual void 						translate(glm::vec3& _transVec);
	virtual void 						rotate(float angle, float _x, float _y, float _z);
	virtual void 						rotate(float angle, glm::vec3& axis);
	virtual void 						rotate(); // with osc parameter
	virtual void 						scale();
	virtual void 						scale(glm::vec3& scaleVec);

	virtual void 						rebuildModelMat();

	virtual void 						addPar(std::string _cmd, float* ptr);
	virtual void 						addOscStringPar(std::string _cmd, std::string val);

	virtual void						dumpTreeIt(SceneNode* tNode, unsigned int level);
	virtual void						dumpTree();


	float*								initCol;

	sceneData* 							scd;
	std::map<std::string, float>* 		sceneArgs;
	std::string 						protoName;
	std::vector<auxTexPar> 				auxTex;
	std::vector<SceneNode*> 			children;
	MaterialProperties					_material;
	VAO*								_vao;

	glm::mat4 							_parentModelMat;
	glm::mat4 							_modelMat;
	glm::mat4 							_transMat;
	glm::mat4 							_rotMat;
	glm::mat4 							_scaleMat;
	glm::mat4 							_m_pvm;

	std::map<std::string, Envelope*> 	envs;
	std::map<std::string, float*> 		par;
	std::map<std::string, oscVal> 		oscPar;
	std::map<std::string, std::string> 	oscStrPar;

	std::vector<TextureManager*>		textures;
	std::vector<GLuint>					indices;
	std::map<std::string, transCbk> 	_transFuncMap;

	glm::vec3							_dimension;
	glm::vec3 							_transVec;
	glm::vec3 							_scaleVec;
	glm::vec3 							_rotVec;
	glm::vec3 							_rotAxis;
	float								_rotAngle;

	glm::vec3 							_diff3;
	glm::vec4 							_diff4;

	Median<glm::vec3>* 					_medTrans;
	Median<glm::vec3>* 					_medRot;
	Median<glm::vec3>* 					_medScale;
	Median<float>* 						_medAlpha;

	Quad* 								_stdQuad;
	Quad* 								_stdHFlipQuad;

	bool								hasBoundingBox;
	bool 								_active;
	bool								_cullFace;
	bool								_depthTest;
	bool								_drawIndexed;
	bool								_hasNewModelMat;
	bool								_hasNewOscValues;
	bool 								_updtTrans = false;
	bool 								_updtRot = false;
	bool 								_updtScale = false;

	GLenum								_blendSrc;
	GLenum								_blendDst;

	std::string 						name;

	glm::vec3							boundingBoxMin;
	glm::vec3							boundingBoxMax;
	glm::vec3							center;
};
}
