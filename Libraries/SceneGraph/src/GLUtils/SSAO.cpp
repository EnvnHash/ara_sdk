//
// SSAO.cpp
//
//  Created by Sven Hahne on 19.08.14.
//
//  in the nvidia-settings the option antialiasing has to be set to "use
//  Application Settings" !!!!! if not GL_INALID_OPERATION on copying (blitting)
//  the FBOs

// #define USE_GLSG_FBO

#include "SSAO.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {
SSAO::SSAO(GLBase* glbase, uint inFboWidth, uint inFboHeight, AlgorithmType _algorithm, bool _blur, float _intensity,
           float _blurSharpness)
    : algorithm(_algorithm), m_glbase(glbase), bias(0.1f), blur(_blur), blurSharpness(_blurSharpness), lastBoundFbo(0),
      intensity(_intensity), inited(false), radius(0.02f), fboWidth(inFboWidth), fboHeight(inFboHeight) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    samples = m_glbase->getNrSamples();

    // init default VAO
    glGenVertexArrays(1, &defaultVAO);
    glBindVertexArray(defaultVAO);

    shCol   = &m_glbase->shaderCollector();
    texShdr = m_glbase->shaderCollector().getStdTexAlpha(samples > 1 ? true : false);

    if (samples > 1)
        texNoAlpha = m_glbase->shaderCollector().getStdTexMulti();
    else
        texNoAlpha = m_glbase->shaderCollector().getStdTex();

    initShaders();
    initMisc();

    glBindVertexArray(0);

    initFramebuffers(inFboWidth, inFboHeight, samples);

    quad = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f});
}

void SSAO::initShaders() {
    com += "#define AO_RANDOMTEX_SIZE " + std::to_string(AO_RANDOMTEX_SIZE) + "\n";
    com += STRINGIFY(
		struct HBAOData {\n 
			float RadiusToScreen;\n        // radius
			float R2;\n// 1/radius
			float NegInvR2;\n// radius * radius
			float NDotVBias;\n 
			vec2 InvFullResolution;\n
			vec2 InvQuarterResolution;\n 
			float AOMultiplier;\n 
			float PowExponent;\n 
			vec2 _pad0;\n 
			vec4 projInfo;\n 
			vec2 projScale;\n 
			int projOrtho;\n 
			int _pad1;\n 
			vec4 float2Offsets[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];\n 
			vec4 jitters[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];\n
		};\n
	);

    shdr_Header = "#version 430\n#define AO_LAYERED " + std::to_string(USE_AO_LAYERED_SINGLEPASS) + "\n";

    initFullScrQuad();
    initBilateralblur();

    std::string frag = initDepthLinearize(0);
    depth_linearize  = shCol->add("SSAODepth_linearize", fullScrQuad.c_str(), frag.c_str());

    frag                 = initDepthLinearize(1);
    depth_linearize_msaa = shCol->add("SSAODepth_linearize_msaa", fullScrQuad.c_str(), frag.c_str());

    initViewNormal();

    frag      = initHbaoCalc(0, 0);
    hbao_calc = shCol->add("SSAOHBAO_Calc", fullScrQuad.c_str(), frag.c_str());

    frag           = initHbaoCalc(1, 0);
    hbao_calc_blur = shCol->add("SSAOHBAO_Calc_Blur", fullScrQuad.c_str(), frag.c_str());

    frag      = initHbaoBlur(0);
    hbao_blur = shCol->add("SSAOHBAO_Blur", fullScrQuad.c_str(), frag.c_str());

    frag       = initHbaoBlur(1);
    hbao_blur2 = shCol->add("SSAOHBAO_BlurMsaa", fullScrQuad.c_str(), frag.c_str());

    frag = initHbaoCalc(0, 1);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
    hbao2_calc = shCol->add("SSAOHBAO2_Calc", fullScrQuad.c_str(), fullScrQuadGeo.c_str(), frag.c_str());
#else
    hbao2_calc = m_shCol->add("SSAOHBAO2_Calc", fullScrQuad.c_str(), frag.c_str());
#endif

    frag = initHbaoCalc(1, 1);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
    hbao2_calc_blur = shCol->add("SSAOHBAO2_Calc_Blur", fullScrQuad.c_str(), fullScrQuadGeo.c_str(), frag.c_str());
#else
    hbao2_calc_blur = m_shCol->add("SSAOHBAO2_Calc_Blur", fullScrQuad.c_str(), frag.c_str());
#endif

    initDeinterleave();

    frag               = initReinterleave(0);
    hbao2_reinterleave = shCol->add("SSAOHBAO2_Reinterleave", fullScrQuad.c_str(), frag.c_str());

    frag                    = initReinterleave(1);
    hbao2_reinterleave_blur = shCol->add("SSAOHBAO2_ReinterleaveBlur", fullScrQuad.c_str(), frag.c_str());

    debugDepth = initDebugDepth();
}

ara::Shaders* SSAO::initDebugDepth() {
    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 2) in vec2 texCoord;
                                 uniform mat4 m_pvm; out vec2 tex_coord; void main() {
                                     tex_coord   = texCoord;
                                     gl_Position = position;
                                 });

    vert = "// debug depth shader, vert\n" + shdr_Header + vert;

    std::string frag = STRINGIFY(in vec4 col; layout(location = 0) out vec4 color; uniform sampler2D tex;
                                 in vec2 tex_coord; void main() { color = texture(tex, tex_coord) * 0.1; });
    frag             = "// debug depth shader, frag\n" + shdr_Header + frag;

    return shCol->add("SSaoDebugDepth", vert.c_str(), frag.c_str());
}

