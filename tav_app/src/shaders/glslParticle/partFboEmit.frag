// GLSLParticleSystemFBO Emit Shader
#version 410 core
#pragma optimize(on)

layout (location = 0) out vec4 pos;
layout (location = 1) out vec4 vel;
layout (location = 2) out vec4 color;
layout (location = 3) out vec4 aux0;

in VS_FS_VERTEX
{
    vec4 pos;
    vec4 vel;
    vec4 col;
    vec4 aux0;
} vertex_in;

void main()
{    
    pos = vertex_in.pos;
    vel = vertex_in.vel;
    color = vertex_in.col;
    aux0 = vertex_in.aux0;
}