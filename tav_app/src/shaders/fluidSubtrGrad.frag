// GLSLFluid subtract gradient frag shader
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
    vec3 oC = texture(tex0, tex_coord).rgb;
    if (oC.x > 0.1) {
        fragColor.gb = oC.yz;
        return;
    }
    
    float pN = texture(Pressure, tex_coord + vec2(0.0, step.y)).r;
    float pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)).r;
    float pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)).r;
    float pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)).r;
    float pC = texture(Pressure, tex_coord).r;
    
    vec3 oN = texture(tex0, tex_coord + vec2(0.0, step.y)).rgb;
    vec3 oS = texture(tex0, tex_coord + vec2(0.0, -step.y)).rgb;
    vec3 oE = texture(tex0, tex_coord + vec2(step.x, 0.0)).rgb;
    vec3 oW = texture(tex0, tex_coord + vec2(-step.x, 0.0)).rgb;
    
    vec2 obstV = vec2(0.0,0.0);
    vec2 vMask = vec2(1.0,1.0);
    
    if (oN.x > 0.1) { pN = pC; obstV.y = oN.z; vMask.y = 0.0; }
    if (oS.x > 0.1) { pS = pC; obstV.y = oS.z; vMask.y = 0.0; }
    if (oE.x > 0.1) { pE = pC; obstV.x = oE.y; vMask.x = 0.0; }
    if (oW.x > 0.1) { pW = pC; obstV.x = oW.y; vMask.x = 0.0; }
    
    vec2 oldV = texture(Velocity, tex_coord).rg;
    vec2 grad = vec2(pE - pW, pN - pS) * GradientScale;
    vec2 newV = oldV - grad;
    
    fragColor.rg = (vMask * newV) + obstV;
}