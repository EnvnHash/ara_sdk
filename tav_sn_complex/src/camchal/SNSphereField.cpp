//
// SNSphereField.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#define STRINGIFY(A) #A

#include "SNSphereField.h"

namespace tav
{
SNSphereField::SNSphereField(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs), m_noiseSize(16.f)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// setup chanCols
	getChanCols(&chanCols);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// init osc parameter
	addPar("spSize", &spSize);
	addPar("fluidForce", &fluidForce);
	addPar("perlForce", &perlForce);
	addPar("randAmt", &randAmt);
	addPar("timeScale", &timeScale);
	addPar("oscSpeed", &oscSpeed);
	addPar("oscAmt", &oscAmt);
	addPar("xPos", &xPos);
	addPar("posRandSpd", &posRandSpd);
	addPar("posRandAmtX", &posRandAmtX);
	addPar("posRandAmtY", &posRandAmtY);
	addPar("velScale", &velScale);
	addPar("rad", &rad);
	addPar("timeStep", &timeStep);

    propo = _scd->roomDim->x / _scd->roomDim->y;

    // init Fluid Sim
    int scrScale = 2;
    flWidth = std::min(_scd->fboWidth, 1280) / scrScale;
    flHeight = int( float(flWidth) / propo );
    fluidSize =  glm::vec2(static_cast<float>(flWidth), static_cast<float>(flHeight));

    fluidSim = new GLSLFluid(false, shCol);
    fluidSim->allocate(flWidth, flHeight, 0.5f);
    fluidSim->setGravity(glm::vec2(0.0f, 0.0f));
    fluidSim->dissipation = 0.99f;
    fluidSim->velocityDissipation = 0.9999f;

    // init default VAO
	glGenVertexArrays(1, &defaultVAO);
	glBindVertexArray(defaultVAO);

	texShdr = shCol->getStdTex();
	initShaders();
	initMisc();
	initScene();

	glBindVertexArray(0);

	initFramebuffers(scd->fboWidth, scd->fboHeight, tweak.samples);

	m_control.m_sceneOrbit = glm::vec3(0.0f);
	m_control.m_sceneDimension = float(globalscale);
	m_control.m_sceneOrthoZoom = m_control.m_sceneDimension;

	//projection.nearplane = m_control.m_sceneDimension * 1.f;
	projection.nearplane = m_control.m_sceneDimension * 0.01f;
	projection.farplane  = m_control.m_sceneDimension * 10.0f;

	noiseTex = new Noise3DTexGen(shCol,
			true, 4,
			256, 256, 64,
			4.f, 4.f, 16.f);

    quad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
                    glm::vec3(0.f, 0.f, 1.f),
                    0.f, 0.f, 0.f, 1.f);


    // test texture
    litsphereTex = new TextureManager();
    litsphereTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/Unknown-28.jpeg");
}

//----------------------------------------------------

void SNSphereField::initShaders()
{
	com = "#define UBO_SCENE " + std::to_string( UBO_SCENE) + "\n";
	com += "#define AO_RANDOMTEX_SIZE " + std::to_string(AO_RANDOMTEX_SIZE) + "\n";
	com += STRINGIFY(struct SceneData {\n
		mat4  viewProjMatrix;\n
		mat4  viewMatrix;\n
		mat4  viewMatrixIT;\n
		uvec2 viewport;\n
		uvec2 _pad;\n
	};\n

	struct HBAOData {\n
		float   	RadiusToScreen;\n        // radius
		float   	R2;\n     // 1/radius
		float   	NegInvR2;\n     // radius * radius
		float   	NDotVBias;\n
		vec2    	InvFullResolution;\n
		vec2    	InvQuarterResolution;\n
		float   	AOMultiplier;\n
		float   	PowExponent;\n
		vec2    	_pad0;\n
		vec4    	projInfo;\n
		vec2    	projScale;\n
		int     	projOrtho;\n
		int     	_pad1;\n
		vec4    	float2Offsets[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];\n
		vec4    	jitters[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];\n
	};\n);
//----------------------------------------------------

	shdr_Header = "#version 430\n#define AO_LAYERED " + std::to_string( USE_AO_LAYERED_SINGLEPASS) + "\n";
//----------------------------------------------------

	initSceneShdr();
	initFullScrQuad();
	initBilateralblur();

	std::string frag = initDepthLinearize(0);
	depth_linearize = shCol->addCheckShaderText("SNSphereFieldDepth_linearize", fullScrQuad.c_str(), frag.c_str());

	frag = initDepthLinearize(1);
	depth_linearize_msaa = shCol->addCheckShaderText("SNSphereFieldDepth_linearize_msaa", fullScrQuad.c_str(), frag.c_str());

	initViewNormal();
	initDisplayTex();

	frag = initHbaoCalc(0, 0);
	hbao_calc = shCol->addCheckShaderText("SNSphereFieldHBAO_Calc", fullScrQuad.c_str(), frag.c_str());

	frag = initHbaoCalc(1, 0);
	hbao_calc_blur = shCol->addCheckShaderText("SNSphereFieldHBAO_Calc_Blur", fullScrQuad.c_str(), frag.c_str());

	frag = initHbaoBlur(0);
	hbao_blur = shCol->addCheckShaderText("SNSphereFieldHBAO_Blur", fullScrQuad.c_str(), frag.c_str());

	frag = initHbaoBlur(1);
	hbao_blur2 = shCol->addCheckShaderText("SNSphereFieldHBAO_BlurMsaa", fullScrQuad.c_str(), frag.c_str());

	frag = initHbaoCalc(0, 1);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
	hbao2_calc = shCol->addCheckShaderText("SNSphereFieldHBAO2_Calc", fullScrQuad.c_str(), fullScrQuadGeo.c_str(), frag.c_str());
#else
	hbao2_calc = shCol->addCheckShaderText("SNSphereFieldHBAO2_Calc", fullScrQuad.c_str(), frag.c_str());
#endif

	frag = initHbaoCalc(1, 1);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
	hbao2_calc_blur = shCol->addCheckShaderText("SNSphereFieldHBAO2_Calc_Blur", fullScrQuad.c_str(), fullScrQuadGeo.c_str(), frag.c_str());
#else
	hbao2_calc_blur = shCol->addCheckShaderText("SNSphereFieldHBAO2_Calc_Blur", fullScrQuad.c_str(), frag.c_str());
#endif

	initDeinterleave();

	frag = initReinterleave(0);
	hbao2_reinterleave = shCol->addCheckShaderText("SNSphereFieldHBAO2_Reinterleave", fullScrQuad.c_str(), frag.c_str());

	frag = initReinterleave(1);
	hbao2_reinterleave_blur = shCol->addCheckShaderText("SNSphereFieldHBAO2_Reinterleave_Blur", fullScrQuad.c_str(), frag.c_str());

	// init compute shader for offseting the positions of the spheres
	std::string src = initComputeUpdtShdr();
	m_updateProg = createShaderPipelineProgram(GL_COMPUTE_SHADER, src.c_str());
}

//----------------------------------------------------