void SSAO::initFullScrQuad() {
    std::string vert = STRINGIFY(out vec2 texCoord;\n
			\n void main() {
        \n uint idx = gl_VertexID % 3;
        \n  // allows rendering multiple fullscreen triangles
            vec4 pos         = vec4(\n(float(idx & 1U)) * 4.0 - 1.0,\n(float((idx >> 1U) & 1U)) * 4.0 - 1.0,\n 0, 1.0);
        \n       gl_Position = pos;
        \n       texCoord    = pos.xy * 0.5 + 0.5;
        \n
    });

    fullScrQuad = "// SSAO Full Screen Quad vertex shader\n" + shdr_Header + com + vert;

    //------------------------------------------------------------------------

    std::string add_shdr_header = "#extension GL_NV_geometry_shader_passthrough : enable\n";

    std::string geo = STRINGIFY(
        layout(triangles) in;\n layout(triangle_strip, max_vertices = 3) out;\n

            in   Inputs { vec2 texCoord; } IN[];
        out vec2 texCoord;

        void main() {
            for (int i = 0; i < 3; i++) {
                texCoord       = IN[i].texCoord;
                gl_Layer       = gl_PrimitiveIDIn;
                gl_PrimitiveID = gl_PrimitiveIDIn;
                gl_Position    = gl_in[i].gl_Position;
                EmitVertex();
            }
        });

    fullScrQuadGeo = "// SSAO Full Screen Quad geometry shader\n" + shdr_Header + add_shdr_header + com + geo;
}

void SSAO::initBilateralblur() {
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
			vec4 c = texture( texSource, uv );\n
			float d = texture( texLinearDepth, uv).x;\n

			const float BlurSigma = float(KERNEL_RADIUS) * 0.5;\n
			const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);\n

			float ddiff = (d - center_d) * g_Sharpness;\n
			float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);\n
			w_total += w;\n

			return c*w;\n
		}\n

		void main()\n
		{\n
			vec4 center_c = texture( texSource, texCoord );\n
			float center_d = texture( texLinearDepth, texCoord).x;\n

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

    bilateralblur = shCol->add("SSAOBilateralblur", fullScrQuad.c_str(), frag.c_str());
}

std::string SSAO::initDepthLinearize(int msaa) {
    std::string add_shdr_Header = "#define DEPTHLINEARIZE_MSAA ";
    add_shdr_Header += std::to_string(msaa);
    add_shdr_Header += "\n";
    add_shdr_Header +=
        "#ifndef DEPTHLINEARIZE_USEMSAA \n#define DEPTHLINEARIZE_USEMSAA 0 "
        "\n#endif\n";
    add_shdr_Header +=
        "#if DEPTHLINEARIZE_MSAA\nlayout(location=1) uniform int "
        "sampleIndex;\n layout(binding=0)  uniform sampler2DMS "
        "inputTexture;\n ";
    add_shdr_Header += "#else\n layout(binding=0)  uniform sampler2D inputTexture;\n#endif\n";

        std::string frag = STRINGIFY(
		layout(location=0) uniform vec4 clipInfo;\n // z_n * z_f,  z_n - z_f,  z_f, perspective = 1 : 0
		layout(location=0,index=0) out float out_Color;\n

		float reconstructCSZ(float d, vec4 clipInfo) {
        \n if (clipInfo[3] != 0) {
            \n return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));
            \n
        }
        else {
            \n return (clipInfo[1] + clipInfo[2] - d * clipInfo[1]);
            \n
        }
        \n
		}

		void main() {\n);

        frag += "#if DEPTHLINEARIZE_MSAA\n";
        frag +=
            "float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), "
            "sampleIndex).x;\n";
        frag += "#else\n";
        frag +=
            "float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), "
            "0).x;\n";
        frag += "#endif\n";
        frag += "out_Color = reconstructCSZ(depth, clipInfo);\n}";

        frag = "// SSAO Depth Linearize vertex shader\n" + shdr_Header + add_shdr_Header + frag;

        return frag;
}


void SSAO::initViewNormal()
{
        std::string frag = STRINGIFY(
            in vec2 texCoord;\n layout(location = 0) uniform vec4 projInfo;\n layout(location = 1) uniform int projOrtho;\n layout(location = 2) uniform vec2 InvFullResolution;\n layout(binding = 0) uniform sampler2D texLinearDepth;\n layout(location = 0, index = 0) out vec4 out_Color;\n
		\n vec3 UVToView(vec2 uv, float eye_z)\n {
                \n return vec3((uv * projInfo.xy + projInfo.zw) * (projOrtho != 0 ? 1.0 : eye_z), eye_z);
                \n
            }\n
		\n vec3 FetchViewPos(vec2 UV)\n {
                \n float ViewDepth = textureLod(texLinearDepth, UV, 0).x;
                \n return UVToView(UV, ViewDepth);
                \n
            }\n
		\n vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)\n {
                \n vec3 V1 = Pr - P;
                \n vec3 V2 = P - Pl;
                \n return (dot(V1, V1) < dot(V2, V2)) ? V1 : V2;
                \n
            }\n
		\n vec3 ReconstructNormal(vec2 UV, vec3 P)\n {
                \n vec3 Pr = FetchViewPos(UV + vec2(InvFullResolution.x, 0));
                \n vec3 Pl = FetchViewPos(UV + vec2(-InvFullResolution.x, 0));
                \n vec3 Pt = FetchViewPos(UV + vec2(0, InvFullResolution.y));
                \n vec3 Pb = FetchViewPos(UV + vec2(0, -InvFullResolution.y));
                \n return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
                \n
            }\n
		\n void main() {
                \n vec3 P         = FetchViewPos(texCoord);
                \n vec3 N         = ReconstructNormal(texCoord, P);
                \n      out_Color = vec4(N * 0.5 + 0.5, 0);
                \n
            });

        frag = "// SSAO View Normal vertex shader\n" + shdr_Header + frag;

        viewnormal = shCol->add("SSAOViewNormal", fullScrQuad.c_str(), frag.c_str());
}

