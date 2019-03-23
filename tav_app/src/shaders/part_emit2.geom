#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 1) out;

in vec4 pos[];

out float countCol;

uniform float dt;
uniform float lifeTime;
uniform float aging;
uniform float ageSizing;
uniform vec3 gravity;

vec4 outV;

void main()
{
    // in dem definierten auschnitt gehe durch die Emit Textur durch
    rec_position = outV;
    rec_aux0 = vec4(0);
    rec_aux1 = vec4(0);
                
    rec_velocity = vec4(0.0, 0.2, 0.0, 0.0);
                
    gl_Position = outV;

    EmitVertex();
    EndPrimitive();
}
