#version 410 core

uniform sampler2D inputTex;
uniform sampler2D fdbk;
uniform sampler2D paper;

uniform float absorbAmt;
uniform float paperStructAmt;

in vec2 tl;
in vec2 t;
in vec2 tr;

in vec2 bl;
in vec2 b;
in vec2 br;

in vec2 l;
in vec2 r;

in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

uniform float coeff[9];
uniform float weight;

vec4 limCol;

void main()
{
    vec4 sumCol = vec4(0.0);
    vec4 inCol = texture(inputTex, tex_coord);
    vec4 paper = texture(paper, tex_coord) * paperStructAmt + vec4(1.0 - paperStructAmt);
    vec4 centerCol = texture(fdbk, tex_coord);
 
    // topLeft
    sumCol += texture(fdbk, tl) * coeff[0];
    //top
    sumCol += texture(fdbk, t) * coeff[1];
    // topright
    sumCol += texture(fdbk, tr) * coeff[2];

    // left
    sumCol += texture(fdbk, l) * coeff[3];
    // center
    sumCol += centerCol * coeff[4];
    // right
    sumCol += texture(fdbk, r) * coeff[5];

    // botLeft
    sumCol += texture(fdbk, bl) * coeff[6];
    // bot
    sumCol += texture(fdbk, b) * coeff[7];
    // botRight
    sumCol += texture(fdbk, br) * coeff[8];
    
    sumCol *= weight; // normalize value
    
    limCol = (sumCol.r +sumCol.g +sumCol.b) > (inCol.r +inCol.g +inCol.b) ? sumCol : inCol;
    
    color = min(limCol, inCol * inCol.a + sumCol * absorbAmt * (1.0 - inCol.a));
    color.a = inCol.a + sumCol.a * absorbAmt * paper.r;
}