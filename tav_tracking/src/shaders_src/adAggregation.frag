#version 410 core

uniform sampler2D costMapIn;
uniform sampler2D winSizeIn;
uniform sampler2D leftLimits;
uniform sampler2D rightLimits;
uniform sampler2D upLimits;
uniform sampler2D downLimits;

uniform int directionH;
uniform int directionW;
uniform int initWhite;
uniform int aggrIt;
uniform float costFactor;

in vec2 tex_coord;
in vec4 col;

ivec2 tex_coordI;
ivec2 offsTexCoord;

layout (location = 0) out vec4 costMap;
layout (location = 1) out vec4 winSize;

void main()
{
    int dmin, dmax, d;
    tex_coordI = ivec2(gl_FragCoord.xy);
    
    if(directionH == 0)
    {
        dmin = int(texelFetch(leftLimits, tex_coordI, 0).r * -1.0);
        dmax = int(texelFetch(rightLimits, tex_coordI, 0).r);
    } else
    {
        dmin = int(texelFetch(upLimits, tex_coordI, 0).r * -1.0);
        dmax = int(texelFetch(downLimits, tex_coordI, 0).r);
    }

    float cost = 0;
    int tmpWindowSize = 0;
    for(d = dmin; d <= dmax; d++)
    {
        offsTexCoord = ivec2(d * directionW, d * directionH);
        cost += (initWhite == 0 && aggrIt == 0) ? texelFetch(costMapIn, tex_coordI + offsTexCoord, 0).r * costFactor :
                                                  texelFetch(costMapIn, tex_coordI + offsTexCoord, 0).r;
//        tmpWindowSize += (initWhite == 0) ? 1 : int(texelFetch(winSizeIn, tex_coordI, 0).r);
        tmpWindowSize += (initWhite == 0) ? 1 : int(texelFetch(winSizeIn, tex_coordI + offsTexCoord, 0).r);
    }

    costMap = vec4(cost, 0, 0, 1);
    winSize = vec4(tmpWindowSize, 0, 0, 1);
}