// GLSLParticleSystemFBO emit shader
// pos.w = lifetime
#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 80) out;

in VS_GS_VERTEX
{
    vec4 pos;
    vec4 vel;
    vec4 col;
    vec4 aux0;  // speicherposition
} vertex_in[];

out GS_FS_VERTEX
{
    vec4 oPos;
    vec4 oVel;
    vec4 oCol;
    vec4 oAux0;
} vertex_out;

uniform sampler2D emitTex;
uniform int nrPartPerCellSqrt;
uniform float invGridSizeF;
uniform int maxEmit;
uniform int texWidth;
uniform int texHeight;
uniform int gridSizeSqrt;

void main()
{
    int emitCount = 0;
    
    int nrHPix = texWidth / gridSizeSqrt;
    int nrVPix = texHeight / gridSizeSqrt;
    
    // in dem definierten auschnitt gehe durch die Emit Textur durch
    for (int y=0;y<nrVPix;y++)
    {
        for (int x=0;x<nrHPix;x++)
        {
            if (emitCount >= maxEmit)
            {
                break;
            } else {
                vec2 writeOffset = vec2(float(x) / float(nrHPix -1),
                                        float(y) / float(nrVPix -1));
                
                vec2 readOffset = vec2(float(x) / float(texWidth), float(y) / float(texHeight));
                
                vec2 tex_coord = vec2(gl_in[0].gl_Position.x * 0.5 + 0.5 + readOffset.x,
                                      1.0 - (gl_in[0].gl_Position.y * 0.5 + 0.5 + readOffset.y));
                
                vec4 texCol = texture(emitTex, tex_coord);
                
                if ((texCol.r + texCol.g) > 0.1)
                {
                    vertex_out.oPos = gl_in[0].gl_Position + vec4(writeOffset.xy * invGridSizeF * 2.0, 0.0, 0.0)
                    + vertex_in[0].pos;
                    vertex_out.oVel = vec4(texCol.r, texCol.g, 0.0, 0.0);
//                    vertex_out.oVel = vertex_in[0].vel;
                    vertex_out.oCol = vertex_in[0].col;
                    vertex_out.oAux0 = vertex_in[0].aux0;
                    
                    gl_Position = gl_in[0].gl_Position + vec4(writeOffset.xy * invGridSizeF * 2.0, 0.0, 0.0);
                    
                    EmitVertex();
                    EndPrimitive();
                    
                    emitCount++;
                }
            }
        }
    }
}