void SNSphereField::initSceneShdr()
{
	std::string vert = "layout(std140, binding=";
	vert += std::to_string(UBO_SCENE)+") ";
	vert += "uniform sceneBuffer { SceneData scene; };\n";
	vert += "layout( std140, binding=1 ) buffer modu_pos { vec4 offs_pos[]; };\n";
	vert += "layout( std140, binding=2 ) buffer Vel { vec4 vel[]; };\n";

	vert += "in layout(location= "; vert += std::to_string(VERTEX_POS); vert += ") vec3 pos;\n";
	vert += "in layout(location= "; vert += std::to_string(VERTEX_NORMAL); vert += ") vec3 normal;\n";
	vert += "in layout(location= "; vert += std::to_string(VERTEX_COLOR); vert += ") vec4 color;\n";

	vert += STRINGIFY(
	out Interpolants {\n
		vec3 pos;\n
		vec3 normal;\n
		flat vec4 color;\n
	} OUT;\n

	uniform uint sphereIndOffs;
	uniform float sphereSize;

	void main() {\n

		uint sphereNr = gl_VertexID / sphereIndOffs;
		vec3 modPos = (pos * sphereSize * vel[sphereNr].x) + offs_pos[sphereNr].xyz;
		gl_Position = scene.viewProjMatrix * vec4(modPos,1);\n
		OUT.pos = modPos;\n
		OUT.normal = normal;\n
		OUT.color = color;\n
	});

	vert = "// SNSphereField Scene Shader vertex shader\n" + shdr_Header +com
			+ vert;

	
//----------------------------------------------------

	std::string frag = "layout(std140, binding=";
	frag += std::to_string(UBO_SCENE)+") ";
	frag += "uniform sceneBuffer { SceneData scene; };\n";

	frag += STRINGIFY(in Interpolants {
		vec3 pos;
		vec3 normal;
		flat vec4 color;
	} IN;

	layout(location=0,index=0) out vec4 out_Color;

	void main()
	{
		vec3  light = normalize(vec3(-1,2,1));
		float intensity = dot(normalize(IN.normal),light) * 0.5 + 0.5;
		vec4  color = IN.color * mix(vec4(0, 0, 0, 0),vec4(1,1,1,0),intensity);

		out_Color = color;
	});

	frag = "// SNSphereField Scene Shader vertex shader\n" + shdr_Header +com
			+ frag;

	draw_scene = shCol->addCheckShaderText("SNSphereFieldDrawScene", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNSphereField::initFullScrQuad()
{
	std::string vert = STRINGIFY(
			out vec2 texCoord;\n
			\n
			void main()	{\n
				uint idx = gl_VertexID % 3;\n // allows rendering multiple fullscreen triangles
				vec4 pos =  vec4(\n
						(float( idx     &1U)) * 4.0 - 1.0,\n
						(float((idx>>1U)&1U)) * 4.0 - 1.0,\n
						0, 1.0);\n
						gl_Position = pos;\n
						texCoord = pos.xy * 0.5 + 0.5;\n
			});

	fullScrQuad = "// SNSphereField Full Screen Quad vertex shader\n" + shdr_Header +com + vert;

	

	std::string add_shdr_header = "#extension GL_NV_geometry_shader_passthrough : enable\n";

	std::string geo = STRINGIFY(layout(triangles) in;\n
		layout(triangle_strip,max_vertices=3) out;\n

		in Inputs {
	    	vec2 texCoord;
		} IN[];
		out vec2 texCoord;

	  void main()
	  {
	    for (int i = 0; i < 3; i++){
	      texCoord = IN[i].texCoord;
	      gl_Layer = gl_PrimitiveIDIn;
	      gl_PrimitiveID = gl_PrimitiveIDIn;
	      gl_Position = gl_in[i].gl_Position;
	      EmitVertex();
	    }
	  });

	fullScrQuadGeo = "// SNSphereField Full Screen Quad geometry shader\n" + shdr_Header + add_shdr_header + com + geo;
}

//----------------------------------------------------

void SNSphereField::initBilateralblur()
{
	std::string frag = STRINGIFY(
	const float KERNEL_RADIUS = 3;\n

	layout(location=0) uniform float g_Sharpness;\n
	layout(location=1) uniform vec2  g_InvResolutionDirection; \n// either set x to 1/width or y to 1/height

	layout(binding=0) uniform sampler2D texSource;\n
	layout(binding=1) uniform sampler2D texLinearDepth;\n

	in vec2 texCoord;\n

	layout(location=0,index=0) out vec4 out_Color;\n

	vec4 BlurFunction(vec2 uv, float r, vec4 center_c, float center_d, inout float w_total)\n
	{\n
		vec4  c = texture2D( texSource, uv );\n
		float d = texture2D( texLinearDepth, uv).x;\n
		\n
		const float BlurSigma = float(KERNEL_RADIUS) * 0.5;\n
		const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);\n
		\n
		float ddiff = (d - center_d) * g_Sharpness;\n
		float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);\n
		w_total += w;
		\n
		return c*w;\n
	}\n

	void main()
	{
		vec4  center_c = texture2D( texSource, texCoord );\n
		float center_d = texture2D( texLinearDepth, texCoord).x;\n
		\n
		vec4  c_total = center_c;\n
		float w_total = 1.0;\n

		for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
		{\n
			vec2 uv = texCoord + g_InvResolutionDirection * r;\n
			c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
		}\n
		\n
		for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
		{\n
			vec2 uv = texCoord - g_InvResolutionDirection * r;\n
			c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
		}\n
		\n
		out_Color = c_total/w_total;\n
	});

	frag = "// SNSphereField Bilateralblur Shader frag shader\n" + shdr_Header + frag;

	bilateralblur = shCol->addCheckShaderText("SNSphereFieldBilateralblur", fullScrQuad.c_str(), frag.c_str());
}

//----------------------------------------------------

std::string  SNSphereField::initDepthLinearize(int msaa)
{
	std::string add_shdr_Header = "#define DEPTHLINEARIZE_MSAA "; add_shdr_Header += std::to_string(msaa); add_shdr_Header += "\n";
	add_shdr_Header += "#ifndef DEPTHLINEARIZE_USEMSAA\n#define DEPTHLINEARIZE_USEMSAA 0\n#endif\n";
	add_shdr_Header += "#if DEPTHLINEARIZE_MSAA\nlayout(location=1) uniform int sampleIndex;\nlayout(binding=0)  uniform sampler2DMS inputTexture;\n ";
	add_shdr_Header += "#else\n layout(binding=0)  uniform sampler2D inputTexture;\n#endif\n";

	std::string frag = STRINGIFY(
			layout(location=0) uniform vec4 clipInfo;\n // z_n * z_f,  z_n - z_f,  z_f, perspective = 1 : 0
			layout(location=0,index=0) out float out_Color;\n

			float reconstructCSZ(float d, vec4 clipInfo) {\n
				if (clipInfo[3] != 0) {\n
					return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));\n
				} else {\n
					return (clipInfo[1]+clipInfo[2] - d * clipInfo[1]);\n
				}\n
			}\n

			void main() {\n);

	frag += "#if DEPTHLINEARIZE_MSAA\n";
	frag += "float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), sampleIndex).x;\n";
	frag += "#else\n";
	frag += "float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), 0).x;\n";
	frag += "#endif\n";
	frag += "out_Color = reconstructCSZ(depth, clipInfo);\n}";

	frag = "// SNSphereField Depth Linearize vertex shader\n" + shdr_Header + add_shdr_Header + frag;

	return frag;
}

//----------------------------------------------------

void SNSphereField::initViewNormal()
{
	std::string frag = STRINGIFY(in vec2 texCoord;\n
	layout(location=0) uniform vec4 projInfo;\n
	layout(location=1) uniform int  projOrtho;\n
	layout(location=2) uniform vec2 InvFullResolution;\n
	layout(binding=0)  uniform sampler2D texLinearDepth;\n
	layout(location=0,index=0) out vec4 out_Color;\n
	\n
	vec3 UVToView(vec2 uv, float eye_z)\n
	{\n
		return vec3((uv * projInfo.xy + projInfo.zw) * (projOrtho != 0 ? 1.0 : eye_z), eye_z);\n
	}\n
	\n
	vec3 FetchViewPos(vec2 UV)\n
	{\n
		float ViewDepth = textureLod(texLinearDepth,UV,0).x;\n
		return UVToView(UV, ViewDepth);\n
	}\n
	\n
	vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)\n
	{\n
		vec3 V1 = Pr - P;\n
		vec3 V2 = P - Pl;\n
		return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;\n
	}\n
	\n
	vec3 ReconstructNormal(vec2 UV, vec3 P)\n
	{\n
		vec3 Pr = FetchViewPos(UV + vec2(InvFullResolution.x, 0));\n
		vec3 Pl = FetchViewPos(UV + vec2(-InvFullResolution.x, 0));\n
		vec3 Pt = FetchViewPos(UV + vec2(0, InvFullResolution.y));\n
		vec3 Pb = FetchViewPos(UV + vec2(0, -InvFullResolution.y));\n
		return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));\n
	}\n
	\n
	void main() {
		vec3 P  = FetchViewPos(texCoord);\n
		vec3 N  = ReconstructNormal(texCoord, P);\n
		\n
		out_Color = vec4(N*0.5 + 0.5,0);\n
	});
