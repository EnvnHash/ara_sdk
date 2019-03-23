//
//  AssimpImport.cpp
//
//  Created by Sven Hahne on 14/8/15.
//

#include "pch.h"
#include "AssimpImport.h"

#define STRINGIFY(A) #A
#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

namespace tav
{

AssimpImport::AssimpImport(sceneData* _scd, bool _drawIndexed,  bool _debug) :
	scd(_scd), scrWidth(_scd->screenWidth), scrHeight(_scd->screenHeight),
	drawIndexed(_drawIndexed), debug(_debug), debugGL(false), glOk(false), loadSuccess(false),
	m_programPipeline(0), normShader(0), isObj(false), useModelRoot(true), scene(NULL) {

	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	bBox = static_cast<BoundingBoxer*>(_scd->boundBoxer);

	// get a handle to the predefined STDOUT log stream and attach
	// it to the logging system. It remains active for all further
	// calls to aiImportFile(Ex) and aiApplyPostProcessing.
	if (debug)
	{
		aiEnableVerboseLogging(false);
		stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
		aiAttachLogStream(&stream);
	}

	float aspect = float(scrWidth) / float(scrHeight);
	cam = new GLMCamera(GLMCamera::FRUSTUM,
		scrWidth, scrHeight,
		-aspect, aspect,	// left, right
		-1.f, 1.f,			// bottom, top
		0.f, 0.f, 1.f,      // camPos
		0.f, 0.f, 0.f,		// lookAt
		0.f, 1.f, 0.f,
		1.f, 100.f);

	normalMatr = glm::mat3(1.f);
}

//------------------------------------------

///> All the Meshes inside a Model will be added to one ScenenNode which gets returned
void AssimpImport::load(const char* _filePath,  SceneNode* _modelRoot, std::function<void()> _cb){

	loadEndCb = _cb;
	filePath = std::string(_filePath);
	modelRoot = _modelRoot;

	// load model in a separate thread
	if (debug){

		printf("AssimpImport starting loading model: %s \n", _filePath);
		startTime = std::chrono::system_clock::now();
	}

	// start the loading thread
	std::thread lThread(&AssimpImport::loadThread, this, true);
	lThread.detach();
}

//------------------------------------------

void AssimpImport::loadThread(bool optimize)
{
	if (access(filePath.c_str(), F_OK) != -1)
	{
		if(filePath.substr(filePath.find_last_of(".") + 1) == "obj") {
			isObj = true;
		}

		fileAbsolutePath = GetDirectory(filePath);

		if (debug)
			printf("AssimpImport::loadThread: loadModel %s fileAbsolutePath %s \n", filePath.c_str(), fileAbsolutePath.c_str());

		// aiProcess_FlipUVs is for VAR code. Not needed otherwise. Not sure why.
		unsigned int flags = aiProcessPreset_TargetRealtime_Fast;

		if (optimize)
			flags |= aiProcess_CalcTangentSpace
			| aiProcess_GenNormals
			| aiProcess_JoinIdenticalVertices
			| aiProcess_Triangulate
			| aiProcess_GenUVCoords
			| aiProcess_SortByPType;


		if (drawIndexed)
			flags |= aiProcess_JoinIdenticalVertices;


		import = new Assimp::Importer();
		//scene = std::shared_ptr<const aiScene>(import->ReadFile(filePath, flags));

		scene = std::shared_ptr<const aiScene>(aiImportFileExWithProperties(filePath.c_str(), flags, NULL, store.get()), aiReleaseImport);

		if (scene != 0)
		{
			get_bounding_box(&this->scene_min, &this->scene_max);

			if (debug){

				printf("AssimpImport::loadThread: scene min: %f, %f, %f \n", scene_min.x, scene_min.y, scene_min.z);
				printf("AssimpImport::loadThread: scene max: %f, %f, %f \n", scene_max.x, scene_max.y, scene_max.z);
			}

			scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
			scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
			scene_center.z = (scene_min.z + scene_max.z) / 2.0f;


			// y and z swapped???? error with 3ds models
			if (useModelRoot)
			{
				modelRoot->setBoundingBox(scene_min.x, scene_max.x, scene_min.z, scene_max.z, scene_min.y, scene_max.y);
				modelRoot->setCenter(getCenter().x, getCenter().z, getCenter().y);
			}

			glm::vec3 dim = getDimensions();
			glm::vec3 center = getCenter();

			if (debug){

				printf( ("AssimpImport::loadThread: dimensions: "+glm::to_string(dim)+"\n").c_str() );
				printf( ("AssimpImport::loadThread: center: "+glm::to_string(center)+"\n").c_str() );
			}

			// scale the scene so that its y-dimension is 2.f -1|1
			normalizedScale = 2.f / dim.y;

			normCubeAndCenterMatr = glm::scale(glm::mat4(1.f), glm::vec3(2.f / dim.x, 2.f / dim.y, 2.f / dim.z))
					* glm::translate(glm::mat4(1.f), -center);

			normAndCenterMatr = glm::scale(glm::mat4(1.f),glm::vec3((float)normalizedScale))
					* glm::translate(glm::mat4(1.f), -center);

			normAndCenterZOffsMatr = cam->getMVP()
					* glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f))
					* glm::scale(glm::mat4(1.f), glm::vec3((float)normalizedScale))
					* glm::translate(glm::mat4(1.f), -center);

			loadSuccess = true;

			if (debug)
			{
				endTime = std::chrono::system_clock::now();
				double dur = std::chrono::duration <double, std::milli>(endTime - startTime).count();
				printf( "AssimpImport loading model done, took: %f ms \n", dur );
			}

			std::vector< std::function<void()> >* openGlCbs = static_cast< std::vector< std::function<void()> >* >(scd->openGlCbs);
			openGlCbs->push_back( std::bind(&AssimpImport::uploadToGL, this) );	// add a callback to the sceneData openGlCbs vector with the local uploadToGl() method

		} else
		{
			if (debug)
			{
				printf("AssimpImport::loadModel Error: couldn´t import scene \n" );
				printf( ("ERROR::ASSIMP "+std::string(import->GetErrorString())).c_str() );
			}
		}
	} else {

		printf("loadModel(): model does not exist: %s \n", filePath.c_str() );
	}
}

