#version 410 core

uniform sampler2D tex;

in vec2 le;
in vec2 ri;

in vec2 tex_coord;
in vec4 col;

layout (location = 0) out vec4 color;

void main()
{
    vec4 left = texture(tex, le);
    vec4 center = texture(tex, tex_coord);
    vec4 right = texture(tex, ri);
    
    bool leftDet = (left.r == 0.0 && center.r > 0.0) || (left.r > 0.0 && center.r == 0.0);
    bool rightDet = (right.r == 0.0 && center.r > 0.0) || (right.r > 0.0 && center.r == 0.0);
    float outVal = leftDet || rightDet ? 1.0 : 0.0;

    color = vec4(outVal);
}