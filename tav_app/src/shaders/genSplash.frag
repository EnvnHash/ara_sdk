#version 410 core

uniform sampler2D noiseTex;
uniform float offset;

in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

vec4 noiseS, noiseM, noiseB, center, smallDots, medDots, outCol;
float thresh = 0.25;

void main()
{
    vec2 toMid = vec2(0.5, 0.5) - tex_coord;
    float bright = max(sqrt(toMid.x*toMid.x + toMid.y*toMid.y), 0.0) * 1.0 / 0.707106781186548;
    
    // small splashes
    noiseS = texture(noiseTex, tex_coord * vec2(0.5, 0.5) + vec2(offset, offset));
    // distribution
    noiseS.rgb *= bright;

    // medium splashes
    noiseM = texture(noiseTex, tex_coord * vec2(0.25, 0.25) + vec2(offset, offset));
    // distribution
    noiseM.rgb *= 1.0 - abs((bright * 1.5 - 0.5) * 2.0);

    // big splash
    noiseB = texture(noiseTex, tex_coord * vec2(0.08, 0.08) + vec2(offset, offset));
    // distribution
    noiseB.rgb *= min((1.0 - bright * 4.0), 1.0);

    center = vec4(vec3(min((1.0 - bright * 7.0), 1.0)), 1.0);
    
    outCol = noiseS * 2.0 + noiseM * 0.2 + noiseB * 0.16 + center * 0.3;
   
    // thresholding
    outCol = outCol.r > thresh ? vec4(1.0, 1.0, 1.0, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
    
    color = outCol;
}