//------------------------------------------

void AssimpImport::uploadToGL()
{
	aiColor4D dcolor, scolor, acolor, ecolor;

	if (debugGL)
		printf("AssimpImport::loadGLResources  loading number of meshes: %d \n ", scene->mNumMeshes);

	// create OpenGL buffers and populate them based on each meshes pertinant info.
	for (unsigned int i=0; i < scene->mNumMeshes; ++i)
	{
		if (debugGL) printf("AssimpImport::loadGLResources mesh nr: %d \n", i );

		// current mesh we are introspecting
		aiMesh* mesh = scene->mMeshes[i];

		// add a new SNAssimpMesh SceneNode
		SceneNode* meshSN = modelRoot->addChild();
		meshSN->setActive(true);
		meshSN->_depthTest = true;
		meshSN->_drawIndexed = drawIndexed;
		meshSN->setName( static_cast<std::string>(mesh->mName.C_Str()) );
		//meshSN->_transFlag |= MC3D_NO_SCALE;

		// Handle material info
		aiMaterial* mtl = scene->mMaterials[mesh->mMaterialIndex];

		// get name
		C_STRUCT aiString mat_name;
		aiGetMaterialString(mtl, AI_MATKEY_NAME, &mat_name);

		if (debugGL) printf("AssimpImport::loadGLResources  handle material: %s \n", mat_name.data);

		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &dcolor))
			meshSN->getMaterial()->setDiffuse(dcolor.r, dcolor.g, dcolor.b, dcolor.a);

		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &scolor))
			meshSN->getMaterial()->setSpecular(scolor.r, scolor.g, scolor.b, scolor.a);

		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &acolor))
			meshSN->getMaterial()->setAmbient(acolor.r, acolor.g, acolor.b, acolor.a);

		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &ecolor))
			meshSN->getMaterial()->setEmissive(ecolor.r, ecolor.g, ecolor.b, ecolor.a);


		int shadeMode;
		aiGetMaterialInteger(mtl, AI_MATKEY_SHADING_MODEL, &shadeMode);
		if ((aiShadingMode)shadeMode == aiShadingMode_Phong)
		{
//			printf( "shadeMode is phong \n" );
		}


		// .mtl format doesn't have a shininess parameter, assimp anyway says there is one...
		float shininess;
		if (!isObj && AI_SUCCESS == aiGetMaterialFloat(mtl, AI_MATKEY_SHININESS, &shininess))
			meshSN->getMaterial()->setShininess(shininess);

		int blendMode;
		if (AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_BLEND_FUNC, &blendMode))
		{
			if (blendMode == aiBlendMode_Default)
				meshSN->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else
				meshSN->setBlendMode(GL_SRC_ALPHA, GL_ONE);
		}

		// Culling
		unsigned int max = 1;
		int two_sided;
		if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
			meshSN->_cullFace = true;
		else
			meshSN->_cullFace = false;


		if (debugGL) printf("AssimpImport::loadGLResources loading textures \n" );


		// Load Textures
		int texIndex = 0;
		bool loadedTextures = false;
		bool wrongfilePath = false;
		aiString texPath;
		std::string texPathStr = std::string(texPath.data);
		std::string realfilePath;

		if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath)){

			std::string relPath = GetDirectory( texPathStr );

			if (debugGL) printf( ("AssimpImport::loadGLResource(): texPath.data: "+(std::string)texPath.data+" \n").c_str() );
			if (debugGL) printf( ("AssimpImport::loadGLResource(): raw relPath: "+relPath+" \n").c_str() );

			// check if the filePath is absolute or relative
			std::size_t foundPos = relPath.find_first_of("/\\");
			if (debugGL) printf( "AssimpImport::loadGLResource(): foundPos: %d \n", foundPos );

			if (foundPos != 2)
			{
				foundPos = relPath.find_last_of("/\\");
				if (foundPos != std::string::npos)
					relPath = relPath.substr(0, relPath.find_last_of("/\\"));
				else
					relPath = "";

				if (debugGL) printf( "AssimpImport::loadGLResource(): relPath: %s \n", relPath.c_str() );

				std::string texFile = texPath.data;
				realfilePath = fileAbsolutePath;

				realfilePath += relPath;
#ifdef _WIN32
				realfilePath += "\\";
#else
				realfilePath += "\/";
#endif
				realfilePath += texFile;

			} else {

				realfilePath = (std::string)texPath.data;
			}

			if (access(realfilePath.c_str(), F_OK) == -1)
			{
				if (debugGL) printf("AssimpImport::loadGLResource(): texture doesn't exist: %s \n", realfilePath.c_str() );
				wrongfilePath = true;
			}


			bool bTextureAlreadyExists = false;

			for (int j=0; j<static_cast<int>(meshSN->textures.size()); j++)
			{
				if (std::strcmp( (*meshSN->textures[j]->getFileName()).c_str(), realfilePath.c_str()) == 0)
				{
					bTextureAlreadyExists = true;
					break;
				}
			}

			if (bTextureAlreadyExists)
			{
				if (debugGL) printf(("AssimpImport::loadGLResource(): texture already loaded: "+realfilePath+" \n").c_str());

			} else
			{
				if (debugGL) printf(("AssimpImport::loadGLResource(): trying to load texture : "+realfilePath+" \n").c_str());

				meshSN->textures.push_back(new tav::TextureManager());
				tav::TextureManager* texture = meshSN->textures.back();
				bool bTextureLoadedOk = texture->loadTexture2D(realfilePath.c_str()) > 0 ? true : false;

				if (bTextureLoadedOk)
					printf("AssimpImport::loadGLResource(): texture loaded, dimensions: %d x %d\n", texture->getWidth(), texture->getHeight() );
				else
					printf(("AssimpImport::loadGLResource(): couldn't load texture: "+realfilePath+"\n").c_str());
			}

			loadedTextures = true;
		}

		if (debugGL) printf("AssimpImport::loadGLResources loading meshes mesh->mNumVertices %d \n", mesh->mNumVertices);

		//meshHelper->animatedPos.resize(mesh->mNumVertices);
		//if (mesh->HasNormals()) meshHelper->animatedNorm.resize(mesh->mNumVertices);

		int usage = GL_STREAM_DRAW;

		if (getAnimationCount())
			usage = GL_STREAM_DRAW;
		else
			usage = GL_STATIC_DRAW;

		// now upload all data to a vao.
		// init vao, check format
		std::string format = "position:3f";

		if (mesh->HasNormals())	format += ",normal:3f";
		if (mesh->HasTextureCoords(0))  format += ",texCoord:2f";

		// also if there are no colors, assing a static color for debugging reasons
		format += ",color:4f";


		if (debugGL) printf("AssimpImport::loadGLResources upload to vaos mesh->mNumVertices %d \n", mesh->mNumVertices);

		meshSN->_vao = new VAO(format.c_str(), GL_DYNAMIC_DRAW);
		meshSN->_vao->upload(POSITION, &mesh->mVertices[0].x, mesh->mNumVertices);


		if (debugGL) printf("AssimpImport::loadGLResources upload to vaos done, uploading normals \n"  );

		if (mesh->HasNormals())
			meshSN->_vao->upload(NORMAL, &mesh->mNormals[0].x, mesh->mNumVertices);


		if (debugGL) printf("AssimpImport::loadGLResources upload to vaos done, uploading normals done \n" );

		if (mesh->HasTextureCoords(0)) {

			glm::vec2* vec = new glm::vec2[mesh->mNumVertices];

			if (debugGL) printf( "AssimImport::loadGLResources Mesh has textures \n" );

			for (unsigned int tc=0; tc<mesh->mNumVertices; tc++) {

				// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set [0].
				if (mesh->mTextureCoords[0] && mesh->mTextureCoords[0][tc].x <= 1.0) {

					vec[tc].x = mesh->mTextureCoords[0][tc].x;
					vec[tc].y = mesh->mTextureCoords[0][tc].y;
				}
			}

			meshSN->_vao->upload(TEXCOORD, &vec[0][0], mesh->mNumVertices);
		}


		if (debugGL) printf("AssimpImport::loadGLResources upload to vaos done, uploading texCoords done \n" );
		if (debugGL) printf("AssimpImport::loadGLResources upload to vaos done, setting static color \n" );

		if (!mesh->HasVertexColors(0))
		{
			if (mesh->mTextureCoords[0] && loadedTextures)
				meshSN->_vao->setStaticColor(0.f, 0.f, 0.f, 1.f);
			else
				meshSN->_vao->setStaticColor(1.f, 1.f, 1.f, 1.f);
		}
		else
			meshSN->_vao->upload(COLOR, &mesh->mColors[0][0].r, mesh->mNumVertices);

		if (debugGL) printf("AssimpImport::loadGLResources upload to vaos done, setting static color done \n" );
		if (debugGL) printf("AssimpImport::loadGLResources uploading indices: %d \n", mesh->mNumFaces);

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
				meshSN->indices.push_back(mesh->mFaces[i].mIndices[j]);

		meshSN->_vao->setElemIndices((int) meshSN->indices.size(), (GLuint*) &meshSN->indices[0]);

		// note: no need to set/calculate the bounding box here, since this is done in the scene-draw loop automatically

	//	normalizePositions(meshSN, mesh); // eigentlich falsch...
	}

	if (debugGL) printf("AssimpImport::loadGLResources scene done \n" );

	/*
	int numOfAnimations = scene->mNumAnimations;
	for (int i=0; i<numOfAnimations; i++)
	{
		aiAnimation * animation = scene->mAnimations[i];
		animations.push_back(AssimpAnimation(scene, animation));
	}
	*/

	// Object Map and LightShadow Maps have to be regenerated
	/*
	scd->renderPassMtx->lock();
	scd->reqRenderPasses->push_back(MC3D_OBJECT_MAP_PASS);
	scd->reqRenderPasses->push_back(MC3D_SHADOW_MAP_PASS);
	scd->reqRenderPasses->push_back(MC3D_OBJECT_ID_PASS);
	scd->renderPassMtx->unlock();
*/

	loadEndCb(); // call load end callback

	// note: no need to set/calculate the bounding box here, since this is done in the scene-draw loop automatically
	/*
	// just set the hasBoundingBox Flags for all the parent nodes up to the root
	SceneNode* parent = modelRoot->getParentNode();
	while (parent)
	{
		parent->hasBoundingBox = false;
		parent = parent->getParentNode();
	}
	*/

	delete this;
}

