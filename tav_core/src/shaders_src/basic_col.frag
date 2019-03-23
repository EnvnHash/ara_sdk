// basic color shader
#version 410 core
#pragma optimize(on)

in vec4 col;
layout (location = 0) out vec4 color;

void main()
{
    color = col;
}