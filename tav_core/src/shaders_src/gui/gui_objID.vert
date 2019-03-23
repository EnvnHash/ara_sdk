// basic color shader
#version 410 core
#pragma optimize(on)

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;
uniform mat4 m_pvm;

out vec4 col;

void main()
{
    col = color;
    gl_Position = m_pvm * position;
}