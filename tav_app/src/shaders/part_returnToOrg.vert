// PartikelSystem vertex update Shader
//
// position -> gl_Position
// velocity : wird nicht benötigt, also ignoriert (einkommentieren?)
// aux0 -> x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

#version 410 core
#pragma optimize(on)

layout(location=0) in vec4 position;
layout(location=0) out vec4 rec_position;

uniform samplerBuffer orgPosTbo;
uniform float reposFact;
uniform mat4 m_pvm;

void main()
{
    vec4 orgPos = texelFetch(orgPosTbo, gl_VertexID);
    rec_position = position * (1.0 - reposFact) + orgPos * reposFact;
    
    gl_Position = m_pvm * position;
}