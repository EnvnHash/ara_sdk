// GLSLParticleSystemFBO Emit Shader
#version 410 core
#pragma optimize(on)

layout (location = 0) out vec4 pos;
layout (location = 1) out vec4 vel;
layout (location = 2) out vec4 color;
layout (location = 3) out vec4 aux0;

in GS_FS_VERTEX
{
    vec4 oPos;
    vec4 oVel;
    vec4 oCol;
    vec4 oAux0;
} vertex_in;

void main()
{    
    pos = vertex_in.oPos;
    vel = vertex_in.oVel;
    color = vertex_in.oCol;
    aux0 = vertex_in.oAux0;
}