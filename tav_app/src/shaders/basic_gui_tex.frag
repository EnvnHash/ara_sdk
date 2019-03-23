// basic color shader
#version 410 core
#pragma optimize(on)

uniform sampler2D tex;
uniform vec4 backColor;
uniform vec4 frontColor;

in vec2 tex_coord;
layout (location = 0) out vec4 color;

vec4 col;

void main()
{
    col = texture(tex, tex_coord) * frontColor;
    color = col.a * col + (1.0 - col.a) * backColor;
}