#version 410 core

uniform sampler2D tex;
in vec2 tl;
in vec2 t;
in vec2 tr;

in vec2 bl;
in vec2 b;
in vec2 br;

in vec2 l;
in vec2 r;

in vec2 tex_coord;
layout (location = 0) out vec4 color;

double pStrength = 2.0;
vec3 norm;

void main()
{

    vec4 topLeft = texture(tex, tl);
    vec4 top = texture(tex, t);
    vec4 topRight = texture(tex, tr);

    vec4 left = texture(tex, l);
    //vec4 center = texture(tex, tex_coord);
    vec4 right = texture(tex, r);

    vec4 bottomRight = texture(tex, br);
    vec4 bottom = texture(tex, b);
    vec4 bottomLeft = texture(tex, bl);
    
    // sobel filter
    double dX = (topRight.r + 2.0 * right.r + bottomRight.r) - (topLeft.r + 2.0 * left.r + bottomLeft.r);
    double dY = (bottomLeft.r + 2.0 * bottom.r + bottomRight.r) - (topLeft.r + 2.0 * top.r + topRight.r);
    double dZ = 1.0 / pStrength;
    
    norm = normalize(vec3(dX, dY, dZ));

//    color = texture(tex, tex_coord);
    color = vec4(norm, 1.0);
}