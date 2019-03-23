//
// SSAO.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  bei den nvidia-settings muss unbedingt antialiasing auf "use Application Settings stehen" !!!!!
//  sonst GL_INALID_OPERATION beim Kopieren (blit) der FBOs

#define STRINGIFY(A) #A

#include "pch.h"
#include "SSAO.h"

namespace tav
{
SSAO::SSAO(sceneData* _scd, AlgorithmType _algorithm, bool _blur,
		float _intensity, float _blurSharpness) :
		scd(_scd), algorithm(_algorithm), blur(_blur), intensity(_intensity),
		blurSharpness(_blurSharpness), lastBoundFbo(0)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	samples = _scd->nrSamples;

	propo = _scd->roomDim->x / _scd->roomDim->y;

	// init default VAO
	glGenVertexArrays(1, &defaultVAO);
	glBindVertexArray(defaultVAO);

	shCol = scd->shaderCollector;
	texShdr = _scd->shaderCollector->getStdTexAlpha(samples > 1);
	initShaders();
	initMisc();

	glBindVertexArray(0);

	initFramebuffers(scd->fboWidth, scd->fboHeight, samples);

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 0.f);
}

//------------------------------------------------------------------------

void SSAO::initShaders()
{
	com = "#define UBO_SCENE " + std::to_string( UBO_SCENE) + "\n";
	com += "#define AO_RANDOMTEX_SIZE " + std::to_string(AO_RANDOMTEX_SIZE)
			+ "\n";
	com +=
			STRINGIFY(
					struct SceneData {\n mat4 viewProjMatrix;\n mat4 viewMatrix;\n mat4 viewMatrixIT;\n uvec2 viewport;\n uvec2 _pad;\n };\n

					struct HBAOData {\n float RadiusToScreen;\n        // radius
					float R2;\n// 1/radius
					float NegInvR2;\n// radius * radius
					float NDotVBias;\n vec2 InvFullResolution;\n vec2 InvQuarterResolution;\n float AOMultiplier;\n float PowExponent;\n vec2 _pad0;\n vec4 projInfo;\n vec2 projScale;\n int projOrtho;\n int _pad1;\n vec4 float2Offsets[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];\n vec4 jitters[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];\n };\n);

	shdr_Header = "#version 430\n#define AO_LAYERED "
			+ std::to_string( USE_AO_LAYERED_SINGLEPASS) + "\n";

	initFullScrQuad();
	initBilateralblur();

	std::string frag = initDepthLinearize(0);
	depth_linearize = shCol->addCheckShaderText("SSAODepth_linearize",
			fullScrQuad.c_str(), frag.c_str());

	frag = initDepthLinearize(1);
	depth_linearize_msaa = shCol->addCheckShaderText("SSAODepth_linearize_msaa",
			fullScrQuad.c_str(), frag.c_str());

	initViewNormal();

	frag = initHbaoCalc(0, 0);
	hbao_calc = shCol->addCheckShaderText("SSAOHBAO_Calc", fullScrQuad.c_str(),
			frag.c_str());

	frag = initHbaoCalc(1, 0);
	hbao_calc_blur = shCol->addCheckShaderText("SSAOHBAO_Calc_Blur",
			fullScrQuad.c_str(), frag.c_str());

	frag = initHbaoBlur(0);
	hbao_blur = shCol->addCheckShaderText("SSAOHBAO_Blur", fullScrQuad.c_str(),
			frag.c_str());

	frag = initHbaoBlur(1);
	hbao_blur2 = shCol->addCheckShaderText("SSAOHBAO_BlurMsaa",
			fullScrQuad.c_str(), frag.c_str());

	frag = initHbaoCalc(0, 1);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
	hbao2_calc = shCol->addCheckShaderText("SSAOHBAO2_Calc",
			fullScrQuad.c_str(), fullScrQuadGeo.c_str(), frag.c_str());
#else
	hbao2_calc = shCol->addCheckShaderText("SSAOHBAO2_Calc", fullScrQuad.c_str(), frag.c_str());
#endif

	frag = initHbaoCalc(1, 1);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
	hbao2_calc_blur = shCol->addCheckShaderText("SSAOHBAO2_Calc_Blur",
			fullScrQuad.c_str(), fullScrQuadGeo.c_str(), frag.c_str());
#else
	hbao2_calc_blur = shCol->addCheckShaderText("SSAOHBAO2_Calc_Blur", fullScrQuad.c_str(), frag.c_str());
#endif

	initDeinterleave();

	frag = initReinterleave(0);
	hbao2_reinterleave = shCol->addCheckShaderText("SSAOHBAO2_Reinterleave",
			fullScrQuad.c_str(), frag.c_str());

	frag = initReinterleave(1);
	hbao2_reinterleave_blur = shCol->addCheckShaderText(
			"SSAOHBAO2_ReinterleaveBlur", fullScrQuad.c_str(), frag.c_str());

	debugDepth = initDebugDepth();
}

//------------------------------------------------------------------------