//----------------------------------------------------

	frag = "// SNSphereField View Normal vertex shader\n" + shdr_Header
			+ frag;

	viewnormal = shCol->addCheckShaderText("SNSphereFieldViewNormal", fullScrQuad.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNSphereField::initDisplayTex()
{
	std::string frag = STRINGIFY(
	layout(binding=0) uniform sampler2D inputTexture;
	layout(location=0,index=0) out vec4 out_Color;
	in vec2 texCoord;
	void main()	{
		out_Color = texture(inputTexture, texCoord);
	});

	frag = "// SNSphereField DisplayTex shader\n" + shdr_Header
			+ frag;

	displaytex = shCol->addCheckShaderText("SNSphereFieldDisplaytex", fullScrQuad.c_str(), frag.c_str());
}

//----------------------------------------------------

std::string SNSphereField::initHbaoCalc(int blur, int deinterl)
{
	std::string add_shdr_Header = "#define AO_DEINTERLEAVED " + std::to_string(deinterl) + "\n";
	add_shdr_Header += "#define AO_BLUR " + std::to_string(blur) + "\n";
	// The pragma below is critical for optimal performance
	// in this fragment shader to let the shader compiler
	// fully optimize the maths and batch the texture fetches
	// optimally
	add_shdr_Header += "#pragma optionNV(unroll all) \n#ifndef AO_DEINTERLEAVED\n#define AO_DEINTERLEAVED 1\n#endif\n";
	add_shdr_Header += "#ifndef AO_BLUR\n#define AO_BLUR 1\n#endif\n#ifndef AO_LAYERED\n#define AO_LAYERED 1\n#endif\n#define M_PI 3.14159265f\n";
//----------------------------------------------------

	// tweakables
	std::string frag = STRINGIFY(
			const float  NUM_STEPS = 4;\n
			const float  NUM_DIRECTIONS = 8;\n // texRandom/g_Jitter initialization depends on this
			layout(std140,binding=0) uniform controlBuffer { HBAOData control; };\n
		);

		frag += "#if AO_DEINTERLEAVED\n";

		frag += "#if AO_LAYERED\n";
			frag += STRINGIFY(vec2 g_Float2Offset = control.float2Offsets[gl_PrimitiveID].xy;\n
			vec4 g_Jitter       = control.jitters[gl_PrimitiveID];\n

			layout(binding=0) uniform sampler2DArray texLinearDepth;\n
			layout(binding=1) uniform sampler2D texViewNormal;\n

			vec3 getQuarterCoord(vec2 UV){\n
				return vec3(UV,float(gl_PrimitiveID));\n
			}\n);
		frag += "#if AO_LAYERED == 1\n";
		frag += "#if AO_BLUR\n";
			frag += "layout(binding=0,rg16f) uniform image2DArray imgOutput;\n";
		frag += "#else\n";
			frag += "layout(binding=0,r8) uniform image2DArray imgOutput;\n";
		frag += "#endif\n";
			frag += STRINGIFY(void outputColor(vec4 color) {\n
				imageStore(imgOutput, ivec3(ivec2(gl_FragCoord.xy),gl_PrimitiveID), color);\n
			}\n);
			frag += "#else\n";
			frag += STRINGIFY(layout(location=0,index=0) out vec4 out_Color;\n
			\n
			void outputColor(vec4 color) {\n
				out_Color = color;\n
			}\n);
frag += "#endif\n";
frag += "#else\n";
frag += STRINGIFY(layout(location=0) uniform vec2 g_Float2Offset;\n
			layout(location=1) uniform vec4 g_Jitter;\n
			\n
			layout(binding=0) uniform sampler2D texLinearDepth;\n
			layout(binding=1) uniform sampler2D texViewNormal;\n
			\n
			vec2 getQuarterCoord(vec2 UV){\n
				return UV;\n
			}\n

			layout(location=0,index=0) out vec4 out_Color;\n

			void outputColor(vec4 color) {\n
				out_Color = color;\n
			}\n);
frag += "#endif\n";

frag += "#else\n";
frag += STRINGIFY(layout(binding=0) uniform sampler2D texLinearDepth;\n
			layout(binding=1) uniform sampler2D texRandom;\n
			\n
			layout(location=0,index=0) out vec4 out_Color;\n
			\n
			void outputColor(vec4 color) {\n
				out_Color = color;\n
			}\n);
frag += "#endif\n";
			frag += STRINGIFY(in vec2 texCoord;\n
			\n
			vec3 UVToView(vec2 uv, float eye_z)	{\n
				return vec3((uv * control.projInfo.xy + control.projInfo.zw) * (control.projOrtho != 0 ? 1. : eye_z), eye_z);\n
			}\n);

frag += "#if AO_DEINTERLEAVED\n";

			frag += STRINGIFY(vec3 FetchQuarterResViewPos(vec2 UV) {\n
				float ViewDepth = textureLod(texLinearDepth,getQuarterCoord(UV),0).x;\n
				return UVToView(UV, ViewDepth);\n
			}\n);

frag += "#else //AO_DEINTERLEAVED\n";

			frag += STRINGIFY(vec3 FetchViewPos(vec2 UV) {\n
				float ViewDepth = textureLod(texLinearDepth,UV,0).x;\n
				return UVToView(UV, ViewDepth);\n
			}\n

			vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl) {\n
				vec3 V1 = Pr - P;\n
				vec3 V2 = P - Pl;\n
				return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;\n
			}\n

			vec3 ReconstructNormal(vec2 UV, vec3 P)	{\n
				vec3 Pr = FetchViewPos(UV + vec2(control.InvFullResolution.x, 0));\n
				vec3 Pl = FetchViewPos(UV + vec2(-control.InvFullResolution.x, 0));\n
				vec3 Pt = FetchViewPos(UV + vec2(0, control.InvFullResolution.y));\n
				vec3 Pb = FetchViewPos(UV + vec2(0, -control.InvFullResolution.y));\n
				return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));\n
			}\n);

frag += "#endif //AO_DEINTERLEAVED\n";

			//------------------------------------------------------------

			frag += STRINGIFY(float Falloff(float DistanceSquare)	{\n
				// 1 scalar mad instruction
				return DistanceSquare * control.NegInvR2 + 1.0;\n
			}\n

			//------------------------------------------------------------

			// P = view-space position at the kernel center
			// N = view-space normal at the kernel center
			// S = view-space position of the current sample
			//------------------------------------------------------------

			float ComputeAO(vec3 P, vec3 N, vec3 S)	{\n
				vec3 V = S - P;\n
				float VdotV = dot(V, V);\n
				float NdotV = dot(N, V) * 1.0/sqrt(VdotV);\n

				// Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
				return clamp(NdotV - control.NDotVBias,0,1) * clamp(Falloff(VdotV),0,1);\n
			}\n

			//------------------------------------------------------------

			vec2 RotateDirection(vec2 Dir, vec2 CosSin)	{\n
				return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,\n
						Dir.x*CosSin.y + Dir.y*CosSin.x);\n
			}\n

			//------------------------------------------------------------
			vec4 GetJitter()	{\n);
frag += "#if AO_DEINTERLEAVED\n";
				// Get the current jitter vector from the per-pass constant buffer
			frag += "return g_Jitter;\n";
frag += "#else\n";
				// (cos(Alpha),sin(Alpha),rand1,rand2)
			frag += "return textureLod( texRandom, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE), 0);\n";
frag += "#endif\n}";

			//------------------------------------------------------------
			frag += "float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal) {\n";
			frag += "#if AO_DEINTERLEAVED\n";
			frag += "RadiusPixels /= 4.0;\n";
			frag += "#endif\n";

				// Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
			frag += STRINGIFY(float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);\n

				const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;\n
				float AO = 0;\n

				for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)		{\n
					float Angle = Alpha * DirectionIndex;\n

					// Compute normalized 2D direction
					vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);\n

					// Jitter starting sample within the first step
					float RayPixels = (Rand.z * StepSizePixels + 1.0);\n

					for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex) {\n);
frag += "#if AO_DEINTERLEAVED\n";
					frag += STRINGIFY(vec2 SnappedUV = round(RayPixels * Direction) * control.InvQuarterResolution + FullResUV;\n
						vec3 S = FetchQuarterResViewPos(SnappedUV);\n);
frag += "#else\n";
						frag += STRINGIFY(vec2 SnappedUV = round(RayPixels * Direction) * control.InvFullResolution + FullResUV;\n
						vec3 S = FetchViewPos(SnappedUV);\n);
frag += "#endif\n";
						frag += STRINGIFY(RayPixels += StepSizePixels;\n

						AO += ComputeAO(ViewPosition, ViewNormal, S);\n
					}\n
				}\n

				AO *= control.AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);\n
				return clamp(1.0 - AO * 2.0,0,1);\n
			}\n

			//------------------------------------------------------------

			void main()\n
			{\n);

frag += "#if AO_DEINTERLEAVED\n";
			frag += STRINGIFY(vec2 base = floor(gl_FragCoord.xy) * 4.0 + g_Float2Offset;\n
				vec2 uv = base * (control.InvQuarterResolution / 4.0);\n

				vec3 ViewPosition = FetchQuarterResViewPos(uv);\n
				vec4 NormalAndAO =  texelFetch( texViewNormal, ivec2(base), 0);\n
				vec3 ViewNormal =  -(NormalAndAO.xyz * 2.0 - 1.0);\n);
