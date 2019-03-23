// 3 Cameras in Cube for Scaler with special mapping
// no blending

#version 410
layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

out VS_GS_VERTEX {
    vec4 normal;
    vec2 texCoord;
    vec4 color;
} vertex_out;

void main()
{
    vertex_out.color = color;
    vertex_out.texCoord = texCoord;
    vertex_out.normal = normal;
    gl_Position = position;
}
