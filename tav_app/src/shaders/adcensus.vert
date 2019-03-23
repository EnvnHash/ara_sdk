#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform mat4 m_pvm;

uniform float dispOffs;
uniform float width_step;
uniform float height_step;
uniform int nrAttachments;

out vec2 tex_coord_l[8];
out vec2 tex_coord_r[8];

out vec4 col;

void main()
{
    col = color;

    for (int i=0;i<8;i++)
    {
        tex_coord_r[i] = texCoord - vec2(width_step * (dispOffs +float(i)), 0.0);
        tex_coord_l[i] = texCoord + vec2(width_step * (dispOffs +float(i)), 0.0);
    }

    gl_Position = m_pvm * position;
}