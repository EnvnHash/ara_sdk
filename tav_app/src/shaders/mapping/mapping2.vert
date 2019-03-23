// basic texShader
#version 410 core
#pragma optimize(on)

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;
uniform mat4 m_pvm;
uniform mat4 persp_matr;
uniform float lookAngle;
uniform float zObjPos;
uniform int useDedist;

out vec2 tex_coord;
out vec4 col;

vec4 pos;

void main()
{
    col = color;
    tex_coord = texCoord;    
    pos = m_pvm * position;
    
    // apply perspective undistortion
//    if (useDedist == 1)
//    {
        pos.y += tan(lookAngle) * (pos.z - zObjPos);
//    }
    
    gl_Position = persp_matr * pos;
}