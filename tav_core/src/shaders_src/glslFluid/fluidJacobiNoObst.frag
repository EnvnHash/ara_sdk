// GLSLFluid jacobi frag no obstacles shader
#version 410
#pragma optimize(on)

uniform sampler2D Pressure;
uniform sampler2D Divergence;
uniform sampler2D tex0;

uniform float Alpha;
uniform float InverseBeta;
uniform vec2 step;
in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 pN = texture(Pressure, tex_coord + vec2(0.0, step.y));
    vec4 pS = texture(Pressure, tex_coord + vec2(0.0, -step.y));
    vec4 pE = texture(Pressure, tex_coord + vec2(step.x, 0.0));
    vec4 pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0));
    vec4 pC = texture(Pressure, tex_coord);
    
    vec4 bC = texture(Divergence, tex_coord);
    fragColor = (pW + pE + pS + pN + Alpha * bC) * InverseBeta;
}