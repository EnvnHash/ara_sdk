// basic texShader
#version 410 core
#pragma optimize(on)

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;
uniform mat4 m_pvm;
uniform mat4 de_dist;
uniform int useDedist;

out vec2 tex_coord;
out vec4 col;

vec4 pos;
float x, y, w;
float eps=1.19209290e-07;

void main()
{
    col = color;
    tex_coord = texCoord;    
	pos = m_pvm * position;
    
    // apply perspective undistortion
    if (useDedist == 1)
    {
        // project onto the 2d plane, coordinates are still normalized (-1|1)
        pos /= pos.w;

        x = pos.x;
        y = pos.y;
        w = x * de_dist[0][2] + y * de_dist[1][2] + de_dist[2][2];
        
        if( abs(w) > eps )
        {
            w = 1./w;
            pos.x = (x * de_dist[0][0] + y * de_dist[1][0] + de_dist[2][0]) *w;
            pos.y = (x * de_dist[0][1] + y * de_dist[1][1] + de_dist[2][1]) *w;
        } else {
            pos.x = pos.y = 0.0;
        }
        
      //  pos = de_dist * pos;
        // z will end up positive, but has no real meaning anymore, so set it to 0
        pos.z = 0;
    }

    gl_Position = pos;
}