Shaders* SSAO::initDebugDepth()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 2 ) in vec2 texCoord; uniform mat4 m_pvm; out vec2 tex_coord; void main() { tex_coord = texCoord; gl_Position = position; });

	vert = "// debug depth shader, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					in vec4 col; layout (location = 0) out vec4 color; uniform sampler2D tex; in vec2 tex_coord; void main() { color = texture(tex, tex_coord) * 0.1; });
	frag = "// debug depth shader, frag\n" + shdr_Header + frag;

	return shCol->addCheckShaderText("SSaoDebugDepth", vert.c_str(),
			frag.c_str());
}

//------------------------------------------------------------------------

void SSAO::initFullScrQuad()
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

	fullScrQuad = "// SSAO Full Screen Quad vertex shader\n" + shdr_Header + com
			+ vert;

	//------------------------------------------------------------------------

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

	fullScrQuadGeo = "// SSAO Full Screen Quad geometry shader\n" + shdr_Header + add_shdr_header + com + geo;
}

//------------------------------------------------------------------------

void SSAO::initBilateralblur()
{
	std::string frag = STRINGIFY(
		const float KERNEL_RADIUS = 3;\n

		layout(location=0) uniform float g_Sharpness;\n
		layout(location=1) uniform vec2 g_InvResolutionDirection;\n // either set x to 1/width or y to 1/height

		layout(binding=0) uniform sampler2D texSource;\n
		layout(binding=1) uniform sampler2D texLinearDepth;\n

		in vec2 texCoord;\n

		layout(location=0,index=0) out vec4 out_Color;\n

		//-------------------------------------------------------------------------

		vec4 BlurFunction(vec2 uv, float r, vec4 center_c, float center_d, inout float w_total)\n
		{\n
			vec4 c = texture2D( texSource, uv );\n
			float d = texture2D( texLinearDepth, uv).x;\n

			const float BlurSigma = float(KERNEL_RADIUS) * 0.5;\n
			const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);\n

			float ddiff = (d - center_d) * g_Sharpness;\n
			float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);\n
			w_total += w;\n

			return c*w;\n
		}\n

		void main()\n
		{\n
			vec4 center_c = texture2D( texSource, texCoord );\n
			float center_d = texture2D( texLinearDepth, texCoord).x;\n

			vec4 c_total = center_c;\n
			float w_total = 1.0;\n

			for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
			{\n
				vec2 uv = texCoord + g_InvResolutionDirection * r;\n
				c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
			}\n


			for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
			{\n
				vec2 uv = texCoord - g_InvResolutionDirection * r;\n
				c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
			}\n

			out_Color = c_total/w_total;\n
		});

	frag = "// SSAO Bilateralblur Shader vertex shader\n" + shdr_Header + frag;

	bilateralblur = shCol->addCheckShaderText("SSAOBilateralblur",
			fullScrQuad.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

std::string SSAO::initDepthLinearize(int msaa)
{
	std::string add_shdr_Header = "#define DEPTHLINEARIZE_MSAA ";
	add_shdr_Header += std::to_string(msaa);
	add_shdr_Header += "\n";
	add_shdr_Header +=
			"#ifndef DEPTHLINEARIZE_USEMSAA\n#define DEPTHLINEARIZE_USEMSAA 0\n#endif\n";
	add_shdr_Header +=
			"#if DEPTHLINEARIZE_MSAA\nlayout(location=1) uniform int sampleIndex;\nlayout(binding=0)  uniform sampler2DMS inputTexture;\n ";
	add_shdr_Header +=
			"#else\n layout(binding=0)  uniform sampler2D inputTexture;\n#endif\n";

	std::string frag =
			STRINGIFY(layout(location=0) uniform vec4 clipInfo;\n // z_n * z_f,  z_n - z_f,  z_f, perspective = 1 : 0
					layout(location=0,index=0) out float out_Color;\n

					float reconstructCSZ(float d, vec4 clipInfo) {\n
						if (clipInfo[3] != 0) {\n
							return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));\n
						} else {\n
							return (clipInfo[1] + clipInfo[2] - d * clipInfo[1]);\n
						}\n
					}

					void main() {\n);

					frag += "#if DEPTHLINEARIZE_MSAA\n";
					frag += "float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), sampleIndex).x;\n";
					frag += "#else\n";
					frag += "float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), 0).x;\n";
					frag += "#endif\n";
					frag += "out_Color = reconstructCSZ(depth, clipInfo);\n}";

					frag = "// SSAO Depth Linearize vertex shader\n" + shdr_Header + add_shdr_Header + frag;

					return frag;
				}

//------------------------------------------------------------------------