std::string SSAO::initHbaoCalc(int _blur, int deinterl)
{
        std::string add_shdr_Header = "#define AO_DEINTERLEAVED " + std::to_string(deinterl) + "\n";
        add_shdr_Header += "#define AO_BLUR " + std::to_string(_blur) + "\n";
        // The pragma below is critical for optimal performance
        // in this fragment shader to let the shader compiler
        // fully optimize the maths and batch the texture fetches
        // optimally
        add_shdr_Header +=
            "#pragma optionNV(unroll all) \n#ifndef AO_DEINTERLEAVED\n#define "
            "AO_DEINTERLEAVED 1\n#endif\n";
        add_shdr_Header +=
            "#ifndef AO_BLUR\n#define AO_BLUR 1\n#endif\n#ifndef "
            "AO_LAYERED\n#define AO_LAYERED 1\n#endif\n#define M_PI "
            "3.14159265f\n";

        // tweakables
        std::string frag = STRINGIFY(
		const float NUM_STEPS = 4;\n
		const float NUM_DIRECTIONS = 8;\n // texRandom/g_Jitter initialization depends on this
		layout(std140,binding=0) uniform controlBuffer { HBAOData control; };\n);

        frag += "#if AO_DEINTERLEAVED\n";

        frag += "#if AO_LAYERED\n";
        frag += STRINGIFY(
		vec2 g_Float2Offset = control.float2Offsets[gl_PrimitiveID].xy;\n
		vec4 g_Jitter = control.jitters[gl_PrimitiveID];\n

		layout(binding=0) uniform sampler2DArray texLinearDepth;\n
		layout(binding=1) uniform sampler2D texViewNormal;\n

		vec3 getQuarterCoord(vec2 UV){\n
			return vec3(UV,float(gl_PrimitiveID));\n
		}\n
	);
        frag += "#if AO_LAYERED == 1\n";
        frag += "#if AO_BLUR\n";
        frag += "layout(binding=0,rg16f) uniform image2DArray imgOutput;\n";
        frag += "#else\n";
        frag += "layout(binding=0,r8) uniform image2DArray imgOutput;\n";
        frag += "#endif\n";
        frag += STRINGIFY(void outputColor(vec4 color) {
            \n imageStore(imgOutput, ivec3(ivec2(gl_FragCoord.xy), gl_PrimitiveID), color);
            \n
        }\n);
        frag += "#else\n";
        frag += STRINGIFY(layout(location = 0, index = 0) out vec4 out_Color;\n
		\n void outputColor(vec4 color) {
            \n out_Color = color;
            \n
        }\n);
        frag += "#endif\n";
        frag += "#else\n";
        frag += STRINGIFY(
		layout(location=0) uniform vec2 g_Float2Offset;\n 
		layout(location=1) uniform vec4 g_Jitter;\n 
		\n 
		layout(binding=0) uniform sampler2D texLinearDepth;\n 
		layout(binding=1) uniform sampler2D texViewNormal;\n 
		\n 
		vec2 getQuarterCoord(vec2 UV){\n
			return UV;\n 
		}\n
		\n
		layout(location=0,index=0) out vec4 out_Color;\n
		\n
		void outputColor(vec4 color) {\n
			out_Color = color;\n 
		}\n
	);
        frag += "#endif\n";

        frag += "#else\n";
        frag += STRINGIFY(
            layout(binding = 0) uniform sampler2D texLinearDepth;\n layout(binding = 1) uniform sampler2D texRandom;\n
		\n layout(location = 0, index = 0) out vec4 out_Color;\n
		\n void outputColor(vec4 color) {
                \n out_Color = color;
                \n
            }\n);
        frag += "#endif\n";
        frag += STRINGIFY(in vec2 texCoord;\n

                              vec3 UVToView(vec2 uv, float eye_z) {
                                  \n return vec3((uv * control.projInfo.xy + control.projInfo.zw) *
                                                     (control.projOrtho != 0 ? 1. : eye_z),
                                                 eye_z);
                                  \n
                              }\n);

        frag += "#if AO_DEINTERLEAVED\n";

        frag += STRINGIFY(vec3 FetchQuarterResViewPos(vec2 UV) {
            \n float ViewDepth = textureLod(texLinearDepth, getQuarterCoord(UV), 0).x;
            \n return UVToView(UV, ViewDepth);
            \n
        }\n);

        frag += "#else //AO_DEINTERLEAVED\n";

        frag += STRINGIFY(
            vec3 FetchViewPos(vec2 UV) {
                \n float ViewDepth = textureLod(texLinearDepth, UV, 0).x;
                \n return UVToView(UV, ViewDepth);
                \n
            }\n

                vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl) {
                    \n vec3 V1 = Pr - P;
                    \n vec3 V2 = P - Pl;
                    \n return (dot(V1, V1) < dot(V2, V2)) ? V1 : V2;
                    \n
                }\n

                    vec3 ReconstructNormal(vec2 UV, vec3 P) {
                        \n vec3 Pr = FetchViewPos(UV + vec2(control.InvFullResolution.x, 0));
                        \n vec3 Pl = FetchViewPos(UV + vec2(-control.InvFullResolution.x, 0));
                        \n vec3 Pt = FetchViewPos(UV + vec2(0, control.InvFullResolution.y));
                        \n vec3 Pb = FetchViewPos(UV + vec2(0, -control.InvFullResolution.y));
                        \n return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
                        \n
                    }\n);

        frag += "#endif //AO_DEINTERLEAVED\n";

        //----------------------------------------------------------------------------------
        frag += STRINGIFY(
				float Falloff(float DistanceSquare) {
            \n
                    // 1 scalar mad instruction
                    return DistanceSquare *
                    control.NegInvR2 +
                1.0;
            \n
				}\n

					//----------------------------------------------------------------------------------
					// P = view-space position at the kernel center
					// N = view-space normal at the kernel center
					// S = view-space position of the current sample
					//----------------------------------------------------------------------------------
					float ComputeAO(vec3 P, vec3 N, vec3 S) {
            \n vec3  V     = S - P;
            \n float VdotV = dot(V, V);
            \n float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);
            \n

                // Use saturate(x) instead of max(x,0.f) because that is faster
                // on Kepler
                return clamp(NdotV - control.NDotVBias, 0, 1) *
                clamp(Falloff(VdotV), 0, 1);
            \n }\n

					//----------------------------------------------------------------------------------
					vec2 RotateDirection(vec2 Dir, vec2 CosSin) {
            \n return vec2(Dir.x * CosSin.x - Dir.y * CosSin.y,\n Dir.x * CosSin.y + Dir.y * CosSin.x);
            \n }\n

					//----------------------------------------------------------------------------------
					vec4 GetJitter() {\n);
            frag += "#if AO_DEINTERLEAVED\n";
            // Get the current jitter vector from the per-pass constant m_buffer
            frag += "return g_Jitter;\n";
            frag += "#else\n";
            // (cos(Alpha),sin(Alpha),rand1,rand2)
            frag +=
                "return textureLod( texRandom, (gl_FragCoord.xy / "
                "AO_RANDOMTEX_SIZE), 0);\n";
            frag += "#endif\n}";

            //----------------------------------------------------------------------------------
            frag +=
                "float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, "
                "vec4 Rand, vec3 ViewPosition, vec3 ViewNormal) {\n";
            frag += "#if AO_DEINTERLEAVED\n";
            frag += "RadiusPixels /= 4.0;\n";
            frag += "#endif\n";

            // Divide by NUM_STEPS+1 so that the farthest samples are not fully
            // attenuated
                                        frag += STRINGIFY(float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);\n

					const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;\n
					float AO = 0;\n

					for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
					{
                \n float Angle = Alpha * DirectionIndex;
                \n

                    // Compute normalized 2D direction
                    vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);
                \n

                    // Jitter starting sample within the first step
                    float RayPixels = (Rand.z * StepSizePixels + 1.0);
                \n

                    for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex) {	\n);
                    frag += "#if AO_DEINTERLEAVED\n";
                    frag += STRINGIFY(
                        vec2 SnappedUV = round(RayPixels * Direction) * control.InvQuarterResolution + FullResUV;\n vec3 S = FetchQuarterResViewPos(SnappedUV);\n);
                    frag += "#else\n";
                    frag += STRINGIFY(
                        vec2 SnappedUV = round(RayPixels * Direction) * control.InvFullResolution + FullResUV;\n vec3 S = FetchViewPos(SnappedUV);\n);
                    frag += "#endif\n";
                                                        frag += STRINGIFY(RayPixels += StepSizePixels;\n

							AO += ComputeAO(ViewPosition, ViewNormal, S);\n
                }
                \n
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
            frag +=
                "outputColor(vec4(pow(AO, control.PowExponent), "
                "ViewPosition.z, 0, 0));\n";
            frag += "#else\n";
            frag += "outputColor(vec4(pow(AO, control.PowExponent)));\n";
            frag += "#endif\n}";

            frag = "//SSAO HBAO Calc \n" + shdr_Header + add_shdr_Header + com + frag;

            return frag;
				}

