// PartikelSystem point->quad geometry Shader
//
// shader um aus den Punkten quads zu machen, es wird gelesen:
// position -> gl_Position
// velocity : wird nicht benötigt, also ignoriert (einkommentieren?)
// aux0 -> x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

#version 410 core
#pragma optimize(on)

// Triangles in, triangles out, large max_vertices as we’re amplifying
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_GS_VERTEX
{
    vec4 aux0;
    vec4 aux1;
} vertex_in[];

out GS_FS_VERTEX
{
    vec2 tex_coord;
    vec4 aux_par0;
} vertex_out;

mat4 trans_matrix;
uniform mat4 m_pvm;

void main()
{
    if (vertex_in[0].aux1.x > 0.0)
    {
        for (int i=0;i<4;i++)
        {
            // Erstelle eine Matrize zum drehen und verschieben
            // aux_par.b = angle
            vec4 column0 = vec4(cos(vertex_in[0].aux0.z), sin(vertex_in[0].aux0.z), 0.0, 0.0);
            vec4 column1 = vec4(-sin(vertex_in[0].aux0.z), cos(vertex_in[0].aux0.z), 0.0, 0.0);
            vec4 column2 = vec4(0.0, 0.0, 1.0, 0.0);
            vec4 column3 = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, 0.0, 1.0);
            //vec4 column3 = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0);
            trans_matrix = mat4(column0, column1, column2, column3);

            // Ermittle die Position der Koordinaten aux_par.r = size, gl_Position ist der Mittelpunkt
            gl_Position = vec4((i == 1 || i == 3) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x,
                               (i == 0 || i == 1) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x,
                               0.0, 1.0);
            gl_Position = m_pvm * trans_matrix * gl_Position;
            vertex_out.aux_par0 = vertex_in[0].aux0;
            
            
            // aus effizienzgründen wird hier die aux_par.b koordinate mit dem alpha wert in life.a überschrieben
            vertex_out.aux_par0.b = vertex_in[0].aux1.a;
            vertex_out.tex_coord = vec2((i == 1 || i == 3) ? 1.0 : 0.0,
                                        (i == 0 || i == 1) ? 1.0 : 0.0);

            EmitVertex();
        }
        EndPrimitive();
    }
}