frag += "#else\n";
				frag += STRINGIFY(vec2 uv = texCoord;\n
				vec3 ViewPosition = FetchViewPos(uv);\n\n

				// Reconstruct view-space normal from nearest neighbors
				vec3 ViewNormal = -ReconstructNormal(uv, ViewPosition);\n);
frag += "#endif\n";

				// Compute projection of disk of radius control.R into screen space
				frag += STRINGIFY(float RadiusPixels = control.RadiusToScreen / (control.projOrtho != 0 ? 1.0 : ViewPosition.z);\n

				// Get jitter vector for the current full-res pixel
				vec4 Rand = GetJitter();\n

				float AO = ComputeCoarseAO(uv, RadiusPixels, Rand, ViewPosition, ViewNormal);\n);

frag += "#if AO_BLUR\n";
				frag += "outputColor(vec4(pow(AO, control.PowExponent), ViewPosition.z, 0, 0));\n";
frag += "#else\n";
				frag += "outputColor(vec4(pow(AO, control.PowExponent)));\n";
				frag += "#endif\n}";

	frag = "//SNSphereField HBAO Calc \n" + shdr_Header + add_shdr_Header + com + frag;

	return frag;
}

//----------------------------------------------------

std::string  SNSphereField::initHbaoBlur(int blur)
{
	std::string add_shdr_Header = "#define AO_BLUR_PRESENT " + std::to_string(blur) +  "\n";

	std::string frag = STRINGIFY(
	const float KERNEL_RADIUS = 3;\n
	layout(location=0) uniform float g_Sharpness;\n
	layout(location=1) uniform vec2  g_InvResolutionDirection;\n // either set x to 1/width or y to 1/height
	layout(binding=0) uniform sampler2D texSource;\n
	in vec2 texCoord;\n
	layout(location=0,index=0) out vec4 out_Color;\n
	);

	frag += "#ifndef AO_BLUR_PRESENT\n";
	frag += "#define AO_BLUR_PRESENT 1\n";
	frag += "#endif\n";

	//------------------------------------------------------------
//----------------------------------------------------

	frag += STRINGIFY(float BlurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total)
	{\n
		vec2  aoz = texture2D( texSource, uv ).xy;\n
		float c = aoz.x;\n
		float d = aoz.y;\n

		const float BlurSigma = float(KERNEL_RADIUS) * 0.5;\n
		const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);\n

		float ddiff = (d - center_d) * g_Sharpness;\n
		float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);\n
		w_total += w;\n

		return c*w;\n
	}\n

	void main()\n
	{
		vec2  aoz = texture2D( texSource, texCoord ).xy;\n
		float center_c = aoz.x;\n
		float center_d = aoz.y;\n

		float c_total = center_c;\n
		float w_total = 1.0;\n

		for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
		{
			vec2 uv = texCoord + g_InvResolutionDirection * r;\n
			c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
		}

		for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
		{
			vec2 uv = texCoord - g_InvResolutionDirection * r;\n
			c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
		}\n);

		frag += "#if AO_BLUR_PRESENT\n";
		frag += "out_Color = vec4(c_total/w_total);\n";
		frag += "#else\n";
		frag += "out_Color = vec4(c_total/w_total, center_d, 0, 0);\n";
		frag += "#endif\n}";

	frag = "// SNSphereField HBAO_Calc Shader frag shader\n" + shdr_Header + add_shdr_Header + frag;

	return frag;
}

//----------------------------------------------------

void SNSphereField::initDeinterleave()
{
	std::string frag = STRINGIFY(
	layout(location=0) uniform vec4 info; // xy
	vec2 uvOffset = info.xy;
	vec2 invResolution = info.zw;

	layout(binding=0)  uniform sampler2D texLinearDepth;
	layout(location=0,index=0) out float out_Color[8];\n

	void main() {
		vec2 uv = floor(gl_FragCoord.xy) * 4.0 + uvOffset + 0.5;
		uv *= invResolution;

		vec4 S0 = textureGather(texLinearDepth, uv, 0);
		vec4 S1 = textureGatherOffset(texLinearDepth, uv, ivec2(2,0), 0);

		out_Color[0] = S0.w;
		out_Color[1] = S0.z;
		out_Color[2] = S1.w;
		out_Color[3] = S1.z;
		out_Color[4] = S0.x;
		out_Color[5] = S0.y;
		out_Color[6] = S1.x;
		out_Color[7] = S1.y;
	});

	/*
	frag += "#else\n";

	frag += STRINGIFY(void main() {
		vec2 uv = floor(gl_FragCoord.xy) * 4.0 + uvOffset;
		ivec2 tc = ivec2(uv);

		out_Color[0] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(0,0)).x;
		out_Color[1] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(1,0)).x;
		out_Color[2] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(2,0)).x;
		out_Color[3] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(3,0)).x;
		out_Color[4] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(0,1)).x;
		out_Color[5] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(1,1)).x;
		out_Color[6] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(2,1)).x;
		out_Color[7] = texelFetchOffset(texLinearDepth, tc, 0, ivec2(3,1)).x;
	});

	frag += "#endif\n}";
	*/

	frag = "// SNSphereField Deinterleave shader\n" + shdr_Header + frag;

	hbao2_deinterleave = shCol->addCheckShaderText("SNSphereFieldHBAO2_deinterleave", fullScrQuad.c_str(), frag.c_str());
}

//----------------------------------------------------

std::string  SNSphereField::initReinterleave(int blur)
{
	std::string add_shdr_Header = "#define AO_BLUR " + std::to_string(blur) + "\n";

	std::string frag = STRINGIFY(
	layout(binding=0)  uniform sampler2DArray texResultsArray;
	layout(location=0,index=0) out vec4 out_Color;

	void main() {
		ivec2 FullResPos = ivec2(gl_FragCoord.xy);
		ivec2 Offset = FullResPos & 3;
		int SliceId = Offset.y * 4 + Offset.x;
		ivec2 QuarterResPos = FullResPos >> 2;\n
	);

	frag += "#if AO_BLUR\n";
	frag += "out_Color = vec4(texelFetch( texResultsArray, ivec3(QuarterResPos, SliceId), 0).xy,0,0);\n";
	frag += "#else\n";
	frag += "out_Color = vec4(texelFetch( texResultsArray, ivec3(QuarterResPos, SliceId), 0).x);\n";
	frag += "#endif\n}";

	frag = "// SNSphereField Reinterleave Shader frag shader\n" + shdr_Header + add_shdr_Header + frag;

	return frag;
}
//----------------------------------------------------


std::string SNSphereField::initComputeUpdtShdr()
{
	// version statement kommt von aussen
	std::string shdr_Header = "#define WORK_GROUP_SIZE 128\n";

	std::string src = STRINGIFY(
	uniform sampler2D fluid;\n
	uniform sampler3D noiseTex3D;\n
	uniform uint totNumParticles;\n
	uniform vec3 numPart;\n
	uniform float time;\n
	uniform float fluidForce;\n
	uniform float perlForce;\n
	//uniform float invNoiseSize;\n
	//uniform vec3 noiseStrength;\n
	uniform float randAmt;\n

	layout( std140, binding=1 ) buffer Modu_pos { vec4 offs_pos[];\n };\n
	layout( std140, binding=2 ) buffer Ref_pos { vec4 r_pos[];\n };\n
	layout( std140, binding=3 ) buffer Vel { vec4 vel[];\n };\n

	layout(local_size_x = 128,  local_size_y = 1, local_size_z = 1) in;\n

	/*
	// noise functions
	// returns random value in [-1, 1]
	vec3 noise3f(vec3 p) {
		return (texture(noiseTex3D, p * invNoiseSize + offsCo).xyz - vec3(0.25)) * 4.0;
	}

	// fractal sum
	vec3 fBm3f(vec3 p, int octaves, float lacunarity, float gain) {\n
		float freq = 1.0;
		float amp = 0.5;
		vec3 sum = vec3(0.0);

		for(int i=0; i<octaves; i++) {
			sum += noise3f(p*freq)*amp;
			freq *= lacunarity;
			amp *= gain;
		}
		return sum;
	}
*/

	// compute shader to update particles
	void main() {

		uint i = gl_GlobalInvocationID.x;

		// thread block size may not be exact multiple of number of particles
		if (i >= totNumParticles) return;

		vec3 normPos = vec3(mod(float(i), numPart.x) / numPart.x,
							mod(float(i) / numPart.x, numPart.y) / numPart.y,
							mod(float(i) / (numPart.x * numPart.y), numPart.z) / numPart.z);

		vec4 fluidVel = texture(fluid, normPos.xy);

		// read particle position and velocity from buffers
		vec3 offsCo = vec3(normPos.x + time * 0.2,
						   normPos.y + time * 0.23,
						   normPos.z * 0.8 + time * 0.12);

		vec4 perlOffs = texture(noiseTex3D, offsCo);
		perlOffs -= vec4(0.25, 0.25, 0.25, 0); // -0.25 bis 0.25
		perlOffs.z *=  10.0;

		// read particle position and velocity from buffers
		vec3 p = r_pos[i].xyz + fluidVel.xyz * fluidForce + perlOffs.xyz * perlForce;\n

		//vec3 v = vel[i].xyz;\n

		// write new values
		offs_pos[i] = vec4(p, 1.0);\n
		vel[i].x = length(fluidVel.xz)* 0.12 + 0.9;\n

		// write new values
		offs_pos[i] = vec4(mix(r_pos[i].xyz, offs_pos[i].xyz, randAmt), 0.0);\n
	});

	src = shdr_Header + src;

	return src;
}

//----------------------------------------------------

GLuint SNSphereField::createShaderPipelineProgram(GLuint target, const char* src)
{
	GLuint object;
	GLint status;
	std::string header = "#version 430\n";

    glUseProgram(0); // MAKE sure no shader is active;

	glGenProgramPipelines(1, &m_programPipeline);

	const GLchar* fullSrc[2] = { header.c_str(), src };
	object = glCreateShaderProgramv( target, 2, fullSrc); // with this command GL_PROGRAM_SEPARABLE is set to true												// and a program object is generated and returned

	{
		GLint logLength;
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
	    char *log = new char [logLength];
	    glGetProgramInfoLog(object, logLength, 0, log);
	    printf("%s\n", log);
	    delete [] log;
	}

	glBindProgramPipeline(m_programPipeline);
	glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, object);

	std::cout << "glValidateProgramPipeline " << std::endl;
	glValidateProgramPipeline(m_programPipeline);
	getGlError();

	std::cout << "glGetProgramPipelineiv " << std::endl;
	glGetProgramPipelineiv(m_programPipeline, GL_VALIDATE_STATUS, &status);
	getGlError();

	std::cout << "check status: " << status <<std::endl;

	if (status != GL_TRUE) {
		GLint logLength;

		std::cout << "glGetProgramPipelineiv " << std::endl;
		glGetProgramPipelineiv(m_programPipeline, GL_INFO_LOG_LENGTH, &logLength);
		getGlError();
//----------------------------------------------------

		char *log = new char [logLength];

		std::cout << "glGetProgramPipelineInfoLog " << std::endl;
		glGetProgramPipelineInfoLog(m_programPipeline, logLength, 0, log);
		getGlError();

		printf("SNSphereField::createShaderPipelineProgram Error: Shader pipeline not valid:\n%s\n", log);
		delete [] log;
	}
	getGlError();

	glBindProgramPipeline(0);

	getGlError();

	return object;
}

