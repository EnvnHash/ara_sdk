// GLSLFluid advect frag shader
#version 410
#pragma optimize(on)

uniform sampler2D tex0;         // Real obstacles
uniform sampler2D backbuffer;
uniform sampler2D VelocityTexture;

uniform float TimeStep;
uniform float Dissipation;
uniform vec2 scr;
uniform int bObstacles;
in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{
    float solid = texture(tex0, tex_coord).r;
    if (solid > 0.1) {
        fragColor = vec4(0.0,0.0,0.0,0.0);
        return;
    }
    vec2 u = texture(VelocityTexture, tex_coord).rg;
    vec2 coord = tex_coord * scr - TimeStep * u;
    fragColor = Dissipation * texture(backbuffer, coord / scr);
}