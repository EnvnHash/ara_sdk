// GLSLParticleSystemFBO Draw Point Shader
#version 410 core

layout (location = 0) out vec4 color;

in vec4 fsColor;

void main()
{
    color = fsColor;
//    color = vec4(1.0, 1.0, 0.0, 1.0);
}