void SSAO::initViewNormal()
{
	std::string frag =
			STRINGIFY(
					in vec2 texCoord;\n
					layout(location=0) uniform vec4 projInfo;\n
					layout(location=1) uniform int projOrtho;\n
					layout(location=2) uniform vec2 InvFullResolution;\n
					layout(binding=0) uniform sampler2D texLinearDepth;\n
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
						vec3 V1 = Pr - P;\n vec3 V2 = P - Pl;\n
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
					void main() {\n
						vec3 P = FetchViewPos(texCoord);\n
						vec3 N = ReconstructNormal(texCoord, P);\n
						out_Color = vec4(N*0.5 + 0.5,0);\n
					});

	frag = "// SSAO View Normal vertex shader\n" + shdr_Header + frag;

	viewnormal = shCol->addCheckShaderText("SSAOViewNormal",
			fullScrQuad.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

std::string SSAO::initHbaoCalc(int _blur, int deinterl)
{
	std::string add_shdr_Header = "#define AO_DEINTERLEAVED "
			+ std::to_string(deinterl) + "\n";
	add_shdr_Header += "#define AO_BLUR " + std::to_string(_blur) + "\n";
	// The pragma below is critical for optimal performance
	// in this fragment shader to let the shader compiler
	// fully optimize the maths and batch the texture fetches
	// optimally
	add_shdr_Header +=
			"#pragma optionNV(unroll all) \n#ifndef AO_DEINTERLEAVED\n#define AO_DEINTERLEAVED 1\n#endif\n";
	add_shdr_Header +=
			"#ifndef AO_BLUR\n#define AO_BLUR 1\n#endif\n#ifndef AO_LAYERED\n#define AO_LAYERED 1\n#endif\n#define M_PI 3.14159265f\n";

	// tweakables
	std::string frag =
			STRINGIFY(
					const float NUM_STEPS = 4;\n const float NUM_DIRECTIONS = 8;\n // texRandom/g_Jitter initialization depends on this
					layout(std140,binding=0) uniform controlBuffer { HBAOData control; };\n);

	frag += "#if AO_DEINTERLEAVED\n";

	frag += "#if AO_LAYERED\n";
	frag +=
			STRINGIFY(
					vec2 g_Float2Offset = control.float2Offsets[gl_PrimitiveID].xy;\n vec4 g_Jitter = control.jitters[gl_PrimitiveID];\n

					layout(binding=0) uniform sampler2DArray texLinearDepth;\n layout(binding=1) uniform sampler2D texViewNormal;\n

					vec3 getQuarterCoord(vec2 UV){\n return vec3(UV,float(gl_PrimitiveID));\n }\n);
	frag += "#if AO_LAYERED == 1\n";
	frag += "#if AO_BLUR\n";
	frag += "layout(binding=0,rg16f) uniform image2DArray imgOutput;\n";
	frag += "#else\n";
	frag += "layout(binding=0,r8) uniform image2DArray imgOutput;\n";
	frag += "#endif\n";
	frag +=
			STRINGIFY(
					void outputColor(vec4 color) {\n imageStore(imgOutput, ivec3(ivec2(gl_FragCoord.xy),gl_PrimitiveID), color);\n }\n);
	frag += "#else\n";
	frag +=
			STRINGIFY(
					layout(location=0,index=0) out vec4 out_Color;\n \n void outputColor(vec4 color) {\n out_Color = color;\n }\n);
	frag += "#endif\n";
	frag += "#else\n";
	frag +=
			STRINGIFY(
					layout(location=0) uniform vec2 g_Float2Offset;\n layout(location=1) uniform vec4 g_Jitter;\n \n layout(binding=0) uniform sampler2D texLinearDepth;\n layout(binding=1) uniform sampler2D texViewNormal;\n \n vec2 getQuarterCoord(vec2 UV){\n return UV;\n }\n

					layout(location=0,index=0) out vec4 out_Color;\n

					void outputColor(vec4 color) {\n out_Color = color;\n }\n);
	frag += "#endif\n";

	frag += "#else\n";
	frag +=
			STRINGIFY(
					layout(binding=0) uniform sampler2D texLinearDepth;\n layout(binding=1) uniform sampler2D texRandom;\n \n layout(location=0,index=0) out vec4 out_Color;\n \n void outputColor(vec4 color) {\n out_Color = color;\n }\n);
	frag += "#endif\n";
	frag +=
			STRINGIFY(in vec2 texCoord;\n

			//----------------------------------------------------------------------------------

					vec3 UVToView(vec2 uv, float eye_z) {\n return vec3((uv * control.projInfo.xy + control.projInfo.zw) * (control.projOrtho != 0 ? 1. : eye_z), eye_z);\n }\n);

	frag += "#if AO_DEINTERLEAVED\n";

	frag +=
			STRINGIFY(
					vec3 FetchQuarterResViewPos(vec2 UV) {\n float ViewDepth = textureLod(texLinearDepth,getQuarterCoord(UV),0).x;\n return UVToView(UV, ViewDepth);\n }\n);

	frag += "#else //AO_DEINTERLEAVED\n";

	frag +=
			STRINGIFY(
					vec3 FetchViewPos(vec2 UV) {\n float ViewDepth = textureLod(texLinearDepth,UV,0).x;\n return UVToView(UV, ViewDepth);\n }\n

					vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl) {\n vec3 V1 = Pr - P;\n vec3 V2 = P - Pl;\n return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;\n }\n

					vec3 ReconstructNormal(vec2 UV, vec3 P) {\n vec3 Pr = FetchViewPos(UV + vec2(control.InvFullResolution.x, 0));\n vec3 Pl = FetchViewPos(UV + vec2(-control.InvFullResolution.x, 0));\n vec3 Pt = FetchViewPos(UV + vec2(0, control.InvFullResolution.y));\n vec3 Pb = FetchViewPos(UV + vec2(0, -control.InvFullResolution.y));\n return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));\n }\n);

	frag += "#endif //AO_DEINTERLEAVED\n";

	//----------------------------------------------------------------------------------
	frag +=
			STRINGIFY(float Falloff(float DistanceSquare) {\n
			// 1 scalar mad instruction
					return DistanceSquare * control.NegInvR2 + 1.0;\n }\n

					//----------------------------------------------------------------------------------
					// P = view-space position at the kernel center
					// N = view-space normal at the kernel center
					// S = view-space position of the current sample
					//----------------------------------------------------------------------------------
					float ComputeAO(vec3 P, vec3 N, vec3 S) {\n vec3 V = S - P;\n float VdotV = dot(V, V);\n float NdotV = dot(N, V) * 1.0/sqrt(VdotV);\n

					// Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
					return clamp(NdotV - control.NDotVBias,0,1) * clamp(Falloff(VdotV),0,1);\n }\n

					//----------------------------------------------------------------------------------
					vec2 RotateDirection(vec2 Dir, vec2 CosSin) {\n return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,\n Dir.x*CosSin.y + Dir.y*CosSin.x);\n }\n

					//----------------------------------------------------------------------------------
					vec4 GetJitter() {\n);
					frag += "#if AO_DEINTERLEAVED\n";
					// Get the current jitter vector from the per-pass constant buffer
					frag += "return g_Jitter;\n";
					frag += "#else\n";
					// (cos(Alpha),sin(Alpha),rand1,rand2)
					frag += "return textureLod( texRandom, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE), 0);\n";
					frag += "#endif\n}";

					//----------------------------------------------------------------------------------
					frag += "float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal) {\n";
					frag += "#if AO_DEINTERLEAVED\n";
					frag += "RadiusPixels /= 4.0;\n";
					frag += "#endif\n";

					// Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
					frag += STRINGIFY(float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);\n

					const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;\n
					float AO = 0;\n

					for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
					{	\n
						float Angle = Alpha * DirectionIndex;\n

						// Compute normalized 2D direction
						vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);\n

						// Jitter starting sample within the first step
						float RayPixels = (Rand.z * StepSizePixels + 1.0);\n

						for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
						{	\n);
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

				//----------------------------------------------------------------------------------
				void main()\n
				{	\n);

					frag += "#if AO_DEINTERLEAVED\n";
					frag += STRINGIFY(vec2 base = floor(gl_FragCoord.xy) * 4.0 + g_Float2Offset;\n
					vec2 uv = base * (control.InvQuarterResolution / 4.0);\n

					vec3 ViewPosition = FetchQuarterResViewPos(uv);\n
					vec4 NormalAndAO = texelFetch( texViewNormal, ivec2(base), 0);\n
					vec3 ViewNormal = -(NormalAndAO.xyz * 2.0 - 1.0);\n);
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

					frag = "//SSAO HBAO Calc \n" + shdr_Header + add_shdr_Header + com + frag;

					return frag;
				}

