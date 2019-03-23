// basic light shader
#version 330
#pragma optimize(on)

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform mat4 m_pvm;
uniform mat3 m_normal;

out vec4 Color;
out vec3 Normal;    // interpolate the normalized surface normal
out vec2 tex_coord;

void main () {
    Color = color;
    
    // transform the normal, without perspective, and normalize it
    Normal = normalize(m_normal * normal);
    //Normal = mat3(transpose(inverse(model))) * normal;

    tex_coord = texCoord;
    gl_Position = m_pvm * position;
}

