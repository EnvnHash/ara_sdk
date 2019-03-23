// GLSLFluid apply buoyancy
#version 410
#pragma optimize(on)

uniform sampler2D Velocity;
uniform sampler2D Temperature;
uniform sampler2D Density;

uniform float AmbientTemperature;
uniform float TimeStep;
uniform float Sigma;
uniform float Kappa;

uniform vec2  Gravity;

in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main(){
    
    float T = texture(Temperature, tex_coord).r;
    vec2 V = texture(Velocity, tex_coord).rg;
    
    fragColor.rg = V;
    
    if (T > AmbientTemperature) {
        float D = texture(Density, tex_coord).r;
        fragColor.rg += (TimeStep * (T - AmbientTemperature) * Sigma - D * Kappa ) * Gravity;
    }
}