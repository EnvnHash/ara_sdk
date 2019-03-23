#version 410 core

uniform sampler2D tex;
uniform float alpha;

in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

void main()
{
    color = texture(tex, tex_coord);
    color.a = color.a - alpha;
}