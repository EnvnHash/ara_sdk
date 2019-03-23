// standard scene blending morphing vertex shader
#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec4 texCoord;
layout( location = 3 ) in vec4 color;

uniform samplerBuffer pos_tbo;
uniform samplerBuffer nor_tbo;
uniform samplerBuffer tex_tbo;
uniform samplerBuffer col_tbo;

uniform int triangle_count;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

//in int gl_VertexID;
out vec4 col;

void main()
{
    col = color;
    vec4 pos = texelFetch(pos_tbo, gl_VertexID);
	gl_Position = projection_matrix * view_matrix * model_matrix * position;
}