std::string SSAO::initHbaoBlur(int _blur)
{
            std::string add_shdr_Header = "#define AO_BLUR_PRESENT " + std::to_string(_blur) + "\n";

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
		float BlurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total) {
                \n vec2  aoz = texture2D(texSource, uv).xy;
                \n float c   = aoz.x;
                \n float d   = aoz.y;
                \n

                    const float BlurSigma   = float(KERNEL_RADIUS) * 0.5;
                \n const float  BlurFalloff = 1.0 / (2.0 * BlurSigma * BlurSigma);
                \n

                    float ddiff = (d - center_d) * g_Sharpness;
                \n float  w     = exp2(-r * r * BlurFalloff - ddiff * ddiff);
                \n        w_total += w;
                \n

                    return c *
                    w;
                \n
		}\n

		void main()\n {
                vec2     aoz      = texture2D(texSource, texCoord).xy;
                \n float center_c = aoz.x;
                \n float center_d = aoz.y;
                \n
			\n float             c_total  = center_c;
                \n float w_total  = 1.0;
                \n
			\n for (float r = 1; r <= KERNEL_RADIUS; ++r)\n {
                    vec2 uv = texCoord + g_InvResolutionDirection * r;
                    \n   c_total += BlurFunction(uv, r, center_c, center_d, w_total);
                    \n
                }
                \n
			\n for (float r = 1; r <= KERNEL_RADIUS; ++r)\n {
                    vec2 uv = texCoord - g_InvResolutionDirection * r;
                    \n   c_total += BlurFunction(uv, r, center_c, center_d, w_total);
                    \n
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

void SSAO::initDeinterleave()
{
                std::string frag = STRINGIFY(layout(location = 0) uniform vec4 info;  // xy
                                             vec2 uvOffset = info.xy; vec2 invResolution = info.zw;

                                             layout(binding = 0) uniform sampler2D     texLinearDepth;
                                             layout(location = 0, index = 0) out float out_Color[8];\n

                                             void main() {
                                                 vec2 uv = floor(gl_FragCoord.xy) * 4.0 + uvOffset + 0.5;
                                                 uv *= invResolution;

                                                 vec4 S0 = textureGather(texLinearDepth, uv, 0);
                                                 vec4 S1 = textureGatherOffset(texLinearDepth, uv, ivec2(2, 0), 0);

                                                 out_Color[0] = S0.w;
                                                 out_Color[1] = S0.z;
                                                 out_Color[2] = S1.w;
                                                 out_Color[3] = S1.z;
                                                 out_Color[4] = S0.x;
                                                 out_Color[5] = S0.y;
                                                 out_Color[6] = S1.x;
                                                 out_Color[7] = S1.y;
                                             });

                frag = "// SSAO Deinterleave shader\n" + shdr_Header + frag;

                hbao2_deinterleave = shCol->add("SSAO_HBAO2_deinterleave", fullScrQuad.c_str(), frag.c_str());
}

std::string SSAO::initReinterleave(int _blur)
{
                std::string add_shdr_Header = "#define AO_BLUR " + std::to_string(_blur) + "\n";
                std::string frag            = "#ifndef AO_BLUR\n";
                frag += "#define AO_BLUR 1\n";
                frag += "#endif\n";

        frag += STRINGIFY(
		layout(binding=0) uniform sampler2DArray texResultsArray;\n
		layout(location=0,index=0) out vec4 out_Color;\n

		void main() {
                    \n ivec2 FullResPos    = ivec2(gl_FragCoord.xy);
                    \n ivec2 Offset        = FullResPos & 3;
                    \n int   SliceId       = Offset.y * 4 + Offset.x;
                    \n ivec2 QuarterResPos = FullResPos >> 2;\n
	);

                    frag += "#if AO_BLUR\n";
                    frag +=
                        "out_Color = vec4(texelFetch( texResultsArray, "
                        "ivec3(QuarterResPos, SliceId), 0).xy,0,0);\n";
                    frag += "#else\n";
                    frag +=
                        "out_Color = vec4(texelFetch( texResultsArray, "
                        "ivec3(QuarterResPos, SliceId), 0).x);\n";
                    frag += "#endif\n}";

                    frag = "// SSAO Reinterleave Shader frag shader\n" + shdr_Header + add_shdr_Header + frag;

                    return frag;
}


bool SSAO::initMisc()
{
                    MTRand rng;
                    float  numDir = 8;  // keep in sync to glsl
                    rng.seed((unsigned)0);

                    signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES * 4];

                    for (int i = 0; i < HBAO_RANDOM_ELEMENTS * MAX_SAMPLES; i++) {
                        float Rand1 = rng.randExc();
                        float Rand2 = rng.randExc();

                        // Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
                        float Angle     = 2.f * float(M_PI) * Rand1 / numDir;
                        hbaoRandom[i].x = cosf(Angle);
                        hbaoRandom[i].y = sinf(Angle);
                        hbaoRandom[i].z = Rand2;
                        hbaoRandom[i].w = 0;
#define SCALE ((1 << 15))
                        hbaoRandomShort[i * 4 + 0] = (signed short)(SCALE * hbaoRandom[i].x);
                        hbaoRandomShort[i * 4 + 1] = (signed short)(SCALE * hbaoRandom[i].y);
                        hbaoRandomShort[i * 4 + 2] = (signed short)(SCALE * hbaoRandom[i].z);
                        hbaoRandomShort[i * 4 + 3] = (signed short)(SCALE * hbaoRandom[i].w);
#undef SCALE
                    }

#ifdef USE_GLSG_FBO
                    textures.hbao_random.allocate3D(HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES, GL_RGBA16_SNORM,
                                                    GL_RGBA, GL_TEXTURE_2D_ARRAY, GL_SHORT);
                    textures.hbao_random.setFiltering(GL_NEAREST, GL_NEAREST);
#else
                    newTexture(textures.hbao_random);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao_random);
#ifndef ARA_USE_GLES31
                    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE,
                                   MAX_SAMPLES);
#endif
                    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES,
                                    GL_RGBA, GL_SHORT, hbaoRandomShort);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#endif

                    for (int i = 0; i < MAX_SAMPLES; i++) {
                        newTexture(textures.hbao_randomview[i]);
#ifndef ARA_USE_GLES31
#ifdef USE_GLSG_FBO
                        glTextureView(textures.hbao_randomview[i], GL_TEXTURE_2D, textures.hbao_random.getId(),
                                      GL_RGBA16_SNORM, 0, 1, i, 1);
#else
                        glTextureView(textures.hbao_randomview[i], GL_TEXTURE_2D, textures.hbao_random, GL_RGBA16_SNORM,
                                      0, 1, i, 1);
#endif
#endif
                        glBindTexture(GL_TEXTURE_2D, textures.hbao_randomview[i]);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    if (hbao_ubo) glDeleteBuffers(1, &hbao_ubo);

                    glGenBuffers(1, &hbao_ubo);
#ifndef ARA_USE_GLES31
                    glNamedBufferStorageEXT(hbao_ubo, sizeof(HBAOData), NULL, GL_DYNAMIC_STORAGE_BIT);
#endif

                    return true;
}

