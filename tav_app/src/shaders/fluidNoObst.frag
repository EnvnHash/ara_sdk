// GLSLFluid advect frag no obstacles shader
#version 410
#pragma optimize(on)

uniform sampler2D tex0;         // Real obstacles
uniform sampler2D backbuffer;
uniform sampler2D VelocityTexture;

uniform float TimeStep;
uniform float Dissipation;
uniform vec2 scr;
in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 u = texture(VelocityTexture, tex_coord).rg;
    vec2 coord = tex_coord * scr - TimeStep * u;
    fragColor = Dissipation * texture(backbuffer, coord / scr);
}