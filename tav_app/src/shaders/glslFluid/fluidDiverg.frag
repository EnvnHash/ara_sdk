// GLSLFluid divergence frag shader
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
    
    vec3 oN = texture(tex0, tex_coord + vec2(0.0, step.y)).rgb;
    vec3 oS = texture(tex0, tex_coord + vec2(0.0, -step.y)).rgb;
    vec3 oE = texture(tex0, tex_coord + vec2(step.x, 0.0)).rgb;
    vec3 oW = texture(tex0, tex_coord + vec2(-step.x, 0.0)).rgb;
    
    if (oN.x > 0.1) vN = oN.yz;
    if (oS.x > 0.1) vS = oS.yz;
    if (oE.x > 0.1) vE = oE.yz;
    if (oW.x > 0.1) vW = oW.yz;
    
    fragColor.r = HalfInverseCellSize * (vE.x - vW.x + vN.y - vS.y);
}