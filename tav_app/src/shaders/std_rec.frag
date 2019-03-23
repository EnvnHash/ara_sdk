// standard record fragment shader
#version 410 core

in vec4 world_space_position;
in vec3 vs_fs_normal;
layout (location = 0) out vec4 color;

void main()
{
    color = vec4(1.0);
}