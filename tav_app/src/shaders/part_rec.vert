// PartikelSystem record vertex shader für blending

// shader um aus den Punkten quads zu machen, es wird gelesen:
// position -> gl_Position
// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

#version 410 core

layout(location=0) in vec4 position;
layout(location=4) in vec4 texCorMod;
layout(location=6) in vec4 aux0;
layout(location=7) in vec4 aux1;
layout(location=10) in mat4 modMatr;

uniform int useInstancing;
uniform int texNr;

uniform mat4 modelMatrix;
uniform mat4 projectionMatrix;

out VS_GS_VERTEX
{
    vec4 position;
    vec4 aux0;
    vec4 aux1;
} vertex_out;

mat4 MVM;

void main(void)
{
    vertex_out.aux0 = aux0;
    vertex_out.aux1 = aux1;
//    vertex_out.texCoord = useInstancing == 0 ? texCoord : texCoord * vec2(texCorMod.b, texCorMod.a) + vec2(texCorMod.r, texCorMod.g);

    MVM = (useInstancing == 0 ? modelMatrix : modMatr);
    vertex_out.position = MVM * position;
   // vertex_out.normal = normalize((MVM * vec4(normal, 0.0)).xyz);

    gl_Position =  vertex_out.position;
//    gl_Position = projectionMatrix * rec_position;
}