//----------------------------------------------------

void SNSphereField::updateOffsPos(double time)
{
	// deactivated any shader actually running
	glUseProgram(0);

	// Invoke the compute shader to integrate the particles
	//glUseProgram(m_updateProg);
	glBindProgramPipeline(m_programPipeline);
//----------------------------------------------------

	GLint loc = glGetUniformLocation(m_updateProg, "time");
	glProgramUniform1f(m_updateProg, loc, float(time * timeScale));

	loc = glGetUniformLocation(m_updateProg, "totNumParticles");
	glProgramUniform1ui(m_updateProg, loc, gridX * gridY * gridZ);

	loc = glGetUniformLocation(m_updateProg, "numPart");
	glProgramUniform3f(m_updateProg, loc, float(gridX), float(gridY), float(gridZ));

	loc = glGetUniformLocation(m_updateProg, "noiseTex3D");
	glProgramUniform1i(m_updateProg, loc, 0);

	loc = glGetUniformLocation(m_updateProg, "fluid");
	glProgramUniform1i(m_updateProg, loc, 1);

	loc = glGetUniformLocation(m_updateProg, "fluidForce");
	glProgramUniform1f(m_updateProg, loc, fluidForce);

	loc = glGetUniformLocation(m_updateProg, "perlForce");
	glProgramUniform1f(m_updateProg, loc,  perlForce);
//
//	glm::vec3 noisStr = glm::vec3(noiseStr"));
//	loc = glGetUniformLocation(m_updateProg, "noiseStrength");
//	glProgramUniform3f(m_updateProg, loc, noisStr.x, noisStr.y, noisStr.z);
//
//	loc = glGetUniformLocation(m_updateProg, "invNoiseSize");
//	glProgramUniform1f(m_updateProg, loc, 1.0f / noiseSize"));

	loc = glGetUniformLocation(m_updateProg, "randAmt");
	glProgramUniform1f(m_updateProg, loc, randAmt);
//----------------------------------------------------

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex() );

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  modu_pos->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  ref_pos->getBuffer() );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  m_vel->getBuffer() );

	// workgroup size manually set to 128, dirty... must be the same as inside shader
	glDispatchCompute( (gridX*gridY*gridZ) / 128 +1, 1,  1 );

	// We need to block here on compute completion to ensure that the
	// computation is done before we render
	glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2,  0 );
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 3,  0 );
	glBindProgramPipeline(0);
}

//----------------------------------------------------

void SNSphereField::udpateLiquid(double time)
{
	  float tm = static_cast<float>(std::sin(time * oscSpeed)) * oscAmt;

		// Adding temporal Force, in pixel relative to flWidth and flHeight
		// range -0.5 | 0.5
		glm::vec2 perlOffs = glm::vec2(
				glm::perlin( glm::vec4(time * posRandSpd, 0.2f, 0.21f, 0.f) ),
				glm::perlin( glm::vec4(time * 1.8f * posRandSpd, 0.4f, 0.f, 0.f) ) ) * 2.f;

		oldPos = forcePos;
		forcePos = glm::vec2( getRawOscPar("xPos"), tm );
		forcePos += glm::vec2(posRandAmtX * perlOffs.x,
							  posRandAmtY * perlOffs.y);

		glm::vec2 d = (oldPos - forcePos) * velScale;
		glm::vec4 forceCol = glm::vec4(1.f, 1.f, 1.f, 1.f);

	    fluidSim->setTimeStep( timeStep );

		//std::cout << "forcePos " << glm::to_string(forcePos) << std::endl;
		//std::cout << "d " << glm::to_string(d) << std::endl;

		fluidSim->addTemporalForce(toFluidCoord(forcePos),                  	// pos
									d,			 // vel
									forceCol, // col
									rad);							// rad
		fluidSim->update();
		//fluidSim->drawVelocity();
}
//----------------------------------------------------


glm::vec2 SNSphereField::toFluidCoord(glm::vec2 normPos)
{
	// convert from -1 | 1 to 0 | 5
	glm::vec2 out = normPos * 0.5f + 0.5f;
	// scale with fluidSize
	out *= fluidSize;
	return out;
}
//----------------------------------------------------


