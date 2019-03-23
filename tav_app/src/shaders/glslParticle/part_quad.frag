// PartikelSystem point->quad fragment Shader
#version 410 core
layout (location = 0) out vec4 color;

in GS_FS_VERTEX
{
    vec2 tex_coord;
    vec4 aux_par0;    // aux_par0: (r: size, g: farbIndex (0-1), b: angle, a: textureUnit)
} vertex_in;

uniform sampler2D texs[8];

void main()
{
    vec4 tex = texture(texs[int(vertex_in.aux_par0.a)], vertex_in.tex_coord);
    color = vec4( tex.rgb, tex.a * vertex_in.aux_par0.b  );
}