//-------------------------------------------

void AssimpImport::createEmptyModel()
{
	if (scene) clear();
}

//-------------------------------------------

void AssimpImport::createLightsFromAiModel()
{
	/*
	 lights.clear();
	 lights.resize(scene->mNumLights);
	 for(int i=0; i<(int)scene->mNumLights; i++){
	 lights[i].enable();
	 if(scene->mLights[i]->mType==aiLightSource_DIRECTIONAL){
	 lights[i].setDirectional();
	 lights[i].setOrientation(aiVecToOfVec(scene->mLights[i]->mDirection));
	 }
	 if(scene->mLights[i]->mType!=aiLightSource_POINT){
	 lights[i].setSpotlight();
	 lights[i].setPosition(aiVecToOfVec(scene->mLights[i]->mPosition));
	 }
	 lights[i].setAmbientColor(aiColorToOfColor(scene->mLights[i]->mColorAmbient));
	 lights[i].setDiffuseColor(aiColorToOfColor(scene->mLights[i]->mColorDiffuse));
	 lights[i].setSpecularColor(aiColorToOfColor(scene->mLights[i]->mColorSpecular));
	 }
	 */
}

//-------------------------------------------

void AssimpImport::optimizeScene()
{
	aiApplyPostProcessing(scene.get(),
			aiProcess_ImproveCacheLocality | aiProcess_OptimizeGraph
					| aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices
					| aiProcess_RemoveRedundantMaterials);
}

