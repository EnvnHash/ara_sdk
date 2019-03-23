// basic color shader
#version 410 core
#pragma optimize(on)

layout(location=0) in vec4 position;
layout(location=5) in vec4 velocity;
layout(location=6) in vec4 aux0;
layout(location=7) in vec4 aux1;

out vec2 tex_coord;
out vec2 tex_coord2;

uniform float step_width;
uniform mat4 m_pvm;

void main()
{
    tex_coord = vec2(position.x * 0.5 + 0.5, position.y * 0.5 + 0.5);
    tex_coord2 = vec2(position.x * 0.5 + 0.5 + step_width, position.y * 0.5 + 0.5 + step_width);
    gl_Position = m_pvm * position;
}