//------------------------------------------------------------------------

std::string SSAO::initHbaoBlur(int _blur)
{
	std::string add_shdr_Header = "#define AO_BLUR_PRESENT "
			+ std::to_string(_blur) + "\n";

	std::string frag = STRINGIFY(
		const float KERNEL_RADIUS = 3;\n

		layout(location=0) uniform float g_Sharpness;\n
		layout(location=1) uniform vec2 g_InvResolutionDirection;\n // either set x to 1/width or y to 1/height

		layout(binding=0) uniform sampler2D texSource;\n
		in vec2 texCoord;\n
		layout(location=0,index=0) out vec4 out_Color;\n
	);

	frag += "#ifndef AO_BLUR_PRESENT\n";
	frag += "#define AO_BLUR_PRESENT 1\n";
	frag += "#endif\n";

	//-------------------------------------------------------------------------

	frag += STRINGIFY(
		float BlurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total) {\n
			vec2 aoz = texture2D( texSource, uv ).xy;\n
			float c = aoz.x;\n
			float d = aoz.y;\n

			const float BlurSigma = float(KERNEL_RADIUS) * 0.5;\n
			const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);\n

			float ddiff = (d - center_d) * g_Sharpness;\n
			float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);\n
			w_total += w;\n

			return c*w;\n
		}\n

		void main()\n {
			vec2 aoz = texture2D( texSource, texCoord ).xy;\n
			float center_c = aoz.x;\n
			float center_d = aoz.y;\n
			\n
			float c_total = center_c;\n
			float w_total = 1.0;\n
			\n
			for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
			{
				vec2 uv = texCoord + g_InvResolutionDirection * r;\n
				c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
			}\n
			\n
			for (float r = 1; r <= KERNEL_RADIUS; ++r)\n
			{
				vec2 uv = texCoord - g_InvResolutionDirection * r;\n
				c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
			}\n
	);

	frag += "#if AO_BLUR_PRESENT\n";
	frag += "out_Color = vec4(c_total/w_total);\n";
	frag += "#else\n";
	frag += "out_Color = vec4(c_total/w_total, center_d, 0, 0);\n";
	frag += "#endif\n";
	frag += "}";

	frag = "// SSAO HBAO_Calc Shader frag shader\n" + shdr_Header + add_shdr_Header + frag;

	return frag;
}

//------------------------------------------------------------------------

