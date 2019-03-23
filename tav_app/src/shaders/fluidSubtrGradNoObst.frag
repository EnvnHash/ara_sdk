// GLSLFluid subtract gradient frag no obstacles shader
#version 410
#pragma optimize(on)

uniform sampler2D Velocity;
uniform sampler2D Pressure;
uniform sampler2D tex0;
uniform vec2 step;
uniform float GradientScale;

in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{    
    float pN = texture(Pressure, tex_coord + vec2(0.0, step.y)).r;
    float pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)).r;
    float pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)).r;
    float pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)).r;
    float pC = texture(Pressure, tex_coord).r;
    
    vec2 obstV = vec2(0.0,0.0);
    vec2 vMask = vec2(1.0,1.0);
    vec2 oldV = texture(Velocity, tex_coord).rg;
    vec2 grad = vec2(pE - pW, pN - pS) * GradientScale;
    vec2 newV = oldV - grad;
    
    fragColor.rg = (vMask * newV) + obstV;
}