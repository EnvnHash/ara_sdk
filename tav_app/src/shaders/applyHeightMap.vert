#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;
uniform mat4 m_pvm;

uniform float stepX;
uniform float stepY;

out vec2 tl;
out vec2 t;
out vec2 tr;

out vec2 l;
out vec2 r;

out vec2 br;
out vec2 b;
out vec2 bl;

out vec2 tex_coord;

void main()
{
    tl = texCoord + vec2(-stepX, stepY);
    t = texCoord + vec2(0.0, stepY);
    tr = texCoord + vec2(stepX, stepY);
    
    l = texCoord + vec2(-stepX, 0.0);
    r = texCoord + vec2(stepX, 0.0);
    
    br = texCoord + vec2(stepX, -stepY);
    b = texCoord + vec2(0.0, -stepY);
    bl = texCoord + vec2(-stepX, -stepY);
    
    tex_coord = texCoord;
    gl_Position = m_pvm * position;
}