void SSAO::initDeinterleave()
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

	frag = "// SSAO Deinterleave shader\n" + shdr_Header + frag;

	hbao2_deinterleave = shCol->addCheckShaderText("SSAO_HBAO2_deinterleave", fullScrQuad.c_str(), frag.c_str());
}

//------------------------------------------------------------------------

std::string SSAO::initReinterleave(int _blur)
{
	std::string add_shdr_Header = "#define AO_BLUR " + std::to_string(_blur)
			+ "\n";

	std::string frag = "#ifndef AO_BLUR\n";
	frag += "#define AO_BLUR 1\n";
	frag += "#endif\n";

	frag +=
			STRINGIFY(
					layout(binding=0) uniform sampler2DArray texResultsArray;\n layout(location=0,index=0) out vec4 out_Color;\n

					void main() {\n ivec2 FullResPos = ivec2(gl_FragCoord.xy);\n ivec2 Offset = FullResPos & 3;\n int SliceId = Offset.y * 4 + Offset.x;\n ivec2 QuarterResPos = FullResPos >> 2;\n );

					frag += "#if AO_BLUR\n";
					frag += "out_Color = vec4(texelFetch( texResultsArray, ivec3(QuarterResPos, SliceId), 0).xy,0,0);\n";
					frag += "#else\n";
					frag += "out_Color = vec4(texelFetch( texResultsArray, ivec3(QuarterResPos, SliceId), 0).x);\n";
					frag += "#endif\n}";

					frag = "// SSAO Reinterleave Shader frag shader\n" + shdr_Header + add_shdr_Header + frag;

					return frag;
				}

//------------------------------------------------------------------------

std::string SSAO::initComputeUpdtShdr()
{
	// version statement kommt von aussen
	std::string shdr_Header = "#define WORK_GROUP_SIZE 128\n";

	std::string src =
			STRINGIFY(
					uniform sampler3D noiseTex3D;\n uniform uint totNumParticles;\n uniform vec3 numPart;\n uniform float time;\n uniform float perlForce;\n uniform float randAmt;\n

					layout( std140, binding=1 ) buffer Modu_pos { vec4 offs_pos[];\n };\n layout( std140, binding=2 ) buffer Ref_pos { vec4 r_pos[];\n };\n layout( std140, binding=3 ) buffer Vel { vec4 vel[];\n };\n

					layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;\n

					// compute shader to update particles
					void main() {\n

					uint i = gl_GlobalInvocationID.x;\n

					// thread block size may not be exact multiple of number of particles
					if (i >= totNumParticles) return;

					vec3 normPos = vec3(mod(float(i), numPart.x) / numPart.x,\n mod(float(i) / numPart.x, numPart.y) / numPart.y,\n mod(float(i) / (numPart.x * numPart.y), numPart.z) / numPart.z);\n

					// read particle position and velocity from buffers
					vec3 offsCo = vec3(normPos.x + time * 0.2,\n normPos.y + time * 0.23,\n normPos.z * 0.8 + time * 0.12);\n

					vec4 perlOffs = texture(noiseTex3D, offsCo);\n perlOffs -= vec4(0.25, 0.25, 0.25, 0);// -0.25 bis 0.25
					perlOffs.z *= 10.0;

					// read particle position and velocity from buffers
					vec3 p = r_pos[i].xyz + perlOffs.xyz * perlForce;\n

					// write new values
					offs_pos[i] = vec4(p, 1.0);\n vel[i].x = 0.9;\n

					// write new values
					offs_pos[i] = vec4(mix(r_pos[i].xyz, offs_pos[i].xyz, randAmt), 0.0);\n });

	src = shdr_Header + src;

	return src;
}

//------------------------------------------------------------------------

bool SSAO::initMisc()
{
	MTRand rng;

	float numDir = 8; // keep in sync to glsl

	rng.seed((unsigned) 0);

	signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES * 4];

	for (int i = 0; i < HBAO_RANDOM_ELEMENTS * MAX_SAMPLES; i++)
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
		hbaoRandomShort[i * 4 + 0] = (signed short) (SCALE * hbaoRandom[i].x);
		hbaoRandomShort[i * 4 + 1] = (signed short) (SCALE * hbaoRandom[i].y);
		hbaoRandomShort[i * 4 + 2] = (signed short) (SCALE * hbaoRandom[i].z);
		hbaoRandomShort[i * 4 + 3] = (signed short) (SCALE * hbaoRandom[i].w);
#undef SCALE
	}

	newTexture(textures.hbao_random);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao_random);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE,
			HBAO_RANDOM_SIZE, MAX_SAMPLES);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, HBAO_RANDOM_SIZE,
			HBAO_RANDOM_SIZE, MAX_SAMPLES, GL_RGBA, GL_SHORT, hbaoRandomShort);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		newTexture(textures.hbao_randomview[i]);
		glTextureView(textures.hbao_randomview[i], GL_TEXTURE_2D,
				textures.hbao_random, GL_RGBA16_SNORM, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, textures.hbao_randomview[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (hbao_ubo)
		glDeleteBuffers(1, &hbao_ubo);
	glGenBuffers(1, &hbao_ubo);
	glNamedBufferStorageEXT(hbao_ubo, sizeof(HBAOData), NULL,
			GL_DYNAMIC_STORAGE_BIT);

	return true;
}

//----------------------------------------------------

bool SSAO::initFramebuffers(int width, int height, int samples)
{
	if (samples > 1)
	{
		newTexture(textures.scene_color);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures.scene_color);
		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8,
				width, height, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		newTexture(textures.scene_depthstencil);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures.scene_depthstencil);
		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples,
				GL_DEPTH24_STENCIL8, width, height, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
	else
	{
		newTexture(textures.scene_color);
		glBindTexture(GL_TEXTURE_2D, textures.scene_color);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);

		newTexture(textures.scene_depthstencil);
		glBindTexture(GL_TEXTURE_2D, textures.scene_depthstencil);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	newFramebuffer(fbos.scene);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			textures.scene_color, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
			textures.scene_depthstencil, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	newTexture(textures.scene_depthlinear);
	glBindTexture(GL_TEXTURE_2D, textures.scene_depthlinear);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	newFramebuffer(fbos.depthlinear);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.depthlinear);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			textures.scene_depthlinear, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	newTexture(textures.scene_viewnormal);
	glBindTexture(GL_TEXTURE_2D, textures.scene_viewnormal);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	newFramebuffer(fbos.viewnormal);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.viewnormal);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			textures.scene_viewnormal, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	// hbao
