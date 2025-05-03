//
// Created by sven on 30-04-25.
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara::ssao {
static std::string hbaoDataStruct = STRINGIFY(
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
    };\n);


static std::string depthDebugVertShdr = STRINGIFY(
    layout(location = 0) in vec4 position; \n
    layout(location = 2) in vec2 texCoord; \n
    uniform mat4 m_pvm; \n
    out vec2 tex_coord; \n
    void main() { \n
        tex_coord   = texCoord; \n
        gl_Position = position; \n
    });

static std::string depthDebugFragShdr = STRINGIFY(
    in vec4 col; \n
    layout(location = 0) out vec4 color;  \n
    uniform sampler2D tex; \n
    in vec2 tex_coord;  \n
    void main() {  \n
        color = texture(tex, tex_coord) * 0.1; \n
    });

static std::string fullScreenQuadVertShdr = STRINGIFY(
    out vec2 texCoord;\n
    void main() { \n
        uint idx = gl_VertexID % 3; \n
        // allows rendering multiple fullscreen triangles
        vec4 pos    = vec4((float(idx & 1U)) * 4.0 - 1.0,\n
                           (float((idx >> 1U) & 1U)) * 4.0 - 1.0,\n
                            0, 1.0); \n
        gl_Position = pos; \n
        texCoord    = pos.xy * 0.5 + 0.5; \n
    });

static std::string fullScreenQuadGeoShader =  STRINGIFY(
    layout(triangles) in;\n
    layout(triangle_strip, max_vertices = 3) out;\n
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

static std::string bilateralBlurFragShdr = STRINGIFY(
    const float KERNEL_RADIUS = 3;\n

    layout(location=0) uniform float g_Sharpness;\n
    layout(location=1) uniform vec2 g_InvResolutionDirection;\n // either set x to 1/width or y to 1/height

    layout(binding=0) uniform sampler2D texSource;\n
    layout(binding=1) uniform sampler2D texLinearDepth;\n

    in vec2 texCoord;\n

    layout(location=0,index=0) out vec4 out_Color;\n

    //-------------------------------------------------------------------------

    vec4 BlurFunction(vec2 uv, float r, vec4 center_c, float center_d, inout float w_total) {\n
        vec4 c = texture( texSource, uv );\n
        float d = texture( texLinearDepth, uv).x;\n

        const float BlurSigma = float(KERNEL_RADIUS) * 0.5;\n
        const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);\n

        float ddiff = (d - center_d) * g_Sharpness;\n
        float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);\n
        w_total += w;\n

        return c*w;\n
    }\n

    void main() {\n
        vec4 center_c = texture( texSource, texCoord );\n
        float center_d = texture( texLinearDepth, texCoord).x;\n

        vec4 c_total = center_c;\n
        float w_total = 1.0;\n

        for (float r = 1; r <= KERNEL_RADIUS; ++r) {\n
            vec2 uv = texCoord + g_InvResolutionDirection * r;\n
            c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
        }\n


        for (float r = 1; r <= KERNEL_RADIUS; ++r) {\n
            vec2 uv = texCoord - g_InvResolutionDirection * r;\n
            c_total += BlurFunction(uv, r, center_c, center_d, w_total);\n
        }\n

        out_Color = c_total/w_total;\n
    });

static std::string viewNormalFragShdr = STRINGIFY(
    in vec2 texCoord;\n
    layout(location = 0) uniform vec4 projInfo;\n
    layout(location = 1) uniform int projOrtho;\n
    layout(location = 2) uniform vec2 InvFullResolution;\n
    layout(binding = 0) uniform sampler2D texLinearDepth;\n
    layout(location = 0, index = 0) out vec4 out_Color;\n
    \n
    vec3 UVToView(vec2 uv, float eye_z) { \n
         return vec3((uv * projInfo.xy + projInfo.zw) * (projOrtho != 0 ? 1.0 : eye_z), eye_z); \n
    }\n
    vec3 FetchViewPos(vec2 UV) { \n
        float ViewDepth = textureLod(texLinearDepth, UV, 0).x; \n
        return UVToView(UV, ViewDepth); \n
    }\n
    vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)\n { \n
        vec3 V1 = Pr - P;\n
        vec3 V2 = P - Pl; \n
        return (dot(V1, V1) < dot(V2, V2)) ? V1 : V2; \n
    }\n
    vec3 ReconstructNormal(vec2 UV, vec3 P)\n { \n
        vec3 Pr = FetchViewPos(UV + vec2(InvFullResolution.x, 0));\n
        vec3 Pl = FetchViewPos(UV + vec2(-InvFullResolution.x, 0));\n
        vec3 Pt = FetchViewPos(UV + vec2(0, InvFullResolution.y));\n
        vec3 Pb = FetchViewPos(UV + vec2(0, -InvFullResolution.y));\n
        return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));\n
    }\n
    void main() {\n
        vec3 P         = FetchViewPos(texCoord);\n
        vec3 N         = ReconstructNormal(texCoord, P);\n
        out_Color = vec4(N * 0.5 + 0.5, 0);\n
    });
}