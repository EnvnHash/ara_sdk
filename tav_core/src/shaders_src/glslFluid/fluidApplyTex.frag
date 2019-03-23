// GLSLFluid APPLY TEXTURE
#version 410
#pragma optimize(on)

uniform sampler2D tex1;
uniform float   pct;
uniform float   Radius;
uniform int   isVel;
uniform vec3 multCol;

vec4 outCol;
in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 newFrame = texture(tex1, tex_coord);
    outCol = vec4(multCol.rgb * newFrame.rgb, newFrame.a) * pct;
    
    if (isVel==1)
    {
        float intens = outCol.r + outCol.g;
        if (abs(intens) < 0.125) discard;
        fragColor = vec4(outCol.r, outCol.g, 0.0, 1.0);
    } else
    {
        float intens = outCol.r + outCol.g + outCol.b * 0.3333;
        if (abs(intens) < Radius) outCol = vec4(0);
        fragColor = outCol;
    }
}