#if USE_AO_SPECIALBLUR
	GLenum formatAO = GL_RG16F;
	GLint swizzle[4] =
	{ GL_RED, GL_GREEN, GL_ZERO, GL_ZERO };
#else
	GLenum formatAO = GL_R8;
	GLint swizzle[4] =
	{	GL_RED,GL_RED,GL_RED,GL_RED};
#endif

	newTexture(textures.hbao_result);
	glBindTexture(GL_TEXTURE_2D, textures.hbao_result);
	glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	newTexture(textures.hbao_blur);
	glBindTexture(GL_TEXTURE_2D, textures.hbao_blur);
	glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	newFramebuffer(fbos.hbao_calc);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			textures.hbao_result, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
			textures.hbao_blur, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	// interleaved hbao
	int quarterWidth = ((width + 3) / 4);
	int quarterHeight = ((height + 3) / 4);

	newTexture(textures.hbao2_deptharray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao2_deptharray);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, quarterWidth, quarterHeight,
			HBAO_RANDOM_ELEMENTS);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
	{
		newTexture(textures.hbao2_depthview[i]);
		glTextureView(textures.hbao2_depthview[i], GL_TEXTURE_2D,
				textures.hbao2_deptharray, GL_R32F, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, textures.hbao2_depthview[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	newTexture(textures.hbao2_resultarray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao2_resultarray);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, formatAO, quarterWidth,
			quarterHeight, HBAO_RANDOM_ELEMENTS);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	GLenum drawbuffers[NUM_MRT];
	for (int layer = 0; layer < NUM_MRT; layer++)
	{
		drawbuffers[layer] = GL_COLOR_ATTACHMENT0 + layer;
	}

	newFramebuffer(fbos.hbao2_deinterleave);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);
	glDrawBuffers(NUM_MRT, drawbuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
	checkFboStatus();

	newFramebuffer(fbos.hbao2_calc);
	getGlError();

	glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
	getGlError();
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
	// this fbo will not have any attachments and therefore requires rasterizer to be configured
	// through default parameters
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, quarterWidth);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, quarterHeight);
#endif
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			textures.hbao2_resultarray, 0);
	getGlError();
#endif
	checkFboStatus();

	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

	return true;
}

//----------------------------------------------------

void SSAO::prepareHbaoData(camPar* cp, int width, int height)
{
	// projection
	const float* P = &cp->multicam_proj_mat4[cp->activeMultiCam][0][0];

	hbaoUbo.projOrtho = 0;
	hbaoUbo.projInfo = glm::vec4(2.0f / (P[4 * 0 + 0]),       // (x) * (R - L)/N
	2.0f / (P[4 * 1 + 1]),       // (y) * (T - B)/N
	-(1.0f - P[4 * 2 + 0]) / P[4 * 0 + 0], // L/N
	-(1.0f + P[4 * 2 + 1]) / P[4 * 1 + 1] // B/N
			);

	float projScale = float(height) / (tanf(cp->fov * 0.5f) * 2.0f);

	// radius
	float R = tweak.radius;
	hbaoUbo.R2 = R * R;
	hbaoUbo.NegInvR2 = -1.0f / hbaoUbo.R2;
	hbaoUbo.RadiusToScreen = R * 0.5f * projScale;

	// ao
	hbaoUbo.PowExponent = std::max(intensity, 0.0f);
	hbaoUbo.NDotVBias = std::min(std::max(0.0f, tweak.bias), 1.0f);
	hbaoUbo.AOMultiplier = 1.0f / (1.0f - hbaoUbo.NDotVBias);

	// resolution
	int quarterWidth = ((width + 3) / 4);
	int quarterHeight = ((height + 3) / 4);

	hbaoUbo.InvQuarterResolution = glm::vec2(1.0f / float(quarterWidth),
			1.0f / float(quarterHeight));
	hbaoUbo.InvFullResolution = glm::vec2(1.0f / float(width),
			1.0f / float(height));

#if USE_AO_LAYERED_SINGLEPASS
	for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
	{
		hbaoUbo.float2Offsets[i] = glm::vec4(float(i % 4) + 0.5f,
				float(i / 4) + 0.5f, 0.f, 0.f);
		hbaoUbo.jitters[i] = hbaoRandom[i];
	}
#endif
}

