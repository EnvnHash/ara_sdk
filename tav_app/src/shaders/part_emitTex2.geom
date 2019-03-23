// GLSLParticleSystem2 emit from Texture
#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 90) out;

in vec4 pos[];

uniform int nrEmitTrig;
uniform ivec2 winSize;
uniform vec2 texSize;
uniform sampler2D posTex;

uniform int emitLimit;

uniform float speed;
uniform float size;
uniform float colInd;
uniform float angle;
uniform float texNr;
uniform float alpha;

uniform float dt;
uniform float lifeTime;
uniform float aging;
uniform float ageSizing;
uniform vec3 gravity;

uniform vec3 massCenter;


layout(location=0) out vec4 rec_position;
layout(location=5) out vec4 rec_velocity;
layout(location=6) out vec4 rec_aux0;
layout(location=7) out vec4 rec_aux1;

ivec2 texPos;
vec4 posTexFrag;
vec4 outV;

// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

void main()
{
    int emitCntr = 0;
    
    // in dem definierten auschnitt gehe durch die Emit Textur durch
    for (int y=0;y<winSize.y+1;y++)
    {
        if (emitCntr > emitLimit) break;

        for (int x=0;x<winSize.x+1;x++)
        {
            if (emitCntr > emitLimit) break;
            
            texPos = ivec2(int(pos[0].x * texSize.x) + x, int(pos[0].y * texSize.y) + y);
            posTexFrag = texelFetch(posTex, texPos, 0);

            if (posTexFrag.r > 0.1)
            {
                outV = vec4(float(texPos.x) / texSize.x * 2.0 - 1.0,
                            (1.0 - (float(texPos.y) / texSize.y)) * 2.0 - 1.0,
                            0.0, 1.0);
                
                rec_position = outV;
                gl_Position = outV;
                
                rec_velocity = vec4(0.2, 0.0, 0.0, 0.0);
                rec_aux0 = vec4(size, colInd, angle, texNr);
                rec_aux1 = vec4(lifeTime, 1.0, 0.0, alpha);
                
                EmitVertex();
                EndPrimitive();
                
                emitCntr++;
            }
        }
    }
}
