// GLSLParticleSystemFBO Draw Point Shader
// pos.w = lifetime
#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 144) out;

uniform sampler2D pos_tex;
uniform sampler2D vel_tex;
uniform sampler2D col_tex;
uniform sampler2D aux0_tex;

uniform int pixPerGrid;
uniform float invNrPartSqrt;

uniform mat4 m_pvm;

out vec4 fsColor;

void main()
{
    vec4 pPos, pVel, pCol, pAux0;
    vec2 texCoord;
    vec2 baseTexCoord = gl_in[0].gl_Position.xy;

    // read the textures
    for (int y=0;y<pixPerGrid;y++)
    {
        for (int x=0;x<pixPerGrid;x++)
        {
            texCoord = baseTexCoord + vec2(float(x) * invNrPartSqrt, float(y) * invNrPartSqrt);
            pPos = texture(pos_tex, texCoord);
            
            // switching doesn´t work, since we are double buffering...
            if (pPos.w > 0.001)
            {
                //pVel = texture(vel_tex, texCoord);
                pCol = texture(col_tex, texCoord);
                pAux0 = texture(aux0_tex, texCoord);

                fsColor = pCol;
//                gl_Position = vec4(pPos.xy, 1.0, 1.0);
                gl_Position = m_pvm * vec4(pPos.xyz, 1.0);
                
                EmitVertex();
                EndPrimitive();
            }
        }
    }
}
