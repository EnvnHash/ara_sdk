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
//uniform vec3 eyePositionW;
uniform vec3 eyePosW;

vec3 N;
vec3 I;
vec3 test;
out vec3 R;

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

    gl_Position = m_pvm * position;
}