bool SSAO::initFramebuffers(int width, int height, int samples)
{
                    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);

#ifndef USE_GLSG_FBO
                    if (samples > 1) {
                        newTexture(textures.scene_color);
                        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures.scene_color);
                        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height,
                                                  GL_FALSE);
                        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

                        newTexture(textures.scene_depthstencil);
                        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures.scene_depthstencil);
                        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH24_STENCIL8, width,
                                                  height, GL_FALSE);
                        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
                    } else {
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
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
                    if (samples > 1) {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                                               textures.scene_color, 0);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE,
                                               textures.scene_depthstencil, 0);
                    } else {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                               textures.scene_color, 0);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                               textures.scene_depthstencil, 0);
                    }

                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

#else
                    if (samples > 1)
                        sceneFbo = make_unique<FBO>(FboInitParams{width, height, 1,GL_RGBA8, GL_TEXTURE_2D_MULTISAMPLE, true, 1, 1,
                                                    samples, GL_REPEAT, false});
                    else
                        sceneFbo = make_unique<FBO>(FboInitParams{width, height, 1, GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, samples,
                                                    GL_REPEAT, false});

#endif

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
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                           textures.scene_depthlinear, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

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
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                           textures.scene_viewnormal, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // hbao
#if USE_AO_SPECIALBLUR
                    GLenum formatAO   = GL_RG16F;
                    GLint  swizzle[4] = {GL_RED, GL_GREEN, GL_ZERO, GL_ZERO};
#else
                    GLenum formatAO   = GL_R8;
                    GLint  swizzle[4] = {GL_RED, GL_RED, GL_RED, GL_RED};
#endif

                    newTexture(textures.hbao_result);
                    glBindTexture(GL_TEXTURE_2D, textures.hbao_result);
                    glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
#ifndef ARA_USE_GLES31
                    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
#endif
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    newTexture(textures.hbao_blur);
                    glBindTexture(GL_TEXTURE_2D, textures.hbao_blur);
                    glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
#ifndef ARA_USE_GLES31
                    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
#endif
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    newFramebuffer(fbos.hbao_calc);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures.hbao_result,
                                           0);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textures.hbao_blur, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // interleaved hbao
                    int quarterWidth  = ((width + 3) / 4);
                    int quarterHeight = ((height + 3) / 4);

                    newTexture(textures.hbao2_deptharray);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao2_deptharray);
                    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, quarterWidth, quarterHeight, HBAO_RANDOM_ELEMENTS);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

                    for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++) {
                        newTexture(textures.hbao2_depthview[i]);
#ifndef ARA_USE_GLES31
                        glTextureView(textures.hbao2_depthview[i], GL_TEXTURE_2D, textures.hbao2_deptharray, GL_R32F, 0,
                                      1, i, 1);
#endif
                        glBindTexture(GL_TEXTURE_2D, textures.hbao2_depthview[i]);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    newTexture(textures.hbao2_resultarray);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao2_resultarray);
                    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, formatAO, quarterWidth, quarterHeight, HBAO_RANDOM_ELEMENTS);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

                    GLenum drawbuffers[NUM_MRT];
                    for (int i = 0; i < NUM_MRT; i++) drawbuffers[i] = GL_COLOR_ATTACHMENT0 + i;

                    newFramebuffer(fbos.hbao2_deinterleave);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);
                    glDrawBuffers(NUM_MRT, drawbuffers);
                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

                    newFramebuffer(fbos.hbao2_calc);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
                    // this s_fbo will not have any attachments and therefore
                    // requires rasterizer to be configured, through default
                    // parameters
                    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, quarterWidth);
                    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, quarterHeight);
