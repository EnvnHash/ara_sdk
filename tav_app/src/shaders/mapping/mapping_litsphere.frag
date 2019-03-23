// mapping litsphere shader
#version 410 core

layout (location = 0) out vec4 color;

in VS_FS_VERTEX
{
    vec4 pos;
    vec3 normal;
    vec4 color;
    vec2 tex_coord;
} vertex_in;

uniform sampler2D tex_diffuse0;
uniform sampler2D tex_diffuse1;
uniform sampler2D litsphereTexture;
uniform sampler2D normalTex;
uniform samplerCube cubeMap;

uniform vec3 lightPos;
uniform float shininess;
uniform vec4 specular;
uniform vec4 diffuse;
uniform vec4 ambient;
uniform vec4 sceneColor;

uniform float blend;
uniform float blendVec;

vec4 tex0, tex1, blendCol;
vec4 envColor;

void main()
{
    tex0 = texture(tex_diffuse0, vertex_in.tex_coord);
    tex1 = texture(tex_diffuse1, vertex_in.tex_coord);
    blendCol = tex0 * (1.0 - blend) + tex1 * blend;
    
    //vec4 normal = texture( normalTex, vertex_in.tex_coord );
        
    //vec3 eyeNormal = normal.xyz;
    vec3 eyeNormal = vertex_in.normal.xyz;
    vec3 L = normalize(lightPos - vertex_in.pos.xyz);
    vec3 E = normalize(-vertex_in.pos.xyz); // we are in Eye Coordinates, so EyePos is (0,0,0)
    vec3 R = normalize(-reflect(L, eyeNormal));
    
    //calculate Ambient Term:
    vec4 Iamb = ambient;
    
    //calculate Diffuse Term:
//    vec4 Idiff = blendCol * max(dot(eyeNormal, L), 0.0);
    vec4 Idiff = diffuse * max(dot(eyeNormal, L), 0.0);
    Idiff = clamp(Idiff, 0.0, 1.0);
    
    // calculate Specular Term:
    vec4 Ispec = specular * pow(max(dot(R, E), 0.0),0.3 * shininess);
    Ispec = clamp(Ispec, 0.0, 1.0);
    
    vec4 shading = texture(litsphereTexture, (-vertex_in.normal.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));
    vec4 shading2 = texture(litsphereTexture, (lightPos.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));
    
    shading = shading * blendVec + shading2 * (1.0 - blendVec);
  
    envColor = texture(cubeMap, R);
    
    color = ((Iamb + Idiff + Ispec + envColor * 0.5 * sceneColor) * shading ) * blendCol * 2.0;// + color;
//    color = envColor;
//    color *= vertex_in.color.a;
}