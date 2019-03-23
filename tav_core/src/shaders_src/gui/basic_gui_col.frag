// basic color shader
#version 410 core
#pragma optimize(on)

uniform sampler2D tex;
uniform vec4 backColor;
uniform vec4 frontColor;

layout (location = 0) out vec4 color;

vec4 col;

void main()
{
    color = backColor;
}