#version 410 core
#pragma optimize(on)

uniform sampler2DRect tex;
in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

void main()
{
    color = texture(tex, tex_coord) + vec4(1.0, 1.0, 0.0, 1.0);
}