#endif

#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_GS
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                           textures.hbao2_resultarray, 0);
#endif
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

                    return true;
}

void SSAO::prepareHbaoData(CameraSet* cs, int width, int height)
{
                    // projection
                    const float* P = &cs->getProjectionMatr()[0][0];

                    hbaoUbo.projOrtho = 0;
                    hbaoUbo.projInfo  = glm::vec4(2.0f / (P[4 * 0 + 0]),                  // (x) * (R - L)/N
                                                  2.0f / (P[4 * 1 + 1]),                  // (y) * (T - B)/N
                                                  -(1.0f - P[4 * 2 + 0]) / P[4 * 0 + 0],  // L/N
                                                  -(1.0f + P[4 * 2 + 1]) / P[4 * 1 + 1]   // B/N
                     );

                    float projScale = float(height) / (tanf(cs->getFov() * 0.5f) * 2.0f);

                    // radius
                    hbaoUbo.R2             = radius * radius;
                    hbaoUbo.NegInvR2       = -1.0f / hbaoUbo.R2;
                    hbaoUbo.RadiusToScreen = radius * 0.5f * projScale;

                    // ao
                    hbaoUbo.PowExponent  = std::max<float>(intensity, 0.0f);
                    hbaoUbo.NDotVBias    = std::min<float>(std::max<float>(0.0f, bias), 1.0f);
                    hbaoUbo.AOMultiplier = 1.0f / (1.0f - hbaoUbo.NDotVBias);

                    // resolution
                    int quarterWidth  = ((width + 3) / 4);
                    int quarterHeight = ((height + 3) / 4);

                    hbaoUbo.InvQuarterResolution = glm::vec2(1.0f / float(quarterWidth), 1.0f / float(quarterHeight));
                    hbaoUbo.InvFullResolution    = glm::vec2(1.0f / float(width), 1.0f / float(height));

#if USE_AO_LAYERED_SINGLEPASS
                    for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++) {
                        hbaoUbo.float2Offsets[i] = glm::vec4(float(i % 4) + 0.5f, float(i / 4) + 0.5f, 0.f, 0.f);
                        hbaoUbo.jitters[i]       = hbaoRandom[i];
                    }
#endif
}

void SSAO::drawLinearDepth(CameraSet* cs, int width, int height, int sampleIdx)
{
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.depthlinear);

                    if (samples > 1) {
                        depth_linearize_msaa->begin();

                        glUniform4f(0, cs->getNear() * cs->getFar(), cs->getNear() - cs->getFar(), cs->getFar(), 1.0f);
                        glUniform1i(1, sampleIdx);

#ifndef ARA_USE_GLES31
                        glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, textures.scene_depthstencil);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, 0);
#endif
                        depth_linearize_msaa->end();
                    } else {
                        depth_linearize->begin();

                        glUniform4f(0, cs->getNear() * cs->getFar(), cs->getNear() - cs->getFar(), cs->getFar(), 1.0f);
#ifndef ARA_USE_GLES31
                        glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthstencil);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
#endif
                        depth_linearize->end();
                    }

                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

void SSAO::drawHbaoBlur(int width, int height, int sampleIdx)
{
                    // here the last Fbo must still stayed bound
                    // if blur is activ, hbao_calc s_fbo is bound, 2 attachments
                    // 0 > result, 1 > blur

                    if (USE_AO_SPECIALBLUR)
                        hbao_blur->begin();
                    else
                        bilateralblur->begin();

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, textures.scene_depthlinear);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, textures.hbao_result);

                    glUniform2f(1, 1.0f / float(width),
                                0.f);               // InvResolutionDirection
                    glUniform1f(0, blurSharpness);  // sharpness

#ifndef ARA_USE_GLES31
                    glDrawBuffer(GL_COLOR_ATTACHMENT1);  // draw to blur result
                                                         // attachment
#endif

                    glDrawArrays(GL_TRIANGLES, 0, 3);

                        //--------------------------------------------------------
                        // final output to main s_fbo
#ifndef USE_GLSG_FBO
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
#else
                    sceneFbo->bind();
#endif

                    glDisable(GL_DEPTH_TEST);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ZERO, GL_SRC_COLOR);

                    if (samples > 1) {
                        glEnable(GL_SAMPLE_MASK);
                        glSampleMaski(0, 1 << sampleIdx);
                    }

#if USE_AO_SPECIALBLUR
                    hbao_blur2->begin();
                    glUniform1f(0, blurSharpness);  // sharpness
#endif

                        //	glActiveTexture(GL_TEXTURE0);
                        //	glBindTexture(GL_TEXTURE_2D,
                        // textures.hbao_blur);

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.hbao_blur);
#endif
                    glUniform2f(1, 0.f,
                                1.0f / float(height));  // InvResolutionDirection
                    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void SSAO::drawHbaoClassic(CameraSet* cs, int width, int height, int sampleIdx)
{
                    prepareHbaoData(cs, width, height);
                    drawLinearDepth(cs, width, height, sampleIdx);  // linearize

                    if (blur) {
                        glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
#ifndef ARA_USE_GLES31
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
#endif
                    } else {
#ifndef USE_GLSG_FBO
                        glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
#else
                        sceneFbo->bind();
#endif
                        glDisable(GL_DEPTH_TEST);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
                        if (samples > 1) {
                            glEnable(GL_SAMPLE_MASK);
                            glSampleMaski(0, 1 << sampleIdx);
                        }
                    }

                    if (USE_AO_SPECIALBLUR && blur)
                        hbao_calc_blur->begin();
                    else
                        hbao_calc->begin();

                    glBindBufferBase(GL_UNIFORM_BUFFER, 0, hbao_ubo);
#ifndef ARA_USE_GLES31
                    glNamedBufferSubDataEXT(hbao_ubo, 0, sizeof(HBAOData), &hbaoUbo);

                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthlinear);
                    glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, textures.hbao_randomview[sampleIdx]);