//-------------------------------------------

std::string AssimpImport::GetDirectory(std::string& _filePath)
{
	size_t found = _filePath.find_last_of("/\\");
	return (_filePath.substr(0, found));
}


//--------------------------------------------------------------------------------------------------------
///> since the SceneEditor needs to be able to transform the imported models and the original values
///> need to be visible, the meshes vertex position are normalized and its original position / scaling are reflected into the scaling transform vectors of its SceneNode
///> and the vertices in the VBOs are normalized between [-0.5 | 0.5]

void AssimpImport::normalizePositions(SceneNode* tNode, aiMesh* tMesh)
{
	if (debugGL) printf("AssimpImport::normalizePositions \n");

	bBox->begin();
	tNode->_vao->draw(GL_TRIANGLES);
	bBox->end();

	// run a compute shader on the vao and save the result into normPos
	if (!normShader) initNormShader();

	//-------------------------------------------------------------------------

	glUseProgram(normShader);

	// send uniforms
	int loc = glGetUniformLocation(normShader, "numVertices");
	if (loc != -1) glUniform1ui(loc, (GLuint) tMesh->mNumVertices);

	loc = glGetUniformLocation(normShader, "center");
	if (loc != -1) glUniform3f(loc, bBox->getCenter()->x, bBox->getCenter()->y, bBox->getCenter()->z);
	tNode->translate( *bBox->getCenter() );
	if (debugGL) printf("AssimpImport::normalizePositions got translate %f %f %f \n", bBox->getCenter()->x, bBox->getCenter()->y, bBox->getCenter()->z );


	loc = glGetUniformLocation(normShader, "bBoxSize");
	glm::vec3 bBoxSize = *bBox->getBoundMax() - *bBox->getBoundMin();
	if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(bBoxSize));

	// check for zero scaling
	for (unsigned int j=0; j<3;j++)
		if (bBoxSize[j] == 0.f) bBoxSize[j] = 1.f;

	tNode->scale( bBoxSize );
	if (debugGL) printf("AssimpImport::normalizePositions got scale %f %f %f \n", bBoxSize.x, bBoxSize.y, bBoxSize.z );

	//-------------------------------------------------------------------------

	glUseProgram(0);

	// Invoke the compute shader to normalize the vertices
	glBindProgramPipeline(m_programPipeline);

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, tNode->_vao->getVBO(POSITION) );
	glDispatchCompute((GLuint)(float(tMesh->mNumVertices) / 128.f + 1.f), 1, 1); // work_group_size = 128

	// We need to block here on compute completion to ensure that the
	// computation is done before we render
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindProgramPipeline(0);
}

//-------------------------------------------

void AssimpImport::initNormShader()
{
	GLint status;

	// std140 pads vec3 to 4 components, to deal with vec3 we have to use a proper struct
	std::string src = STRINGIFY(
		layout(std430, binding=1 ) buffer Vertices { Position pos[];\n }\n;
		layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;\n
		uniform uint numVertices;\n
		uniform vec3 center;\n
		uniform vec3 bBoxSize;\n
		void main() {
			uint i = gl_GlobalInvocationID.x;
			if (i >= numVertices) return;
			vec3 p = ( vec3(pos[i].x, pos[i].y, pos[i].z) - center );
			pos[i].x = bBoxSize.x != 0.0 ? p.x / bBoxSize.x : p.x;\n
			pos[i].y = bBoxSize.y != 0.0 ? p.y / bBoxSize.y : p.y;\n
			pos[i].z = bBoxSize.z != 0.0 ? p.z / bBoxSize.z : p.z;\n
		});


	glGenProgramPipelines(1, &m_programPipeline);

	// since the layout std430 and std140 are padding vec3 to vec4, we have to use a proper struct to
	// deal with vec3
	const GLchar* fullSrc[2] = { "#version 430\n#define WORK_GROUP_SIZE 128\nstruct Position { float x, y, z; };\n", src.c_str() };
	normShader = glCreateShaderProgramv(GL_COMPUTE_SHADER, 2, fullSrc); // with this command GL_PROGRAM_SEPARABLE is set to true

	GLint logLength;
	glGetProgramiv(normShader, GL_INFO_LOG_LENGTH, &logLength);

	char *log = new char[logLength];
	glGetProgramInfoLog(normShader, logLength, 0, log);
	if (debug) printf(log);
	delete[] log;

	glBindProgramPipeline(m_programPipeline);
	glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, normShader);
	glValidateProgramPipeline(m_programPipeline);
	glGetProgramPipelineiv(m_programPipeline, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE)
	{
		GLint logLength;
		glGetProgramPipelineiv(m_programPipeline, GL_INFO_LOG_LENGTH, &logLength);

		char *log = new char[logLength];
		memset(log, 0, logLength);
		glGetProgramPipelineInfoLog(m_programPipeline, logLength, 0, log);
		printf("Shader pipeline not valid:\n%s\n", log);
		delete[] log;
	}

	glBindProgramPipeline(0);


	if (debug) printf("AssimpImport::initNormShader finished \n" );
}

//-------------------------------------------

void AssimpImport::deleteNormShader()
{
	glDeleteProgramPipelines(1, &m_programPipeline);
	glDeleteProgram(normShader);
}

//-------------------------------------------

void AssimpImport::clear()
{
	// clear out everything.
	animations.clear();
	modelMeshes.clear();
	pos = glm::vec3(0.f, 0.f, 0.f);
	scale = glm::vec3(1.f, 1.f, 1.f);
	rotAngle.clear();
	rotAxis.clear();
	//lights.clear();

	scale = glm::vec3(1, 1, 1);
	normalizeScale = true;
	bUsingMaterials = true;
	bUsingNormals = true;
	bUsingTextures = true;
	bUsingColors = true;

	//updateModelMatrix();
}