bool SNSphereField::initMisc()
{
	MTRand rng;

	float numDir = 8; // keep in sync to glsl

	rng.seed((unsigned)0);

	signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS*MAX_SAMPLES*4];

	for(int i=0; i<HBAO_RANDOM_ELEMENTS*MAX_SAMPLES; i++)
	{
		float Rand1 = rng.randExc();
		float Rand2 = rng.randExc();

		// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
		float Angle = 2.f * M_PI * Rand1 / numDir;
		hbaoRandom[i].x = cosf(Angle);
		hbaoRandom[i].y = sinf(Angle);
		hbaoRandom[i].z = Rand2;
		hbaoRandom[i].w = 0;
#define SCALE ((1<<15))
		hbaoRandomShort[i*4+0] = (signed short)(SCALE*hbaoRandom[i].x);
		hbaoRandomShort[i*4+1] = (signed short)(SCALE*hbaoRandom[i].y);
		hbaoRandomShort[i*4+2] = (signed short)(SCALE*hbaoRandom[i].z);
		hbaoRandomShort[i*4+3] = (signed short)(SCALE*hbaoRandom[i].w);
#undef SCALE
	}

	newTexture(textures.hbao_random);
	glBindTexture(GL_TEXTURE_2D_ARRAY,textures.hbao_random);
	glTexStorage3D (GL_TEXTURE_2D_ARRAY,1,GL_RGBA16_SNORM,HBAO_RANDOM_SIZE,HBAO_RANDOM_SIZE,MAX_SAMPLES);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,0, HBAO_RANDOM_SIZE,HBAO_RANDOM_SIZE,MAX_SAMPLES,GL_RGBA,GL_SHORT,hbaoRandomShort);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY,0);

	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		newTexture(textures.hbao_randomview[i]);
		glTextureView(textures.hbao_randomview[i], GL_TEXTURE_2D, textures.hbao_random, GL_RGBA16_SNORM, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, textures.hbao_randomview[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	newBuffer(buffers.hbao_ubo);
	glNamedBufferStorageEXT(buffers.hbao_ubo, sizeof(HBAOData), NULL, GL_DYNAMIC_STORAGE_BIT);

	return true;
}
//----------------------------------------------------


// bau die geometrie zusammen
bool SNSphereField::initScene()
{
	// Shader Storage for Position manipulation
	modu_pos = new ShaderBuffer<glm::vec4>(gridX * gridY * gridZ);
	glm::vec4 *pos = modu_pos->map();
	for(size_t i=0; i<gridX * gridY * gridZ; i++)
		pos[i] = glm::vec4(0.f, 0.f, 0.f, 0.f);
	modu_pos->unmap();

	m_vel = new ShaderBuffer<glm::vec4>(gridX * gridY * gridZ);
	glm::vec4 *vel = m_vel->map();
	for(size_t i=0; i<gridX * gridY * gridZ; i++) {
		vel[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	m_vel->unmap();

	// Shader Storage for Position manipulation
	ref_pos = new ShaderBuffer<glm::vec4>(gridX * gridY * gridZ);
	glm::vec4 *ref_pos_ptr = ref_pos->map();
//	for(size_t i=0; i<gridX * gridY * gridZ; i++)

	// Scene Geometry
	tav::geometry::Mesh<tav::Vertex>  scene;

	int sphereDimWH = 10;
	int sphereDimZ = 10;

	sceneObjects = 0;

	for (int z=0; z<gridZ; z++)
	{
		for (int y=0; y<gridY; y++)
		{
			for (int x=0; x<gridX; x++)
			{
				int colInd = static_cast<int>(getRandF(0.f, 5.2f));
				glm::vec4 color = chanCols[colInd];
				color *= 0.8f;
				color += 0.3f;

				color = glm::saturation(0.8f, color);

				//std::cout << "colInd: " << colInd <<  glm::to_string(color) << std::endl;

				// groesse entspricht einem grid
				glm::vec3 size = glm::vec3(globalscale * 0.5f / float(gridY),
										   globalscale * 0.5f / float(gridY),
										   globalscale * 0.5f / float(gridY));
				//globalscale * 0.5f / float(gridY) / float(sphereDimZ));

				// position in grid einheiten
				glm::vec3 pos(float(x), float(y), float(-z) * 10.f);

				// verschiebe ein halbes grids nach links unten
//				pos -=  glm::vec3( gridX/2, gridY/2, gridY /2 / float(sphereDimZ));
				pos -=  glm::vec3( gridX/2, gridY/2, gridY/2  );

				// skaliere die position auf die definierte gesamt groesse des grids
				pos.x /=  float(gridX) / globalscale;
				pos.x *= propo;
				pos.y /=  float(gridY) / globalscale;
				pos.z /=  float(gridY) / globalscale;

				ref_pos_ptr[z*gridY*gridX + y*gridX +x] = glm::vec4(pos.x, pos.y, pos.z, 0.f);

				glm::mat4  matrix    = glm::mat4(1.f);

//				glm::mat4  matrix    = glm::translate(glm::mat4(1.f), pos);
				matrix = glm::scale(matrix, size);

				uint  oldverts  = scene.getVerticesCount();
				uint  oldinds   = scene.getTriangleIndicesCount();


				tav::geometry::Sphere<tav::Vertex>::add(scene, matrix, sphereDimWH, sphereDimZ);
		//		tav::geometry::Box<tav::Vertex>::add(scene,matrix,2,2,2);

				if(x==0 && y==0) sphereIndOffs = scene.getVerticesCount() - oldverts;

				for (uint v = oldverts; v < scene.getVerticesCount(); v++){
					scene.m_vertices[v].color = color;
				}
			}
		}

		sceneObjects++;
	}

	ref_pos->unmap();

	sceneTriangleIndices = scene.getTriangleIndicesCount();

	//----------------------------------------------------
	// default vao sollte noch gebunden sein sonst gl error

	newBuffer(buffers.scene_ibo);
	glNamedBufferStorageEXT(buffers.scene_ibo, scene.getTriangleIndicesSize(), &scene.m_indicesTriangles[0], 0);

	newBuffer(buffers.scene_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, buffers.scene_vbo);
	glNamedBufferStorageEXT(buffers.scene_vbo, scene.getVerticesSize(), &scene.m_vertices[0], 0);

	glVertexAttribFormat(VERTEX_COLOR,  4, GL_FLOAT, GL_FALSE,  offsetof(tav::Vertex, color));
	glVertexAttribBinding(VERTEX_COLOR, 0);
	glVertexAttribFormat(VERTEX_POS,    3, GL_FLOAT, GL_FALSE,  offsetof(tav::Vertex, position));
	glVertexAttribBinding(VERTEX_POS,   0);
	glVertexAttribFormat(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE,  offsetof(tav::Vertex, normal));
	glVertexAttribBinding(VERTEX_NORMAL,0);

	// Scene UBO
	newBuffer(buffers.scene_ubo);
	glNamedBufferStorageEXT(buffers.scene_ubo, sizeof(SceneData), NULL, GL_DYNAMIC_STORAGE_BIT);

	return true;
}
//----------------------------------------------------


bool SNSphereField::initFramebuffers(int width, int height, int samples)
{
	if (samples > 1){
		newTexture(textures.scene_color);
		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, textures.scene_color);
		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height, GL_FALSE);
		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, 0);

		newTexture(textures.scene_depthstencil);
		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, textures.scene_depthstencil);
		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH24_STENCIL8, width, height, GL_FALSE);
		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
	else
	{
		newTexture(textures.scene_color);
		glBindTexture (GL_TEXTURE_2D, textures.scene_color);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
		glBindTexture (GL_TEXTURE_2D, 0);

		newTexture(textures.scene_depthstencil);
		glBindTexture (GL_TEXTURE_2D, textures.scene_depthstencil);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, width, height);
		glBindTexture (GL_TEXTURE_2D, 0);
	}

	std::cout << "init fbo scene" << std::endl;

	newFramebuffer(fbos.scene);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER,     fbos.scene);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,        textures.scene_color, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, textures.scene_depthstencil, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	newTexture(textures.scene_depthlinear);
	glBindTexture (GL_TEXTURE_2D, textures.scene_depthlinear);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glBindTexture (GL_TEXTURE_2D, 0);

	std::cout << "init fbo depthlinear" << std::endl;
	newFramebuffer(fbos.depthlinear);
	glBindFramebuffer(GL_FRAMEBUFFER,     fbos.depthlinear);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,  textures.scene_depthlinear, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	newTexture(textures.scene_viewnormal);
	glBindTexture (GL_TEXTURE_2D, textures.scene_viewnormal);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glBindTexture (GL_TEXTURE_2D, 0);

	std::cout << "init fbo viewnormal" << std::endl;
	newFramebuffer(fbos.viewnormal);
	glBindFramebuffer(GL_FRAMEBUFFER,     fbos.viewnormal);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,  textures.scene_viewnormal, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	// hbao
#if USE_AO_SPECIALBLUR
	GLenum formatAO = GL_RG16F;
	GLint swizzle[4] = {GL_RED,GL_GREEN,GL_ZERO,GL_ZERO};
#else
	GLenum formatAO = GL_R8;
	GLint swizzle[4] = {GL_RED,GL_RED,GL_RED,GL_RED};
