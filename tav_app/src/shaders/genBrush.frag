#version 410 core

uniform sampler2D alphaTex;
uniform sampler2D patternTex;
uniform sampler2D oldCoverage;

uniform float colMult;
//uniform float patternToAlpha;

uniform vec2 scale;
uniform vec2 offset;
uniform float pressure;
uniform float mixColors;
uniform float patternAmt;

uniform vec4 brushColor;

in vec2 tex_coord;
in vec2 full_tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

vec4 pattern, alpha, oldCol;
float intens, scaleMixColors;

void main()
{
    oldCol = texture(oldCoverage, full_tex_coord);
    pattern = texture(patternTex, vec2(tex_coord.x * (1.0 / scale.x) + offset.x,
                                       tex_coord.y * (1.0 / scale.y) + offset.y) );
    alpha = texture(alphaTex, tex_coord);

    intens = (pattern.r + pattern.g + pattern.b) * pressure;

    scaleMixColors = mixColors * oldCol.a + (1.0 - oldCol.a);
    
    color = vec4(brushColor.rgb * pattern.r * colMult * scaleMixColors + oldCol.rgb * (1.0 - scaleMixColors), 1.0);
    color.a = alpha.r * (intens * patternAmt + 1.0 - patternAmt);
}