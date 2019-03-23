#version 330 core

layout( location = 0 ) in vec4 position;
layout( location = 2 ) in vec2 texCoord;

out vec2 tex_coord;

void main(void) {
    tex_coord = texCoord;
	gl_Position = vec4(position.xy, 0.0, 1.0);
}