//----------------------------------------------------

void SSAO::drawLinearDepth(camPar* cp, int width, int height, int sampleIdx)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.depthlinear);

	if (samples > 1)
	{
		depth_linearize_msaa->begin();

		glUniform4f(0, cp->near * cp->far, cp->near - cp->far, cp->far, 1.0f);
		glUniform1i(1, sampleIdx);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE,
				textures.scene_depthstencil);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, 0);

		depth_linearize_msaa->end();
	}
	else
	{
		depth_linearize->begin();

		glUniform4f(0, cp->near * cp->far, cp->near - cp->far, cp->far, 1.0f);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D,
				textures.scene_depthstencil);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);

		depth_linearize->end();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

//----------------------------------------------------

void SSAO::drawHbaoBlur(int width, int height, int sampleIdx)
{
	// hier muss noch der letzte fbo gebunden bleiben
	//float meters2viewspace = 1.0f; geteilt durch 1.0f ? unnoetig

	// wenn blur activ, hbao_calc fbo gebunden, 2 attachments 0 > result, 1 > blur

	if ( USE_AO_SPECIALBLUR)
	{
		hbao_blur->begin();
	}
	else
	{
		bilateralblur->begin();
	}
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures.scene_depthlinear);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures.hbao_result);

	glUniform2f(1, 1.0f / float(width), 0.f);		// InvResolutionDirection
	glUniform1f(0, blurSharpness); 	// sharpness

	glDrawBuffer(GL_COLOR_ATTACHMENT1);		// draw to blur result attachment

	glDrawArrays(GL_TRIANGLES, 0, 3);

	//--------------------------------------------------------
	// final output to main fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);

	if (samples > 1)
	{
		glEnable(GL_SAMPLE_MASK);
		glSampleMaski(0, 1 << sampleIdx);
	}

#if USE_AO_SPECIALBLUR
	hbao_blur2->begin();
	glUniform1f(0, blurSharpness);	// sharpness
#endif

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures.hbao_blur);
	glUniform2f(1, 0.f, 1.0f / float(height));	// InvResolutionDirection
	glDrawArrays(GL_TRIANGLES, 0, 3);

}

//----------------------------------------------------

void SSAO::drawHbaoClassic(camPar* cp, int width, int height, int sampleIdx)
{
	prepareHbaoData(cp, width, height);
	drawLinearDepth(cp, width, height, sampleIdx); // linearize

	{
		if (blur)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			if (samples > 1)
			{
				glEnable(GL_SAMPLE_MASK);
				glSampleMaski(0, 1 << sampleIdx);
			}
		}

		if (USE_AO_SPECIALBLUR && blur)
		{
			hbao_calc_blur->begin();
		}
		else
		{
			hbao_calc->begin();
		}

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, hbao_ubo);
		glNamedBufferSubDataEXT(hbao_ubo, 0, sizeof(HBAOData), &hbaoUbo);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D,
				textures.scene_depthlinear);
		glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D,
				textures.hbao_randomview[sampleIdx]);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	if (blur)
		drawHbaoBlur(width, height, sampleIdx);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_SAMPLE_MASK);
	glSampleMaski(0, ~0);

	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

//----------------------------------------------------

void SSAO::drawHbaoCacheAware(camPar* cp, int width, int height, int sampleIdx)
{
	int quarterWidth = ((width + 3) / 4);
	int quarterHeight = ((height + 3) / 4);

	prepareHbaoData(cp, width, height);
	drawLinearDepth(cp, width, height, sampleIdx); // linearize

	// ------ berechne normalen und speichere in einen FBO
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.viewnormal);

		viewnormal->begin();

		glUniform4fv(0, 1, &hbaoUbo.projInfo[0]);
		glUniform1i(1, hbaoUbo.projOrtho);
		glUniform2fv(2, 1, &hbaoUbo.InvFullResolution[0]);

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D,
				textures.scene_depthlinear);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	}

	// ------ deinterleave
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);
		glViewport(0, 0, quarterWidth, quarterHeight);

		hbao2_deinterleave->begin();
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D,
				textures.scene_depthlinear);

		for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i += NUM_MRT)
		{
			glUniform4f(0, float(i % 4) + 0.5f, float(i / 4) + 0.5f,
					hbaoUbo.InvFullResolution.x, hbaoUbo.InvFullResolution.y);

			for (int layer = 0; layer < NUM_MRT; layer++)
				glFramebufferTexture(GL_FRAMEBUFFER,
						GL_COLOR_ATTACHMENT0 + layer,
						textures.hbao2_depthview[i + layer], 0);

			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}

	// ------ ssao calc
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
		glViewport(0, 0, quarterWidth, quarterHeight);

		if (USE_AO_SPECIALBLUR)
		{
			hbao2_calc_blur->begin();
		}
		else
		{
			hbao2_calc->begin();
		}

		glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D,
				textures.scene_viewnormal);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, hbao_ubo);
		glNamedBufferSubDataEXT(hbao_ubo, 0, sizeof(HBAOData), &hbaoUbo);