#endif
                    glDrawArrays(GL_TRIANGLES, 0, 3);

                    if (blur) drawHbaoBlur(width, height, sampleIdx);

                    glEnable(GL_DEPTH_TEST);
                    glDisable(GL_BLEND);
                    glDisable(GL_SAMPLE_MASK);
                    glSampleMaski(0, ~0);

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
                    glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, 0);
#endif
                    glUseProgram(0);
}

void SSAO::drawHbaoCacheAware(CameraSet* cs, int width, int height, int sampleIdx)
{
                    int quarterWidth  = ((width + 3) / 4);
                    int quarterHeight = ((height + 3) / 4);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ZERO);

                    prepareHbaoData(cs, width, height);
                    drawLinearDepth(cs, width, height, sampleIdx);  // linearize

                    // ------ calculate the normals and save them into an FBO

                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.viewnormal);

                    viewnormal->begin();

                    glUniform4fv(0, 1, &hbaoUbo.projInfo[0]);
                    glUniform1i(1, hbaoUbo.projOrtho);
                    glUniform2fv(2, 1, &hbaoUbo.InvFullResolution[0]);

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthlinear);
#endif
                    glDrawArrays(GL_TRIANGLES, 0, 3);
#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
#endif

                    // ------ deinterleave ---------------------

                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);
                    glViewport(0, 0, quarterWidth, quarterHeight);

                    hbao2_deinterleave->begin();
#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.scene_depthlinear);
#endif

                    for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i += NUM_MRT) {
                        glUniform4f(0, float(i % 4) + 0.5f, float(i / 4) + 0.5f, hbaoUbo.InvFullResolution.x,
                                    hbaoUbo.InvFullResolution.y);

                        for (int layer = 0; layer < NUM_MRT; layer++)
                            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + layer, GL_TEXTURE_2D,
                                                   textures.hbao2_depthview[i + layer], 0);

                        glDrawArrays(GL_TRIANGLES, 0, 3);
                    }

                    // ------ ssao calc ---------------------

                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
                    glViewport(0, 0, quarterWidth, quarterHeight);

                    if (USE_AO_SPECIALBLUR)
                        hbao2_calc_blur->begin();
                    else
                        hbao2_calc->begin();

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, textures.scene_viewnormal);
#endif
                    glBindBufferBase(GL_UNIFORM_BUFFER, 0, hbao_ubo);
#ifndef ARA_USE_GLES31
                    glNamedBufferSubDataEXT(hbao_ubo, 0, sizeof(HBAOData), &hbaoUbo);
#endif

#if USE_AO_LAYERED_SINGLEPASS

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, textures.hbao2_deptharray);
#endif

#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
                    glBindImageTexture(0, textures.hbao2_resultarray, 0, GL_TRUE, 0, GL_WRITE_ONLY,
                                       USE_AO_SPECIALBLUR ? GL_RG16F : GL_R8);
#endif

                    glDrawArrays(GL_TRIANGLES, 0, 3 * HBAO_RANDOM_ELEMENTS);

#if USE_AO_LAYERED_SINGLEPASS == AO_LAYERED_IMAGE
                    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif

#else
                    for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++) {
                        glUniform2f(0, float(i % 4) + 0.5f, float(i / 4) + 0.5f);
                        glUniform4fv(1, 1, hbaoRandom[i].get_value());

                        glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, textures.hbao2_depthview[i]);
                        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures.hbao2_resultarray, 0,
                                                  i);

                        glDrawArrays(GL_TRIANGLES, 0, 3);
                    }
#endif

                    // ------ reinterleave ---------------------

                    if (blur) {
                        glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao_calc);
#ifndef ARA_USE_GLES31
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
#endif

                    } else {
#ifndef USE_GLSG_FBO
                        glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);
#else
                        sceneFbo->bind();
#endif
                        glDisable(GL_DEPTH_TEST);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_ZERO, GL_SRC_COLOR);

                        if (samples > 1) {
                            glEnable(GL_SAMPLE_MASK);
                            glSampleMaski(0, 1 << sampleIdx);
                        }
                    }

                    glViewport(0, 0, width, height);

                    if (USE_AO_SPECIALBLUR && blur)
                        hbao2_reinterleave_blur->begin();
                    else
                        hbao2_reinterleave->begin();

                        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, textures.hbao2_resultarray);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_ARRAY, 0);
#endif
                    // --------- blur ---------------------

                    if (blur) drawHbaoBlur(width, height, sampleIdx);

                    glDisable(GL_BLEND);
                    glEnable(GL_DEPTH_TEST);
                    glDisable(GL_SAMPLE_MASK);
                    glSampleMaski(0, ~0);

#ifndef ARA_USE_GLES31
                    glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
                    glBindMultiTextureEXT(GL_TEXTURE1, GL_TEXTURE_2D, 0);
#endif

                    glUseProgram(0);
}

void SSAO::copyFbo(CameraSet* cs)
{
                    int width  = int(cs->getActFboSize()->x);
                    int height = int(cs->getActFboSize()->y);

                    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);

#ifndef USE_GLSG_FBO
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos.scene);
#else
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFbo->getFbo());
#endif
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, lastBoundFbo);
                    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

void SSAO::bind()
{
#ifndef USE_GLSG_FBO
                    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
                    glGetBooleanv(GL_MULTISAMPLE, &lastMultiSample);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbos.scene);

                    glGetIntegerv(GL_VIEWPORT, &csVp[0]);
                    glViewport(0, 0, fboWidth, fboHeight);
                    glScissor(0, 0, fboWidth, fboHeight);  // wichtig!!!

#else
                    sceneFbo->bind();
#endif
}

void SSAO::clear(glm::vec4 clearCol)
{
                    glClearBufferfv(GL_COLOR, 0, &clearCol.x);
                    glClearDepthf(1.f);
                    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void SSAO::unbind()
{
#ifndef USE_GLSG_FBO
                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
                    lastMultiSample ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);

                    glViewport(csVp[0], csVp[1], csVp[2], csVp[3]);
                    glScissor(csVp[0], csVp[1], csVp[2],
                              csVp[3]);  // wichtig!!!

#else
                    sceneFbo->unbind();
#endif
}