#endif

	newTexture(textures.hbao_result);
	glBindTexture (GL_TEXTURE_2D, textures.hbao_result);
	glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture (GL_TEXTURE_2D, 0);

	newTexture(textures.hbao_blur);
	glBindTexture (GL_TEXTURE_2D, textures.hbao_blur);
	glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture (GL_TEXTURE_2D, 0);

	std::cout << "init fbo hbao_calc" << std::endl;
	newFramebuffer(fbos.hbao_calc);
	glBindFramebuffer(GL_FRAMEBUFFER,     fbos.hbao_calc);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures.hbao_result, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, textures.hbao_blur, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	// interleaved hbao
	std::cout << "init fbo interleaved hbao" << std::endl;

	int quarterWidth  = ((width+3)/4);
	int quarterHeight = ((height+3)/4);

	newTexture(textures.hbao2_deptharray);
	glBindTexture (GL_TEXTURE_2D_ARRAY, textures.hbao2_deptharray);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, quarterWidth, quarterHeight, HBAO_RANDOM_ELEMENTS);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture (GL_TEXTURE_2D_ARRAY, 0);

	for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++){
		newTexture(textures.hbao2_depthview[i]);
		glTextureView(textures.hbao2_depthview[i], GL_TEXTURE_2D, textures.hbao2_deptharray, GL_R32F, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, textures.hbao2_depthview[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	//----------------------------------------------------

	newTexture(textures.hbao2_resultarray);
	glBindTexture (GL_TEXTURE_2D_ARRAY, textures.hbao2_resultarray);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, formatAO, quarterWidth, quarterHeight, HBAO_RANDOM_ELEMENTS);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture (GL_TEXTURE_2D_ARRAY, 0);

	//----------------------------------------------------

	GLenum drawbuffers[NUM_MRT];
	for (int layer = 0; layer < NUM_MRT; layer++){
		drawbuffers[layer] = GL_COLOR_ATTACHMENT0 + layer;
	}

	std::cout << "init fbo hbao2_deinterleave" << std::endl;

	newFramebuffer(fbos.hbao2_deinterleave);
	//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER,fbos.hbao2_deinterleave);
	glDrawBuffers(NUM_MRT,drawbuffers);
	glBindFramebuffer(GL_FRAMEBUFFER,lastBoundFbo);
	checkFboStatus();

	//----------------------------------------------------

	std::cout << "init fbo hbao2_calc" << std::endl;
	newFramebuffer(fbos.hbao2_calc);
	getGlError();

	std::cout << "bind framebuffer" << std::endl;
	//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
	getGlError();

#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
		// this fbo will not have any attachments and therefore requires rasterizer to be configured
	// through default parameters
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH,  quarterWidth);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, quarterHeight);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures.hbao2_resultarray, 0);

#endif
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
	std::cout << "layered gs, framebuffer texture" << std::endl;
	std::cout << "textures.hbao2_resultarray: " << textures.hbao2_resultarray << std::endl;
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures.hbao2_resultarray, 0);
	getGlError();
#endif
	checkFboStatus();

	glBindFramebuffer(GL_FRAMEBUFFER,lastBoundFbo);

	return true;
}

//----------------------------------------------------

void SNSphereField::prepareHbaoData(const Projection& projection, int width, int height)
{
	// projection
	const float* P = &projection.matrix[0][0];

	glm::vec4 projInfoPerspective = glm::vec4(
			2.0f / (P[4*0+0]),       // (x) * (R - L)/N
			2.0f / (P[4*1+1]),       // (y) * (T - B)/N
			-( 1.0f - P[4*2+0]) / P[4*0+0], // L/N
			-( 1.0f + P[4*2+1]) / P[4*1+1] // B/N
	);

	glm::vec4 projInfoOrtho = glm::vec4(
			2.0f / ( P[4*0+0]),      // ((x) * R - L)
			2.0f / ( P[4*1+1]),      // ((y) * T - B)
			-( 1.0f + P[4*3+0]) / P[4*0+0], // L
			-( 1.0f - P[4*3+1]) / P[4*1+1] // B
	);

	int useOrtho = projection.ortho ? 1 : 0;
	hbaoUbo.projOrtho = useOrtho;
	hbaoUbo.projInfo  = useOrtho ? projInfoOrtho : projInfoPerspective;

	float projScale;
	if (useOrtho){
		projScale = float(height) / (projInfoOrtho[1]);
	}
	else {
		projScale = float(height) / (tanf( projection.fov * 0.5f) * 2.0f);
	}

	// radius
	float meters2viewspace = 1.0f;
	float R = tweak.radius * meters2viewspace;
	hbaoUbo.R2 = R * R;
	hbaoUbo.NegInvR2 = -1.0f / hbaoUbo.R2;
	hbaoUbo.RadiusToScreen = R * 0.5f * projScale;

	// ao
	hbaoUbo.PowExponent = std::max(tweak.intensity,0.0f);
	hbaoUbo.NDotVBias = std::min(std::max(0.0f, tweak.bias),1.0f);
	hbaoUbo.AOMultiplier = 1.0f / (1.0f - hbaoUbo.NDotVBias);

	// resolution
	int quarterWidth  = ((width+3)/4);
	int quarterHeight = ((height+3)/4);

	hbaoUbo.InvQuarterResolution = glm::vec2(1.0f/float(quarterWidth),1.0f/float(quarterHeight));
	hbaoUbo.InvFullResolution = glm::vec2(1.0f/float(width),1.0f/float(height));

#if USE_AO_LAYERED_SINGLEPASS
	for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++){
		hbaoUbo.float2Offsets[i] = glm::vec4(float(i % 4) + 0.5f, float(i / 4) + 0.5f, 0.f, 0.f);
		hbaoUbo.jitters[i] = hbaoRandom[i];
	}
#endif
}

//----------------------------------------------------

void SNSphereField::drawLinearDepth(const Projection& projection, int width, int height, int sampleIdx)
{
	//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.depthlinear);

	if (tweak.samples > 1)
	{
		depth_linearize_msaa->begin();

		glUniform4f(0, projection.nearplane * projection.farplane, projection.nearplane - projection.farplane, projection.farplane, projection.ortho ? 0.0f : 1.0f);
		glUniform1i(1,sampleIdx);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, textures.scene_depthstencil);
		glDrawArrays(GL_TRIANGLES,0,3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, 0);

		depth_linearize_msaa->end();
	} else
	{
		depth_linearize->begin();

		glUniform4f(0,
				projection.nearplane * projection.farplane,
				projection.nearplane - projection.farplane,
				projection.farplane,
				projection.ortho ? 0.0f : 1.0f);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthstencil);
		glDrawArrays(GL_TRIANGLES,0,3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);

		depth_linearize->end();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

//----------------------------------------------------

void SNSphereField::drawHbaoBlur(const Projection& projection, int width, int height, int sampleIdx)
{
	// hier muss noch der letzte fbo gebunden bleiben
	float meters2viewspace = 1.0f;

	if( USE_AO_SPECIALBLUR ){
		hbao_blur->begin();
	} else {
		bilateralblur->begin();
	}

	glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, textures.scene_depthlinear);

	glUniform1f(0.f, tweak.blurSharpness/meters2viewspace);

	glDrawBuffer(GL_COLOR_ATTACHMENT1);

	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.hbao_result);
	glUniform2f(1.f, 1.0f/float(width), 0.f);
	glDrawArrays(GL_TRIANGLES,0,3);

	// final output to main fbo
	//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO,GL_SRC_COLOR);

	if (tweak.samples > 1){
		glEnable(GL_SAMPLE_MASK);
		glSampleMaski(0, 1<<sampleIdx);
	}

#if USE_AO_SPECIALBLUR
	hbao_blur2->begin();
	glUniform1f(0, tweak.blurSharpness/meters2viewspace);
#endif

	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.hbao_blur);
	glUniform2f(1.f, 0.f, 1.0f/float(height));
	glDrawArrays(GL_TRIANGLES,0,3);
}

//----------------------------------------------------

void SNSphereField::drawHbaoClassic(const Projection& projection, int width, int height, int sampleIdx)
{
	prepareHbaoData(projection,width,height);
	drawLinearDepth(projection,width,height,sampleIdx);

	{
		if (tweak.blur)
		{
			//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		} else
		{
			//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO,GL_SRC_COLOR);
			if (tweak.samples > 1){
				glEnable(GL_SAMPLE_MASK);
				glSampleMaski(0, 1<<sampleIdx);
			}
		}

		if (USE_AO_SPECIALBLUR && tweak.blur){
			hbao_calc_blur->begin();
		} else {
			hbao_calc->begin();
		}

		glBindBufferBase(GL_UNIFORM_BUFFER,0,buffers.hbao_ubo);
		glNamedBufferSubDataEXT(buffers.hbao_ubo,0,sizeof(HBAOData),&hbaoUbo);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthlinear);
		glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, textures.hbao_randomview[sampleIdx]);
		glDrawArrays(GL_TRIANGLES,0,3);
	}

	if (tweak.blur){
		drawHbaoBlur(projection,width,height,sampleIdx);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_SAMPLE_MASK);
	glSampleMaski(0, ~0);

	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

//----------------------------------------------------

