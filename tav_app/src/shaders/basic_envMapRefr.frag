#version 410 core

in vec3 R;
in vec3 T;
in vec2 tex_coord;

uniform samplerCube cubeMap;
uniform samplerCube refrMap;
uniform sampler2D decalMap;
out vec4 color;

void main()
{
    vec4 reflectedColor = texture(cubeMap, R);
    vec4 refractedColor = texture(refrMap, T);
    vec4 decalColor = texture(decalMap, tex_coord);

//    color = refractedColor;
//    color = decalColor.r * refractedColor + (1.0 - decalColor.r) * reflectedColor;
   color = reflectedColor * 0.25 + refractedColor * 0.75;
}