#version 410
in vec4 vs_Color;
in vec3 vs_Normal;
in vec4 vs_TexCoord;
in vec4 vs_Position;

uniform vec3 ambient;
uniform vec3 LPosition;
uniform vec3 LColor;
uniform vec3 LSpecular;
uniform vec3 halfVector;
uniform vec3 LDirection;
uniform vec4 sceneColor;
uniform float shininess;
uniform float strength;
uniform float brightness;
uniform float oscAlpha;
uniform float blendVec;
uniform sampler2D texs[1];
uniform sampler2D litsphereTexture;
uniform sampler2D normalTex;
uniform samplerCube cubeMap;

vec4 tex0;
vec4 envColor;
vec4 color;
out vec4 FragColor;

void main() {
    tex0 = texture(texs[0], vs_TexCoord.xy);
    vec3 L = normalize(LPosition - vs_Position.xyz);
    vec3 E = normalize(-vs_Position.xyz); // we are in Eye Coordinates, so EyePos is (0,0,0)
    vec3 R = normalize(-reflect(L, vs_Normal.xyz));
    vec4 Iamb = vec4(ambient, 1.0);
    vec4 Idiff = vec4(LColor, 1.0) * max(dot(vs_Normal.xyz, L), 0.0);
    Idiff = clamp(Idiff, 0.0, 1.0);
    vec4 Ispec = vec4(LSpecular, 1.0) * pow(max(dot(R, E), 0.0),0.3 * shininess);
    Ispec = clamp(Ispec, 0.0, 1.0);
    vec4 shading = texture(litsphereTexture, (-vs_Normal.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));
    vec4 shading2 = texture(litsphereTexture, (LPosition.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));
    shading = shading * blendVec + shading2 * (1.0 - blendVec);
    envColor = texture(cubeMap, R);
    color = ((Iamb + Idiff + Ispec + envColor * 0.5 * sceneColor) * shading ) * tex0;
    float alpha = (vs_Color.a + tex0.a) * oscAlpha;
    if (alpha < 0.001) discard;
    FragColor = vec4(color.rgb, alpha) * brightness;
}