void SSAO::proc(CameraSet* cs)
{
                    int width  = int(cs->getActFboSize()->x);
                    int height = int(cs->getActFboSize()->y);

                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glEnable(GL_CULL_FACE);
                    glEnable(GL_DEPTH_TEST);

                    glBindVertexArray(defaultVAO);  // create VAO, assign the
                                                    // name and bind that array

                    for (int sample = 0; sample < samples; sample++) {
                        switch (algorithm) {
                            case ALGORITHM_HBAO_CLASSIC: drawHbaoClassic(cs, width, height, sample); break;
                            case ALGORITHM_HBAO_CACHEAWARE: drawHbaoCacheAware(cs, width, height, sample); break;
                            default: break;
                        }
                    }

                    glBindVertexArray(0);  // create VAO, assign the name and bind that array
}

void SSAO::drawBlit(CameraSet* cs, bool copyDepth)
{
                    // glBlitFramebuffer not working ????
#ifndef USE_GLSG_FBO
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos.scene);
#else
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFbo->getFbo());
#endif
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastBoundFbo);

                    glBlitFramebuffer(0, 0, int(cs->getActFboSize()->x), int(cs->getActFboSize()->y), 0, 0,
                                      int(cs->getActFboSize()->x), int(cs->getActFboSize()->y), GL_COLOR_BUFFER_BIT,
                                      GL_NEAREST);

                    // blit depth
                    if (copyDepth)
                        glBlitFramebuffer(0, 0, int(cs->getActFboSize()->x), int(cs->getActFboSize()->y), 0, 0,
                                          int(cs->getActFboSize()->x), int(cs->getActFboSize()->y), GL_DEPTH_BUFFER_BIT,
                                          GL_NEAREST);

                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

void SSAO::drawAlpha(CameraSet* cs, float alpha)
{
                    unbind();

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_CULL_FACE);
                    // glDisable(GL_SAMPLE_MASK);

                    texShdr->begin();
                    texShdr->setIdentMatrix4fv("m_pvm");
                    texShdr->setUniform1i("tex", 0);
                    texShdr->setUniform1f("alpha", 1.f);
                    texShdr->setUniform2f("scr_size", cs->getActFboSize()->x, cs->getActFboSize()->y);
                    texShdr->setUniform1i("nrSamples", samples);

                    glActiveTexture(GL_TEXTURE0);

#ifndef USE_GLSG_FBO
                    if (samples > 1)
                        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures.scene_color);
                    else
                        glBindTexture(GL_TEXTURE_2D, textures.scene_color);
#else
                    if (samples > 1)
                        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, sceneFbo->getColorImg());
                    else
                        glBindTexture(GL_TEXTURE_2D, sceneFbo->getColorImg());
#endif

                    /*
                    texNoAlpha->begin();
                    texNoAlpha->setIdentMatrix4fv("m_pvm");
                    texNoAlpha->setUniform1i("tex", 0);
                    //glBindTexture(GL_TEXTURE_2D, textures.scene_depthlinear);

                    if (samples > 1)
                    {
                            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,
                    textures.scene_color); } else { glBindTexture(GL_TEXTURE_2D,
                    textures.scene_color);
                    }
                    //glBindTexture(GL_TEXTURE_2D, textures.scene_viewnormal);
                    //glBindTexture(GL_TEXTURE_2D, textures.hbao2_depthview[0]);
                    //glBindTexture(GL_TEXTURE_2D, textures.hbao2_resultarray);
                    // schwarz -> GL_RG16F
                    //glBindTexture(GL_TEXTURE_2D, textures.hbao_result);
                    // schwarz -> GL_RG16F
                    //glBindTexture(GL_TEXTURE_2D, textures.hbao_blur);

                    */

                    quad->draw();
                    texShdr->end();

                    glDisable(GL_SAMPLE_MASK);
                    glSampleMaski(0, ~0);
}

void SSAO::blitDepthBuffer(CameraSet* cs)
{
#ifndef USE_GLSG_FBO
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos.scene);
#else
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFbo->getFbo());
#endif
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastBoundFbo);
                    // does not work with framebuffer name = 0 (default
                    // framebuffer)
                    glBlitFramebuffer(0, 0, int(cs->getActFboSize()->x), int(cs->getActFboSize()->y), 0, 0,
                                      int(cs->getActFboSize()->x), int(cs->getActFboSize()->y), GL_DEPTH_BUFFER_BIT,
                                      GL_NEAREST);

                    glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);
}

GLuint SSAO::getSceneFboColorTex()
{
#ifndef USE_GLSG_FBO
                    return textures.scene_color;
#else
                    return sceneFbo->getColorImg();
#endif
}

void SSAO::resize(uint _width, uint _height)
{
                    fboWidth  = _width;
                    fboHeight = _height;
                    initFramebuffers((int)_width, (int)_height, m_glbase->getNrSamples());
}

SSAO::~SSAO()
{
                    glDeleteVertexArrays(1, &defaultVAO);
                    if (hbao_ubo) glDeleteBuffers(1, &hbao_ubo);
                    glDeleteTextures(MAX_SAMPLES, &textures.hbao_randomview[0]);

#ifndef USE_GLSG_FBO
                    if (samples > 1) {
                        glDeleteTextures(1, &textures.scene_color);
                        glDeleteTextures(1, &textures.scene_depthstencil);
                    } else {
                        glDeleteTextures(1, &textures.scene_color);
                        glDeleteTextures(1, &textures.scene_depthstencil);
                    }

                    glDeleteFramebuffers(1, &fbos.scene);
#endif

                    glDeleteTextures(1, &textures.scene_depthlinear);
                    glDeleteTextures(1, &textures.scene_viewnormal);
                    glDeleteTextures(1, &textures.hbao_result);
                    glDeleteTextures(1, &textures.hbao_blur);
                    glDeleteTextures(1, &textures.hbao2_deptharray);
                    glDeleteTextures(HBAO_RANDOM_ELEMENTS, &textures.hbao2_depthview[0]);
                    glDeleteTextures(1, &textures.hbao2_resultarray);

                    glDeleteFramebuffers(1, &fbos.hbao2_deinterleave);
                    glDeleteFramebuffers(1, &fbos.hbao2_calc);
                    glDeleteFramebuffers(1, &fbos.hbao_calc);
                    glDeleteFramebuffers(1, &fbos.depthlinear);
                    glDeleteFramebuffers(1, &fbos.viewnormal);
}

}
