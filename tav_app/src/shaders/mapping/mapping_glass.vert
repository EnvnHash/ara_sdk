// Vertex shader for environment mapping with an
// equirectangular 2D texture and refraction mapping
// with a background texture blended together using
// the fresnel terms
#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform float lookAngle;
uniform float zObjPos;

uniform vec3  LightPos;
uniform mat4  persp_matr;
uniform mat4  m_vm;
uniform mat3  m_normal;

out vec3  Normal;
out vec3  EyeDir;
out vec4  EyePos;
out vec4  col;
out float LightIntensity;

vec4 pos;

void main(void)
{
    col            = color;
    Normal         = normalize(m_normal * vec3(normal));
    Normal.y        *= -1.0;

    pos = m_vm * position;
    pos.y += tan(lookAngle) * (pos.z - zObjPos);

    EyeDir         = pos.xyz;
    EyePos		   = persp_matr * pos;
    LightIntensity = max(dot(normalize(LightPos - EyeDir), Normal), 0.0);
    
    gl_Position    = persp_matr * pos;
}