//------------------------------------------- update.

void AssimpImport::update(double time)
{
	if (glOk)
	{
		if (!scene)
			return;
		updateAnimations(time);
		updateMeshes(scene->mRootNode, glm::mat4(1.f));

		if (hasAnimations() == false)
			return;

		updateBones();
		updateGLResources();
	}
}

//-------------------------------------------

void AssimpImport::updateAnimations(double time)
{
	if (glOk)
	{
		for (unsigned int i = 0; i < animations.size(); i++)
			animations[i].update(time);
	}
}

//-------------------------------------------

void AssimpImport::updateMeshes(aiNode * node, glm::mat4 parentMatrix) {

	if (glOk){

		aiMatrix4x4 m = node->mTransformation;
		m.Transpose();
		glm::mat4 matrix(m.a1, m.a2, m.a3, m.a4, m.b1, m.b2, m.b3, m.b4, m.c1, m.c2,
			m.c3, m.c4, m.d1, m.d2, m.d3, m.d4);
		matrix *= parentMatrix;

		for (unsigned int i = 0; i < node->mNumMeshes; i++){

			int meshIndex = node->mMeshes[i];
			AssimpMeshHelper* mesh = modelMeshes[meshIndex];
			mesh->matrix = matrix;
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
			updateMeshes(node->mChildren[i], matrix);
	}
}

//-------------------------------------------

void AssimpImport::updateBones() {

	if (glOk){

		// update mesh position for the animation
		for (unsigned int i = 0; i < modelMeshes.size(); ++i){

			// current mesh we are introspecting
			const aiMesh* mesh = modelMeshes[i]->mesh;

			// calculate bone matrices
			std::vector<aiMatrix4x4> boneMatrices(mesh->mNumBones);
			for (unsigned int a = 0; a < mesh->mNumBones; ++a)
			{
				const aiBone* bone = mesh->mBones[a];

				// find the corresponding node by again looking recursively through the node hierarchy for the same name
				aiNode* node = scene->mRootNode->FindNode(bone->mName);

				// start with the mesh-to-bone matrix
				boneMatrices[a] = bone->mOffsetMatrix;

				// and now append all node transformations down the parent chain until we're back at mesh coordinates again
				const aiNode* tempNode = node;

				while (tempNode)
				{
					// check your matrix multiplication order here!!!
					boneMatrices[a] = tempNode->mTransformation * boneMatrices[a];
					// boneMatrices[a] = boneMatrices[a] * tempNode->mTransformation;
					tempNode = tempNode->mParent;
				}
				modelMeshes[i]->hasChanged = true;
			}

			modelMeshes[i]->animatedPos.assign(modelMeshes[i]->animatedPos.size(),
				aiVector3D(0.0f));

			if (mesh->HasNormals())
				modelMeshes[i]->animatedNorm.assign(
					modelMeshes[i]->animatedNorm.size(), aiVector3D(0.0f));

			// loop through all vertex weights of all bones
			for (unsigned int a = 0; a < mesh->mNumBones; ++a)
			{
				const aiBone* bone = mesh->mBones[a];
				const aiMatrix4x4& posTrafo = boneMatrices[a];

				for (unsigned int b = 0; b < bone->mNumWeights; ++b)
				{
					const aiVertexWeight& weight = bone->mWeights[b];

					size_t vertexId = weight.mVertexId;
					const aiVector3D& srcPos = mesh->mVertices[vertexId];

					modelMeshes[i]->animatedPos[vertexId] += weight.mWeight
						* (posTrafo * srcPos);
				}

				if (mesh->HasNormals())
				{
					// 3x3 matrix, contains the bone matrix without the translation, only with rotation and possibly scaling
					aiMatrix3x3 normTrafo = aiMatrix3x3(posTrafo);
					for (unsigned int b = 0; b < bone->mNumWeights; ++b)
					{
						const aiVertexWeight& weight = bone->mWeights[b];
						size_t vertexId = weight.mVertexId;

						const aiVector3D& srcNorm = mesh->mNormals[vertexId];
						modelMeshes[i]->animatedNorm[vertexId] += weight.mWeight
							* (normTrafo * srcNorm);
					}
				}
			}
		}
	}
}

//-------------------------------------------

void AssimpImport::updateGLResources() {

	if (glOk){

		/*
		 // now upload the result position and normal along with the other vertex attributes into a dynamic vertex buffer, VBO or whatever
		 for (unsigned int i = 0; i < modelMeshes.size(); ++i)
		 {
		 if(modelMeshes[i].hasChanged)
		 {
		 const aiMesh* mesh = modelMeshes[i].mesh;
		 modelMeshes[i].vbo.updateVertexData(&modelMeshes[i].animatedPos[0].x,mesh->mNumVertices);

		 if(mesh->HasNormals())
		 modelMeshes[i].vbo.updateNormalData(&modelMeshes[i].animatedNorm[0].x,mesh->mNumVertices);

		 modelMeshes[i].hasChanged = false;
		 }
		 }
		 */
	}
}

//------------------------------------------- animations.

bool AssimpImport::hasAnimations()
{
	return animations.size() > 0;
}

//-------------------------------------------

unsigned int AssimpImport::getAnimationCount()
{
	return static_cast<unsigned int>(animations.size());
}

//-------------------------------------------

AssimpAnimation& AssimpImport::getAnimation(int animationIndex)
{
	animationIndex = (int)glm::clamp((float)animationIndex, 0.f, static_cast<float>(animations.size() - 1));
	return animations[animationIndex];
}

//-------------------------------------------

void AssimpImport::playAllAnimations()
{
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		animations[i].play();
	}
}

//-------------------------------------------

void AssimpImport::stopAllAnimations()
{
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		animations[i].stop();
	}
}