#if USE_AO_LAYERED_SINGLEPASS
		// instead of drawing to each layer individually
		// we draw all layers at once, and use image writes to update the array texture
		// this buys additional performance :)

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY,
				textures.hbao2_deptharray);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
		glBindImageTexture( 0, textures.hbao2_resultarray, 0, GL_TRUE, 0, GL_WRITE_ONLY,
				USE_AO_SPECIALBLUR ? GL_RG16F : GL_R8);
#endif
		glDrawArrays(GL_TRIANGLES, 0, 3 * HBAO_RANDOM_ELEMENTS);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif
#else
		for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++)
		{
			glUniform2f(0, float(i % 4) + 0.5f, float(i / 4) + 0.5f);
			glUniform4fv(1, 1, hbaoRandom[i].get_value());

			glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.hbao2_depthview[i]);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					textures.hbao2_resultarray, 0, i);

			glDrawArrays(GL_TRIANGLES,0,3);
		}
#endif
	}

	// ------ reinterleave
	{
		if (blur)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			if (samples > 1)
			{
				glEnable(GL_SAMPLE_MASK);
				glSampleMaski(0, 1 << sampleIdx);
			}
		}
		glViewport(0, 0, width, height);

		if (USE_AO_SPECIALBLUR && blur)
		{
			hbao2_reinterleave_blur->begin();
		}
		else
		{
			hbao2_reinterleave->begin();
		}

		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY,
				textures.hbao2_resultarray);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, 0);
	}

	// --------- blur
	if (blur)
		drawHbaoBlur(width, height, sampleIdx);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_SAMPLE_MASK);
	glSampleMaski(0, ~0);

	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

//----------------------------------------------------

void SSAO::copyFbo(camPar* cp)
{
	int width = int(cp->actFboSize.x);
	int height = int(cp->actFboSize.y);

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos.scene);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, lastBoundFbo);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

//----------------------------------------------------

void SSAO::bind()
{
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
}

//----------------------------------------------------

void SSAO::clear(glm::vec4 clearCol)
{
	glClearBufferfv(GL_COLOR, 0, &clearCol.x);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

//----------------------------------------------------

void SSAO::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

//----------------------------------------------------

void SSAO::proc(camPar* cp)
{
	int width = int(cp->actFboSize.x);
	int height = int(cp->actFboSize.y);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glBindVertexArray(defaultVAO); // create VAO, assign the name and bind that array

	for (int sample = 0; sample < samples; sample++)
	{
		switch (algorithm)
		{
		case ALGORITHM_HBAO_CLASSIC:
			drawHbaoClassic(cp, width, height, sample);
			break;
		case ALGORITHM_HBAO_CACHEAWARE:
			drawHbaoCacheAware(cp, width, height, sample);
			break;
		default:
			break;
		}
	}

	glBindVertexArray(0);     // create VAO, assign the name and bind that array
}

//----------------------------------------------------

void SSAO::drawBlit(camPar* cp, bool copyDepth)
{
	// blit depth
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos.scene);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastBoundFbo);

	glBlitFramebuffer(0, 0, int(cp->actFboSize.x), int(cp->actFboSize.y), 0, 0,
			int(cp->actFboSize.x), int(cp->actFboSize.y),
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	if (copyDepth)
		glBlitFramebuffer(0, 0, int(cp->actFboSize.x), int(cp->actFboSize.y), 0,
				0, (cp->actFboSize.x), int(cp->actFboSize.y),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

//----------------------------------------------------

void SSAO::drawAlpha(camPar* cp, float alpha)
{
	unbind();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	texShdr->begin();
	texShdr->setIdentMatrix4fv("m_pvm");
	texShdr->setUniform1i("tex", 0);
	texShdr->setUniform1f("alpha", alpha);
	texShdr->setUniform2f("scr_size", cp->actFboSize.x, cp->actFboSize.y);
	texShdr->setUniform1i("nrSamples", samples);

	glActiveTexture(GL_TEXTURE0);

	if (samples > 1)
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures.scene_color);
	else
		glBindTexture(GL_TEXTURE_2D, textures.scene_color);

	quad->draw();

	texShdr->end();
}

//---------------------------------------------------------

void SSAO::checkFboStatus()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if ( GL_FRAMEBUFFER_COMPLETE != status)
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cerr << "FBO Error: Attachment Point Unconnected" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cerr << "FBO Error: Missing Attachment" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			std::cerr << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"
					<< std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			std::cerr << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"
					<< std::endl;
			break;
			//                case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			//                    std::cout << "Framebuffer Object %d Error: Dimensions do not match" << std::endl;
			//                    break;
			//                case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
			//                    std::cout << "Framebuffer Object %d Error: Formats" << std::endl;
			//                    break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cerr
					<< "Framebuffer Object %d Error: Unsupported Framebuffer Configuration"
					<< std::endl;
			break;
		default:
			break;
		}
	}
	getGlError();
}

//----------------------------------------------------

SSAO::~SSAO()
{
}

}
