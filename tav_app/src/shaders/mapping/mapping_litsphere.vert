// mapping litsphere
#version 410
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

out VS_FS_VERTEX
{
    vec4 pos;
    vec3 normal;
    vec4 color;
    vec2 tex_coord;
} vertex_out;

vec4 pos;

void main ()
{
    vertex_out.color = color;
    
    // transform the normal, without perspective, and normalize it
    vertex_out.normal = normalize(m_normal * normal);
    //Normal = mat3(transpose(inverse(model))) * normal;
    vertex_out.tex_coord = texCoord;
    
    pos = m_pvm * position;
    pos.y += tan(lookAngle) * (pos.z - zObjPos);
    
    vertex_out.pos = position;
    
    gl_Position = persp_matr * pos;
}