//-------------------------------------------

void AssimpImport::resetAllAnimations()
{
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		animations[i].reset();
	}
}

//-------------------------------------------

void AssimpImport::setPausedForAllAnimations(bool pause)
{
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		animations[i].setPaused(pause);
	}
}

//-------------------------------------------

void AssimpImport::setLoopStateForAllAnimations(
		enum AssimpAnimation::loopType state)
{
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		animations[i].setLoopState(state);
	}
}

//-------------------------------------------

void AssimpImport::setPositionForAllAnimations(float position)
{
	for (unsigned int i = 0; i < animations.size(); i++)
	{
		animations[i].setPosition(position);
	}
}

//------------------------------------------- meshes.

bool AssimpImport::hasMeshes()
{
	return modelMeshes.size() > 0;
}

//-------------------------------------------

unsigned int AssimpImport::getMeshCount()
{
	return static_cast<unsigned int>(modelMeshes.size());
}

//-------------------------------------------

AssimpMeshHelper* AssimpImport::getMeshHelper(int meshIndex)
{
	meshIndex = static_cast<int>( tav::clamp((float)meshIndex, 0.f, static_cast<float>(modelMeshes.size() - 1)) );
	return modelMeshes[meshIndex];
}

// ----------------------------------------------------------------------------

void AssimpImport::get_bounding_box(aiVector3D* min, aiVector3D* max)
{
	aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;

	get_bounding_box_for_node(scene->mRootNode, min, max, &trafo);
}

//-------------------------------------------

void AssimpImport::get_bounding_box_for_node(const aiNode* nd, aiVector3D* min,
		aiVector3D* max, aiMatrix4x4* trafo)
{
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo, &nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n)
	{
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t)
		{

			aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp, trafo);

			min->x = aisgl_min(min->x, tmp.x);
			min->y = aisgl_min(min->y, tmp.y);
			min->z = aisgl_min(min->z, tmp.z);

			max->x = aisgl_max(max->x, tmp.x);
			max->y = aisgl_max(max->y, tmp.y);
			max->z = aisgl_max(max->z, tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n)
	{
		get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);
	}
	*trafo = prev;
}

//-------------------------------------------

void AssimpImport::drawMeshNoShdr(unsigned int meshNr, GLenum renderType)
{
	if (loadSuccess)
	{
		AssimpMeshHelper* mesh = modelMeshes[std::max(static_cast<unsigned int>(0),
			std::min(meshNr, static_cast<unsigned int>(modelMeshes.size() - 1)))];
		if (drawIndexed)
			mesh->vao->drawElements(renderType);
		else
			mesh->vao->draw(renderType);
	}
}

//-------------------------------------------

void AssimpImport::sendMeshMatToShdr(unsigned int meshNr, tav::Shaders* shdr)
{
	if (loadSuccess)
	{
		AssimpMeshHelper* mesh = modelMeshes[std::max(static_cast<unsigned int>(0),
			std::min(meshNr, static_cast<unsigned int>(modelMeshes.size() - 1)))];
		if (bUsingMaterials)
			mesh->material.sendToShader(shdr->getProgram());
	}
}

//-------------------------------------------

void AssimpImport::drawMesh(unsigned int meshNr, GLenum renderType,
		Shaders* shdr)
{
	if (glOk)
	{
		AssimpMeshHelper* mesh = modelMeshes[std::max(static_cast<unsigned int>(0),
			std::min(meshNr, static_cast<unsigned int>(modelMeshes.size() - 1)))];

		if (bUsingTextures && mesh->hasTexture())
			for (unsigned int t = 0; t < int(mesh->textures.size()); t++)
			{
				glActiveTexture(GL_TEXTURE0 + t);
				shdr->setUniform1i("tex_diffuse" + std::to_string(t), t);
				mesh->textures[t]->bind();
			}

		if (bUsingMaterials)
			mesh->material.sendToShader(shdr->getProgram());

		if (mesh->twoSided)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);

		if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ALPHA)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ADD)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		if (drawIndexed)
			mesh->vao->drawElements(renderType);
		else
			mesh->vao->draw(renderType);
	}
}

//-------------------------------------------

void AssimpImport::draw(GLenum renderType, Shaders* shdr)
{
	if (glOk)
	{
		for (unsigned int i = 0; i < static_cast<unsigned int>(modelMeshes.size()); i++)
		{
			AssimpMeshHelper* mesh = modelMeshes[i];

			if (bUsingTextures && mesh->hasTexture())
			{
				for (unsigned int t = 0; t < int(mesh->textures.size()); t++)
				{
					glActiveTexture(GL_TEXTURE0 + t);
					shdr->setUniform1i("tex_diffuse" + std::to_string(t), t);
					mesh->textures[t]->bind();
				}
			}

			if (bUsingMaterials)
				mesh->material.sendToShader(shdr->getProgram());

			if (mesh->twoSided)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);

			if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ALPHA)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ADD)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if (drawIndexed)
				mesh->vao->drawElements(renderType);
			else
				mesh->vao->draw(renderType);
		}
	}
}

//-------------------------------------------

void AssimpImport::drawInstanced(GLenum renderType, unsigned int nrInst,
		Shaders* shdr)
{
	if (glOk)
	{
		for (unsigned int i = 0; i < static_cast<unsigned int>(modelMeshes.size());
			i++)
		{
			AssimpMeshHelper* mesh = modelMeshes[i];

			if (bUsingTextures && mesh->hasTexture())
			{
				for (unsigned int t = 0; t < int(mesh->textures.size()); t++)
				{
					glActiveTexture(GL_TEXTURE0 + t);
					shdr->setUniform1i("tex_diffuse" + std::to_string(t), t);
					mesh->textures[t]->bind();
				}
			}

			if (bUsingMaterials)
				mesh->material.sendToShader(shdr->getProgram());

			if (mesh->twoSided)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);

			if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ALPHA)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ADD)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if (drawIndexed)
				mesh->vao->drawElementsInst(renderType, nrInst, 0, renderType);
			else
				mesh->vao->drawInstanced(renderType, nrInst, 0, (float)mesh->mesh->mNumVertices);
		}
	}
}

