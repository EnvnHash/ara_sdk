// PartikelSystem point->quad vertex Shader
#version 410 core
#pragma optimize(on)

layout(location=0) in vec4 position;
layout(location=5) in vec4 velocity;
layout(location=6) in vec4 aux0;
layout(location=7) in vec4 aux1;

out VS_GS_VERTEX
{
    vec4 aux0;
    vec4 aux1;
} vertex_out;

void main()
{
    vertex_out.aux0 = aux0;
    vertex_out.aux1 = aux1;
    gl_Position = position;
}