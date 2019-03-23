// basic light shader
#version 330
#pragma optimize(on)

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform mat4 m_pvm;
uniform mat4 persp_matr;
uniform mat3 m_normal;
uniform float lookAngle;
uniform float zObjPos;

out vec4 Color;
out vec3 Normal;    // interpolate the normalized surface normal
out vec2 tex_coord;
vec4 pos;

void main () {
    Color = color;
    
    // transform the normal, without perspective, and normalize it
    Normal = normalize(m_normal * normal);
    //Normal = mat3(transpose(inverse(model))) * normal;
    tex_coord = texCoord;

    pos = m_pvm * position;
    pos.y += tan(lookAngle) * (pos.z - zObjPos);

    //pos.y = tan(lookAngle) * -(zObjPos - pos.z);
    
    gl_Position = persp_matr * pos;
}

