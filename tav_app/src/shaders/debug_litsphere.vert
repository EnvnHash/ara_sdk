#version 410
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 texCoord;
layout(location=3) in vec4 color;
layout(location=4) in vec4 texCorMod;
layout(location=10) in mat4 modMatr;

uniform int useInstancing;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

out vec4 vs_Color;
out vec3 vs_Normal;
out vec4 vs_TexCoord;
out vec4 vs_Position;

uniform samplerBuffer pos_tbo;
uniform samplerBuffer nor_tbo;
uniform samplerBuffer tex_tbo;
uniform samplerBuffer col_tbo;
uniform float mapScaleFact;
uniform float relBlend;
mat4 MVM;
vec4 mapPos;
vec3 mapNorm;
int mappedPos;

void main()
{
vs_Position = position;
mappedPos = int(float(gl_VertexID) * mapScaleFact);
mapPos = (position * (1.0 - relBlend)) + relBlend * texelFetch(pos_tbo, mappedPos);
mapNorm = (normal * (1.0 - relBlend)) + relBlend * texelFetch(nor_tbo, mappedPos).xyz;
vs_Color = (color * (1.0 - relBlend)) + relBlend * texelFetch(col_tbo, mappedPos);
vs_TexCoord = (texCoord * (1.0 - relBlend)) + relBlend * texelFetch(tex_tbo, mappedPos);
MVM = viewMatrix * (useInstancing == 0 ? modelMatrix : modMatr);
vs_Normal = normalize(normalMatrix * mapNorm);
gl_Position = projectionMatrix * MVM * mapPos;
}
