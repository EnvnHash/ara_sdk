// basic color shader
#version 410 core
#pragma optimize(on)

uniform int objId;
in vec4 col;
layout (location = 0) out vec4 color;

void main()
{
    color = vec4(float(objId), 0, 0, 1);
}