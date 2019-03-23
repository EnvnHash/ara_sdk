// basic light shader
#version 330
#pragma optimize(on)

uniform sampler2D tex_diffuse0;
//uniform sampler2D tex_diffuse1;
//uniform sampler2D tex_diffuse2;
//uniform sampler2D tex_specular0;
//uniform sampler2D tex_specular1;

uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 lightColor;
uniform vec3 lightDirection;    // direction toward the light
uniform vec3 halfVector;        // surface orientation for shiniest spots
uniform float shininess;        // exponent for sharping highlights
uniform float strength;         // extra factor to adjust shininess

in vec4 Color;
in vec3 Normal; // surface normal, interpolated between vertices
in vec2 tex_coord;

out vec4 FragColor;

vec4 tex0;
//vec4 tex1;


void main()
{
    tex0 = texture(tex_diffuse0, tex_coord);
    //tex1 = texture(tex_diffuse1, tex_coord);

    // compute cosine of the directions, using dot products,
    // to see how much light would be reflected
    float diffuseAmt = max(0.0, dot(Normal, lightDirection));
    float specular = max(0.0, dot(Normal, halfVector));
    
    // surfaces facing away from the light (negative dot products)
    // won’t be lit by the directional light
    if (diffuseAmt == 0.0)
        specular = 0.0;
    else
        specular = pow(specular, shininess); // sharpen the highlight
    
    vec3 scatteredLight = vec3(ambient) + vec3(diffuse) * diffuseAmt;
    vec3 reflectedLight = vec3(lightColor) * specular * strength;
    
    // don’t modulate the underlying color with reflected light,
    // only with scattered light

    vec3 rgb = min((Color.rgb + tex0.rgb) * scatteredLight + reflectedLight, vec3(1.0));
    rgb.r *= 2.0;
    FragColor = vec4(rgb * 1.3, Color.a);
    
}