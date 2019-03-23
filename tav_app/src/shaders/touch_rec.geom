#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 1) out;

uniform sampler2D idMap;
layout(location=0) out vec4 rec_position;

vec4 outV;

void main()
{
    // in dem definierten auschnitt gehe durch die Emit Textur durch
    outV = gl_in[0].gl_Position;

    vec4 objId = texture(idMap, vec2(outV.x * 0.5 + 0.5, outV.y * 0.5 + 0.5));
    
    rec_position = vec4(objId.r, 0.0, 0.0, 1.0);
    gl_Position = gl_in[0].gl_Position;
    
    EmitVertex();
    EndPrimitive();
}