//-------------------------------------------

void AssimpImport::drawNoText(unsigned int meshNr, GLenum renderType,
		Shaders* shdr)
{
	if (glOk)
	{
		for (unsigned int i = 0; i < static_cast<unsigned int>(modelMeshes.size());
			i++)
		{
			AssimpMeshHelper* mesh = modelMeshes[i];

			if (bUsingMaterials)
				mesh->material.sendToShader(shdr->getProgram());

			if (mesh->twoSided)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);

			if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ALPHA)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ADD)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if (drawIndexed)
				mesh->vao->drawElements(renderType);
			else
				mesh->vao->draw(renderType);
		}
	}
}

//-------------------------------------------

void AssimpImport::drawNormCenter(GLenum renderType, Shaders* shdr){

	if (glOk) {

		shdr->setUniformMatrix4fv("m_pvm", &normAndCenterZOffsMatr[0][0]);
		shdr->setUniformMatrix3fv("m_normal", &normalMatr[0][0]);

		for (unsigned int i = 0; i < modelMeshes.size(); i++) {

			AssimpMeshHelper* mesh = modelMeshes[i];

			if (bUsingTextures && mesh->hasTexture()) {

				for (unsigned int t = 0; t < int(mesh->textures.size()); t++) {

					glActiveTexture(GL_TEXTURE0 + t);
					shdr->setUniform1i("tex_diffuse" + std::to_string(t), t);
					mesh->textures[t]->bind();
				}
			}

			if (bUsingMaterials)
				mesh->material.sendToShader(shdr->getProgram());

			if (mesh->twoSided)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);

			if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ALPHA)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else if (mesh->blendMode == AssimpMeshHelper::BLENDMODE_ADD)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if (drawIndexed)
				mesh->vao->drawElements(renderType);
			else
				mesh->vao->draw(renderType);
		}
	}
}

//-------------------------------------------

std::vector<std::string> AssimpImport::getMeshNames()
{
	std::vector<std::string> names(scene->mNumMeshes);

	for (int i = 0; i < (int) scene->mNumMeshes; i++)
		names[i] = scene->mMeshes[i]->mName.data;

	return names;
}

//-------------------------------------------

int AssimpImport::getNumMeshes()
{
	return scene->mNumMeshes;
}

//-------------------------------------------

Mesh* AssimpImport::getMesh(std::string name)
{
	Mesh* _mesh = NULL;

	// default to triangle mode
	//ofm.setMode(OF_PRIMITIVE_TRIANGLES);
	aiMesh* aim = NULL;

	for (int i = 0; i < (int) scene->mNumMeshes; i++)
	{
		if (std::string(scene->mMeshes[i]->mName.data) == name)
		{
			aim = scene->mMeshes[i];
			break;
		}
	}

	if (!aim)
		printf( ("couldn't find mesh " +name).c_str() );
	else
		aiMeshToTavMesh(aim, _mesh);

	return _mesh;
}

//-------------------------------------------

Mesh* AssimpImport::getMesh(int num)
{
	Mesh* _mesh = NULL;

	if (static_cast<int>(scene->mNumMeshes) <= num)
		printf( ("couldn't find mesh " + std::to_string(num) + " there's only " + std::to_string(scene->mNumMeshes)).c_str() );
	else
		aiMeshToTavMesh(scene->mMeshes[num], _mesh);

	return _mesh;
}

//-------------------------------------------

VAO* AssimpImport::getVao(int num)
{
	VAO* _vao = NULL;

	if (static_cast<int>(scene->mNumMeshes) <= num)
		printf( ("couldn't find mesh "+std::to_string(num)+" there's only "+ std::to_string(scene->mNumMeshes)).c_str() );
	else
		_vao = modelMeshes[num]->vao;

	return _vao;
}

//-------------------------------------------

Mesh* AssimpImport::getCurrentAnimatedMesh(std::string name)
{
	Mesh* ptr = 0;
	bool found = false;

	for (int i = 0; i < (int) modelMeshes.size(); i++)
	{
		if (std::string(modelMeshes[i]->mesh->mName.data) == name)
		{
			/*
			 if(!modelMeshes[i]->validCache)
			 {
			 modelMeshes[i]->cachedMesh.clear_positions();
			 modelMeshes[i]->cachedMesh.clear_normals();
			 
			 std::vector<glm::vec3> pos = aiVecVecToGlmVecVec(modelMeshes[i]->animatedPos);
			 for(short j=0;j<(int)pos.size();j++)
			 modelMeshes[i]->cachedMesh.push_back_positions(&pos[j][0], 3);
			 
			 std::vector<glm::vec3> norm = aiVecVecToGlmVecVec(modelMeshes[i]->animatedNorm);
			 for(short j=0;j<(int)pos.size();j++)
			 modelMeshes[i]->cachedMesh.push_back_normals(&norm[j][0], 3);
			 
			 modelMeshes[i]->validCache = true;
			 
			 found = true;
			 }
			 ptr = &modelMeshes[i]->cachedMesh;
			 */
		}
	}

	if (!found)
		printf("couldn't find mesh %s \n", name.c_str());
	return ptr;
}
//-------------------------------------------

