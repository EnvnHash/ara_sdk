// WIRD MOMENTAN NICHT BENUTZT
// GLSLParticleSystemFBO emit shader
// pos.w = lifetime
#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 1) out;

in VS_GS_VERTEX
{
    vec4 pos;
    vec4 vel;
    vec4 col;
    vec4 aux0;
} vertex_in[];

out GS_FS_VERTEX
{
    vec4 oPos;
    vec4 oVel;
    vec4 oCol;
    vec4 oAux0;
} vertex_out;


void main()
{
    int emitPos = nrEmitPerGrid * gl_InvocationID;
    int curEmitPos;
    
    // in dem definierten auschnitt gehe durch die Emit Textur durch
    for (int i=0;i<nrEmitPerGrid;i++)
    {
        curEmitPos = emitPos +i;
        
        vertex_out.oPos = vertex_in[0].pos;
        vertex_out.oVel = vertex_in[0].vel;
        vertex_out.oCol = vertex_in[0].col;
        vertex_out.oAux0 = vertex_in[0].aux0;

        gl_Position = vec4((gl_in[0].gl_Position.x + float(i % gridSizeX) / float(gridSizeX)) * 2.0 - 1.0,
                           (gl_in[0].gl_Position.y + float(i / gridSizeX) / float(gridSizeY)) * 2.0 - 1.0,
                           gl_in[0].gl_Position.z,
                           gl_in[0].gl_Position.w);

        EmitVertex();
        EndPrimitive();
    }
}
