#version 410 core

uniform sampler2D cColor;
uniform sampler2D cHeightMap;

uniform vec3 Ambient;
uniform vec3 LightColor;
uniform vec3 LightDirection;    // direction toward the light
uniform vec3 HalfVector;        // surface orientation for shiniest spots

uniform float Shininess;        // exponent for sharping highlights
uniform float Strength;         // extra factor to adjust shininess
uniform float clearAmt;

uniform float hMStrength;

vec4 canvasColor;
float canvasCoverage;

in vec2 tex_coord;
in vec4 col;

layout (location = 0) out vec4 color;

void main()
{
    vec3 norm = vec3(texture(cHeightMap, tex_coord));
    canvasColor = texture(cColor, tex_coord);
    
    // compute cosine of the directions, using dot products,
    // to see how much light would be reflected
    float diffuse = max(0.0, dot(norm, LightDirection));
    float specular = max(0.0, dot(norm, HalfVector));
    
    // surfaces facing away from the light (negative dot products)
    // won’t be lit by the directional light
    if (diffuse == 0.0)
        specular = 0.0;
    else
        specular = pow(specular, Shininess); // sharpen the highlight
    
    vec3 scatteredLight = Ambient + LightColor * diffuse;
    vec3 reflectedLight = LightColor * specular * Strength;
    
    // don’t modulate the underlying color with reflected light,
    // only with scattered light
    vec3 rgb = min(canvasColor.rgb * scatteredLight + reflectedLight, vec3(1.0));
    color = vec4(rgb, canvasColor.a) * hMStrength + (1.0 - hMStrength) * canvasColor;
}