Mesh* AssimpImport::getCurrentAnimatedMesh(int num)
{
	Mesh* ptr = 0;

	if (static_cast<int>(modelMeshes.size()) <= num)
	{
		printf(("couldn't find mesh "+std::to_string(num)+" there's only " + std::to_string(scene->mNumMeshes)).c_str());
	}
	else
	{
		/*
		 if(!modelMeshes[num]->validCache)
		 {
		 modelMeshes[num]->cachedMesh.clear_positions();
		 modelMeshes[num]->cachedMesh.clear_normals();
		 
		 std::vector<glm::vec3> pos = aiVecVecToGlmVecVec(modelMeshes[num]->animatedPos);
		 for(short i=0;i<(int)pos.size();i++)
		 modelMeshes[i]->cachedMesh.push_back_positions(&pos[i][0], 3);
		 
		 std::vector<glm::vec3> norm = aiVecVecToGlmVecVec(modelMeshes[num]->animatedNorm);
		 for(short i=0;i<(int)pos.size();i++)
		 modelMeshes[i]->cachedMesh.push_back_normals(&norm[i][0], 3);
		 
		 modelMeshes[num]->validCache = true;
		 
		 }
		 ptr = &modelMeshes[num]->cachedMesh;
		 */
	}

	return ptr;
}

//-------------------------------------------

MaterialProperties* AssimpImport::getMaterialForMesh(std::string name)
{
	for (int i = 0; i < (int) modelMeshes.size(); i++)
		if (std::string(modelMeshes[i]->mesh->mName.data) == name)
			return &modelMeshes[i]->material;

	printf("couldn't find mesh %s\n", name.c_str());
	return 0;
}

//-------------------------------------------

MaterialProperties* AssimpImport::getMaterialForMesh(int num)
{
	if ((int) modelMeshes.size() <= num)
	{
		printf("couldn't find mesh %d there's only %d", num, scene->mNumMeshes);
		return 0;
	}
	return &modelMeshes[num]->material;
}

//-------------------------------------------

TextureManager* AssimpImport::getTextureForMesh(std::string name, int ind)
{
	bool found = false;
	TextureManager* ptr = 0;

	for (int i = 0; i < (int) modelMeshes.size(); i++)
		if (std::string(modelMeshes[i]->mesh->mName.data) == name)
		{
			found = true;
			ptr = modelMeshes[i]->textures[ind];
		}

	if (!found)
		printf("couldn't find mesh %s \n", name.c_str());

	return ptr;
}

//-------------------------------------------

TextureManager* AssimpImport::getTextureForMesh(int num, int ind)
{
	if (static_cast<int>(modelMeshes.size()) <= num)
	{
		std::cerr << "AssimpImport::getTextureForMesh Error: couldn't find mesh " << num << " there's only " << scene->mNumMeshes << std::endl;
		return nullptr;
	}
	return modelMeshes[num]->textures[ind];
}

//-------------------------------------------

glm::vec3 AssimpImport::getPosition()
{
	return pos;
}

//-------------------------------------------

glm::vec3 AssimpImport::getSceneCenter()
{
	return aiVecToOfVec(scene_center);
}

//-------------------------------------------

float AssimpImport::getNormalizedScale()
{
	return (float)normalizedScale;
}

//-------------------------------------------

glm::vec3 AssimpImport::getScale()
{
	return scale;
}

//-------------------------------------------

glm::mat4* AssimpImport::getNormAndCenterMatr()
{
	return &normAndCenterMatr;
}

//-------------------------------------------

glm::mat4* AssimpImport::getNormCubeAndCenterMatr()
{
	return &normCubeAndCenterMatr;
}

//-------------------------------------------

glm::mat4* AssimpImport::getNormAndCenterZOffsMatr()
{
	return &normAndCenterZOffsMatr;
}

//-------------------------------------------

glm::vec3 AssimpImport::getDimensions()
{
	return glm::vec3(scene_max.x, scene_max.y, scene_max.z)
			- glm::vec3(scene_min.x, scene_min.y, scene_min.z);
}

//-------------------------------------------

glm::vec3 AssimpImport::getCenter()
{
	return glm::vec3(scene_center.x, scene_center.y, scene_center.z);
}

//-------------------------------------------

glm::vec3 AssimpImport::getSceneMin(bool bScaled)
{
	glm::vec3 sceneMin(scene_min.x, scene_min.y, scene_min.z);

	if (bScaled)
		return sceneMin * scale;
	else
		return sceneMin;
}

//-------------------------------------------

glm::vec3 AssimpImport::getSceneMax(bool bScaled)
{
	glm::vec3 sceneMax(scene_max.x, scene_max.y, scene_max.z);

	if (bScaled)
		return sceneMax * scale;
	else
		return sceneMax;
}

//-------------------------------------------

int AssimpImport::getNumRotations()
{
	return static_cast<int>(rotAngle.size());
}

//-------------------------------------------

glm::vec3 AssimpImport::getRotationAxis(int which)
{
	if (static_cast<unsigned int>(rotAxis.size()) > static_cast<unsigned int>(which))
		return rotAxis[which];
	else
		return glm::vec3();
}

//-------------------------------------------

float AssimpImport::getRotationAngle(int which)
{
	if (static_cast<unsigned int>(rotAngle.size()) > static_cast<unsigned int>(which))
		return rotAngle[which];
	else
		return 0.f;
}

//-------------------------------------------

const aiScene* AssimpImport::getAssimpScene()
{
	return scene.get();
}

//--------------------------------------------------------------

void AssimpImport::enableTextures()
{
	bUsingTextures = true;
}

//--------------------------------------------------------------

void AssimpImport::enableNormals()
{
	bUsingNormals = true;
}

//--------------------------------------------------------------

void AssimpImport::enableColors()
{
	bUsingColors = true;
}

//--------------------------------------------------------------

void AssimpImport::enableMaterials()
{
	bUsingMaterials = true;
}

//--------------------------------------------------------------

void AssimpImport::disableTextures()
{
	bUsingTextures = false;
}

//--------------------------------------------------------------

void AssimpImport::disableNormals()
{
	bUsingNormals = false;
}

//--------------------------------------------------------------

void AssimpImport::disableColors()
{
	bUsingColors = false;
}

//--------------------------------------------------------------

void AssimpImport::disableMaterials()
{
	bUsingMaterials = false;
}

//------------------------------------------

AssimpImport::~AssimpImport()
{
	clear();
	store.reset();
	scene.reset();
	delete cam;
	aiDetachAllLogStreams();
	deleteNormShader();
}
}
