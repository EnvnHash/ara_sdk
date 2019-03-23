// basic texShader
#version 410 core
#pragma optimize(on)

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;
uniform mat4 m_pvm;

out vec2 tex_coord;

void main()
{
    tex_coord = texCoord;
    gl_Position = m_pvm * position;
}