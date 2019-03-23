#version 410 core

layout (points) in;
layout (points, max_vertices = 1) out;

in vec2 tex_coord[];
in vec2 tex_coord2[];

uniform sampler2D velTex;

layout(location=0) out vec4 rec_position;
layout(location=5) out vec4 rec_velocity;

void main()
{
    vec4 velTexFrag = texture(velTex, tex_coord[0]);
    vec4 velTexFragRight = texture(velTex, tex_coord2[0]);
    float angleOffset = dot(velTexFrag.xy, velTexFragRight.xy);

    rec_velocity = vec4(velTexFrag.rgb * 0.1, angleOffset * 0.0005);
    
    // Copy the input position to the output
//    gl_Position = gl_in[0].gl_Position;
    gl_Position = vec4(gl_in[0].gl_Position.x, gl_in[0].gl_Position.y, 0.0, 1.0);
    rec_position = gl_Position;
    
    EmitVertex();
    EndPrimitive();
}
