#version 410 core

layout (points) in;
layout (points, max_vertices = 80) out;

in vec4 pos[];

uniform float nrEmitTrig;
uniform ivec2 winSize;
uniform vec2 texSize;
uniform sampler2D posTex;

layout(location=0) out vec4 rec_position;
layout(location=5) out vec4 rec_velocity;

ivec2 texPos;
vec4 posTexFrag;
vec4 outV;

void main()
{
    // in dem definierten auschnitt gehe durch die Emit Textur durch
    for (int y=0;y<winSize.y;y++)
    {
        for (int x=0;x<winSize.x;x++)
        {
            texPos = ivec2(int(pos[0].x * texSize.x) + x, int(pos[0].y * texSize.y) + y);
            posTexFrag = texelFetch(posTex, texPos, 0);

            if (posTexFrag.r > 0.1)
            {
                outV = vec4(float(texPos.x) / texSize.x * 2.0 - 1.0,
                            float(texPos.y) / texSize.y * 2.0 - 1.0,
                            0.0, 1.0);
                
                rec_position = outV;
                gl_Position = outV;
                
                rec_velocity = vec4(0.0, 0.1, 0.0, 0.0);
                
                EmitVertex();
                EndPrimitive();
            }
        }
    }
}
