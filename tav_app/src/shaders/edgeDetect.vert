#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform float stepX;
uniform float stepY;
uniform mat4 m_pvm;


out vec2 le;
out vec2 ri;
out vec2 tex_coord;
out vec4 col;

void main()
{
    le = texCoord + vec2(-stepX, 0.0);
    ri = texCoord + vec2(stepX, 0.0);
    
    col = color;
    tex_coord = texCoord;
    gl_Position = m_pvm * position;
}