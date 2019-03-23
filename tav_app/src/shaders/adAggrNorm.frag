#version 410 core

uniform sampler2D costMap;
uniform sampler2D winSize;

in vec2 tex_coord;
in vec4 col;

layout (location = 0) out vec4 color;

ivec2 tex_coordI;

void main()
{
    tex_coordI = ivec2(gl_FragCoord.xy);

    float cM = texelFetch(costMap, tex_coordI, 0).r;
    float wS = texelFetch(winSize, tex_coordI, 0).r;
    
    color = vec4(cM / wS, 0, 0, 1);
}