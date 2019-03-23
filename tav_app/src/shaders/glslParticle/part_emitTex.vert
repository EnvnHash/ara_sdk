// basic color shader
#version 410 core
#pragma optimize(on)

layout(location=0) in vec4 position;
layout(location=5) in vec4 velocity;
layout(location=6) in vec4 aux0;
layout(location=7) in vec4 aux1;

out vec4 pos;

uniform float nrEmitTrig;
uniform mat4 m_pvm;

void main()
{
    pos = position;
    gl_Position = m_pvm * position;
}