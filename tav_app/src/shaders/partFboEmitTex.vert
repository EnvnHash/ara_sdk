// GLSLParticleSystemFBO Emit Shader
// pos.w = lifetime
#version 410 core
#pragma optimize(on)

layout(location = 0) in vec4 position;
layout(location = 3) in vec4 color;
layout(location = 5) in vec4 velocity;
layout(location = 6) in vec4 aux0;
layout(location = 7) in vec4 aux1;

uniform mat4 m_pvm;

out VS_GS_VERTEX
{
    vec4 pos;
    vec4 vel;
    vec4 col;
    vec4 aux0;
} vertex_out;

void main()
{
    vertex_out.pos = position;
    vertex_out.vel = velocity;
    vertex_out.col = color;
    vertex_out.aux0 = aux0;
    gl_Position = aux1; // speicherposition
}