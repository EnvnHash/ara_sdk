// GLSLFluid divergence frag no obstacles shader
#version 410
#pragma optimize(on)

uniform sampler2D Velocity;
uniform sampler2D tex0;
uniform vec2 step;
uniform float HalfInverseCellSize;

in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 vN = texture(Velocity, tex_coord + vec2(0.0,step.y)).rg;
    vec2 vS = texture(Velocity, tex_coord + vec2(0.0,-step.y)).rg;
    vec2 vE = texture(Velocity, tex_coord + vec2(step.x,0.0)).rg;
    vec2 vW = texture(Velocity, tex_coord + vec2(-step.x,0.0)).rg;
    fragColor.r = HalfInverseCellSize * (vE.x - vW.x + vN.y - vS.y);
}