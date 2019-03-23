// PartikelSystem record geometrie shader für blending
// amplifys the geometry, points -> quads

#version 410 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_GS_VERTEX
{
    vec4 position;
    vec4 aux0;
    vec4 aux1;
} vertex_in[];

uniform vec4 auxCol0;
uniform vec4 auxCol1;
uniform vec4 auxCol2;
uniform vec4 auxCol3;

out vec4 rec_position;
out vec3 rec_normal;
out vec4 rec_texCoord;
out vec4 rec_color;

mat4 trans_matrix;

// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

void main()
{
    for (int i=0;i<4;i++)
    {
        // aux0.b = angle
        vec4 column0 = vec4(cos(vertex_in[0].aux0.b), sin(vertex_in[0].aux0.b), 0.0, 0.0);
        vec4 column1 = vec4(-sin(vertex_in[0].aux0.b), cos(vertex_in[0].aux0.b), 0.0, 0.0);
        vec4 column2 = vec4(0.0, 0.0, 1.0, 0.0);
        vec4 column3 = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, gl_in[0].gl_Position.z, 1.0);
        trans_matrix = mat4(column0, column1, column2, column3);

        // aux0.x = size
        gl_Position = vec4((i == 1 || i == 3) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x,
                           (i == 0 || i == 1) ? vertex_in[0].aux0.x : -vertex_in[0].aux0.x,
                           0.0, 1.0);
        
        rec_position = trans_matrix * gl_Position;
        gl_Position = rec_position;
        
        // hack: die texCoord hat 4 koordinaten, die beiden letzen koordinaten
        // werden für rec_texCoord.z <- aux0.w (texUnit)
        // und rec_texCoord.w <- aux0.y (farbIndex) werwendet
        rec_texCoord = vec4((i == 1 || i == 3) ? 1.0 : 0.0,
                            (i == 0 || i == 1) ? 1.0 : 0.0,
                            vertex_in[0].aux0.w,
                            vertex_in[0].aux0.y);
        rec_normal = vec3(0.0, 0.0, -1.0);
        
        // hier wird die lebenszeit auf den alpha wert gemappt
        // und mit dem alpha wert von aux1.a multipliziert
        // pseudo tiefeneffekt: die z Koordinate zieht das alpha runter
        float depth = max(2.0 + gl_in[0].gl_Position.z, 0.0);
        vec4 particCol = max(1.0 - vertex_in[0].aux0.y *2.0, 0.0) *auxCol0
                        + max(1.0 - abs(vertex_in[0].aux0.y *2.0 -1.0), 0.0) *auxCol1
                        + max((vertex_in[0].aux0.y -0.5) *2.0, 0.0) *auxCol2;
        rec_color = vec4(particCol.rgb, vertex_in[0].aux1.y * vertex_in[0].aux1.a);
//        rec_color = vec4(depth, depth, depth, vertex_in[0].aux1.y * vertex_in[0].aux1.a );
        EmitVertex();
    }
    EndPrimitive();
}