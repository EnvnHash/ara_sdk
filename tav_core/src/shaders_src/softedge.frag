#version 410 core

uniform sampler2D tex;
in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

void main()
{
    //    color = vec4(1.0, 0.0, 0.0, 1.0);
    color = texture(tex, tex_coord);
}