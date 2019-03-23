// basic texShader
#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform mat4 viewMatrix; // world to view transformation
uniform mat4 modelMatrix; // world to view transformation for normal
uniform mat4 m_pvm;
uniform mat3 m_normal;
uniform vec3 eyePosW;
uniform float etaRatio;

vec3 N, I;
out vec3 R;
out vec3 T;
out vec2 tex_coord;

void main()
{
    // position.w should be 1 if not position /= position.w
    vec4 positionInViewSpace = viewMatrix * modelMatrix * position;

    // Compute position and normal in world space
    N = m_normal * normal.xyz;
    N = normalize(N);
    
    // Compute the incident and reflected vectors
    I = positionInViewSpace.xyz - eyePosW;
    R = reflect(I, N);
    T = refract(I, N, etaRatio);
    
    tex_coord = texCoord;
    
    gl_Position = m_pvm * position;
}