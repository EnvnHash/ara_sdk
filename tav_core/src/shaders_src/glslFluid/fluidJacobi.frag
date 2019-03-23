// GLSLFluid jacobi frag shader
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

void main() {
    vec4 pN = texture(Pressure, tex_coord + vec2(0.0, step.y));
    vec4 pS = texture(Pressure, tex_coord + vec2(0.0, -step.y));
    vec4 pE = texture(Pressure, tex_coord + vec2(step.x, 0.0));
    vec4 pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0));
    vec4 pC = texture(Pressure, tex_coord);
    
    glm::vec3 oN = texture(tex0, tex_coord + vec2(0.0, step.y)).rgb;
    glm::vec3 oS = texture(tex0, tex_coord + vec2(0.0, -step.y)).rgb;
    glm::vec3 oE = texture(tex0, tex_coord + vec2(step.x, 0.0)).rgb;
    glm::vec3 oW = texture(tex0, tex_coord + vec2(-step.x, 0.0)).rgb;
    
    if (oN.x > 0.1) pN = pC;
    if (oS.x > 0.1) pS = pC;
    if (oE.x > 0.1) pE = pC;
    if (oW.x > 0.1) pW = pC;
    
    vec4 bC = texture(Divergence, tex_coord);
    fragColor = (pW + pE + pS + pN + Alpha * bC) * InverseBeta;
}