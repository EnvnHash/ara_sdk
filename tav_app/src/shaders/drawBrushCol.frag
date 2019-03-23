#version 410 core

uniform sampler2D tex;
uniform sampler2D oldCoverage;

uniform float alphaMult;

in vec2 tex_coord;
in vec2 full_tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

vec4 oldBack;
vec4 brushTex;

void main()
{
    // background
    oldBack = texture(oldCoverage, full_tex_coord);
    // brush texture
    brushTex = texture(tex, tex_coord);
    
    float scaleAlpha = min(max((brushTex.a * 2.0) - 1.0 - (oldBack.a * 0.8), 0.0), 1.0);
    color = brushTex * scaleAlpha + oldBack * (1.0 - scaleAlpha);
    color.a = scaleAlpha * alphaMult + oldBack.a;
}