void SNSphereField::drawHbaoCacheAware(const Projection& projection, int width, int height, int sampleIdx)
{
	int quarterWidth  = ((width+3)/4);
	int quarterHeight = ((height+3)/4);

	prepareHbaoData(projection,width,height);
	drawLinearDepth(projection,width,height,sampleIdx);

	{
	//	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.viewnormal);

		viewnormal->begin();

		glUniform4fv(0, 1, &hbaoUbo.projInfo[0]);
		glUniform1i (1, hbaoUbo.projOrtho);
		glUniform2fv(2, 1, &hbaoUbo.InvFullResolution[0]);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthlinear);
		glDrawArrays(GL_TRIANGLES,0,3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	}

	{
	//	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);
		glViewport(0,0,quarterWidth,quarterHeight);

		hbao2_deinterleave->begin();
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthlinear);

		for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i+= NUM_MRT)
		{
			glUniform4f(0, float(i % 4) + 0.5f, float(i / 4) + 0.5f, hbaoUbo.InvFullResolution.x, hbaoUbo.InvFullResolution.y);

			for (int layer = 0; layer < NUM_MRT; layer++){
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + layer, textures.hbao2_depthview[i+layer], 0);
			}
			glDrawArrays(GL_TRIANGLES,0,3);
		}
	}

	{
	//	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
		glViewport(0,0,quarterWidth,quarterHeight);

		if(USE_AO_SPECIALBLUR && tweak.blur){
			hbao2_calc_blur->begin();
		} else {
			hbao2_calc->begin();
		}

		glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, textures.scene_viewnormal);

		glBindBufferBase(GL_UNIFORM_BUFFER,0,buffers.hbao_ubo);
		glNamedBufferSubDataEXT(buffers.hbao_ubo,0,sizeof(HBAOData),&hbaoUbo);

#if USE_AO_LAYERED_SINGLEPASS
		// instead of drawing to each layer individually
		// we draw all layers at once, and use image writes to update the array texture
		// this buys additional performance :)

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, textures.hbao2_deptharray);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
		glBindImageTexture( 0, textures.hbao2_resultarray, 0, GL_TRUE, 0, GL_WRITE_ONLY, USE_AO_SPECIALBLUR ? GL_RG16F : GL_R8);
#endif
		glDrawArrays(GL_TRIANGLES,0,3 * HBAO_RANDOM_ELEMENTS);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif
#else
		for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++){
			glUniform2f(0, float(i % 4) + 0.5f, float(i / 4) + 0.5f);
			glUniform4fv(1, 1, hbaoRandom[i].get_value());

			glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.hbao2_depthview[i]);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures.hbao2_resultarray, 0, i);

			glDrawArrays(GL_TRIANGLES,0,3);
		}
#endif
	}

	{
		if (tweak.blur){
		//	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}
		else{
		//	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO,GL_SRC_COLOR);
			if (tweak.samples > 1){
				glEnable(GL_SAMPLE_MASK);
				glSampleMaski(0, 1<<sampleIdx);
			}
		}
		glViewport(0,0,width,height);

		if(USE_AO_SPECIALBLUR && tweak.blur)
		{
			//std::cout << "use hbao2_reinterleave_blur" << std::endl;
			hbao2_reinterleave_blur->begin();
		} else {
			hbao2_reinterleave->begin();
		}

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, textures.hbao2_resultarray);
		glDrawArrays(GL_TRIANGLES,0,3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, 0);
	}

	if (tweak.blur){
		drawHbaoBlur(projection,width,height,sampleIdx);
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_SAMPLE_MASK);
	glSampleMaski(0,~0);

	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

//----------------------------------------------------

void SNSphereField::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
	}

	if(!inited)
	{
//		noiseTex = new Noise3DTexGen(shCol,
//				true, 4,
//				256, 256, 64,
//				4.f, 4.f, 16.f);
		inited = true;
	}

	m_control.m_sceneOrtho = tweak.ortho;

//	int width   = scd->screenWidth;
//	int height  = scd->screenHeight;

	int width   = int(cp->actFboSize.x);
	int height  = int(cp->actFboSize.y);

	m_control.m_viewMatrix = glm::lookAt(
			m_control.m_sceneOrbit - (glm::vec3(0.f, 0.f, -4.5f) * m_control.m_sceneDimension), // Camera pos
			//m_control.m_sceneOrbit - (glm::vec3(0.4f,-0.35f,-0.6f) * m_control.m_sceneDimension * 0.9f), // Camera pos
			m_control.m_sceneOrbit, // looks at
			glm::vec3(0.f, 1.f, 0.f)) * _modelMat;

	projection.fov = 0.2f;
	projection.nearplane = m_control.m_sceneDimension * 1.f;
	projection.ortho       = m_control.m_sceneOrtho;
	projection.orthoheight = m_control.m_sceneOrthoZoom;
	projection.update(width,height);

	udpateLiquid(time);

	// update offset positions via copute shader
	updateOffsPos(time);

	if(cp->usesFbo)
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

	glm::vec4   bgColor(0.0, 0.0, 0.0, 0.0);
	glClearBufferfv(GL_COLOR, 0, &bgColor.x);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glEnable(GL_DEPTH_TEST);

	sceneUbo.viewport = glm::uvec2(width,height);

	glm::mat4 view = m_control.m_viewMatrix;
//	glm::mat4 view = cp->view_matrix_mat4;

	sceneUbo.viewProjMatrix = projection.matrix * view;
	//sceneUbo.viewProjMatrix = cp->projection_matrix_mat4 * view;
	sceneUbo.viewMatrix = view;
	sceneUbo.viewMatrixIT = glm::transpose(glm::inverse(view));

	draw_scene->begin();
	draw_scene->setUniform1ui("sphereIndOffs", sphereIndOffs);
	draw_scene->setUniform1f("sphereSize", spSize);

	glBindVertexArray(defaultVAO);               // create VAO, assign the name and bind that array

	// bind position offset buffer
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, modu_pos->getBuffer());
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, m_vel->getBuffer());

	// bind Uniform Buffer
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, buffers.scene_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &sceneUbo);

	glBindVertexBuffer(0, buffers.scene_vbo, 0, sizeof(tav::Vertex));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.scene_ibo);

	glEnableVertexAttribArray(VERTEX_POS);
	glEnableVertexAttribArray(VERTEX_NORMAL);
	glEnableVertexAttribArray(VERTEX_COLOR);

	glDrawElements(GL_TRIANGLES, sceneTriangleIndices, GL_UNSIGNED_INT, NV_BUFFER_OFFSET(0));

	glDisableVertexAttribArray(VERTEX_POS);
	glDisableVertexAttribArray(VERTEX_NORMAL);
	glDisableVertexAttribArray(VERTEX_COLOR);

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, 0);
	glBindVertexBuffer(0,0,0,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 2, 0);
//----------------------------------------------------

	for (int sample = 0; sample < tweak.samples; sample++)
	{
		switch(tweak.algorithm){
			case ALGORITHM_HBAO_CLASSIC:
				drawHbaoClassic(projection, width, height, sample);
				break;
			case ALGORITHM_HBAO_CACHEAWARE:
				drawHbaoCacheAware(projection, width, height, sample);
				break;
		}
	}

	glBindVertexArray(0);               // create VAO, assign the name and bind that array
//----------------------------------------------------

	// blit to background
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos.scene);

	if(cp->usesFbo)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastBoundFbo);
	else
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(0, 0, width, height, 0, 0, (cp->actFboSize.x), int(cp->actFboSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if(cp->usesFbo)
		glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

//----------------------------------------------------

void SNSphereField::update(double time, double dt)
{
	if (time - lastUpdt > 0.2)
	{
//		oscPar["spSize"].second = 0.f;
//		oscPar["spSize"].first->clear();
//		oscPar["spSize"].first->update(0.f);
	}

	lastUpdt = time;
}

//----------------------------------------------------

void SNSphereField::checkFboStatus()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if ( GL_FRAMEBUFFER_COMPLETE != status )
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cout << "FBO Error: Attachment Point Unconnected" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cout << "FBO Error: Missing Attachment" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			std::cout << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			std::cout << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
			break;
			//                case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			//                    std::cout << "Framebuffer Object %d Error: Dimensions do not match" << std::endl;
			//                    break;
			//                case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
			//                    std::cout << "Framebuffer Object %d Error: Formats" << std::endl;
			//                    break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cout << "Framebuffer Object %d Error: Unsupported Framebuffer Configuration"  << std::endl;
			break;
		default:
			break;
		}
	}
	getGlError();
}
//----------------------------------------------------


SNSphereField::~SNSphereField()
{ }

}
