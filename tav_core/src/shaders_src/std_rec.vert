// standard record vertex shader
#version 410 core

layout (location=0) in vec4 position;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 texCoord;
layout (location=3) in vec4 color;
layout (location=4) in vec4 texCorMod;
layout (location=10) in mat4 modMatr;

uniform int useInstancing;
uniform int texNr;

uniform mat4 modelMatrix;
uniform mat4 projectionMatrix;

out vec4 rec_position;
out vec3 rec_normal;
out vec4 rec_texCoord;
out vec4 rec_color;

mat4 MVM;

void main(void)
{
    rec_color = color;
    vec2 tc = useInstancing == 0 ? texCoord : texCoord * vec2(texCorMod.b, texCorMod.a) + vec2(texCorMod.r, texCorMod.g);
    rec_texCoord = vec4(tc.x, tc.y, float(texNr), 0.0);

    MVM = (useInstancing == 0 ? modelMatrix : modMatr);
    rec_position = MVM * position;
    rec_normal = normalize((MVM * vec4(normal, 0.0)).xyz);

    gl_Position = rec_position;
}