// 3 Cameras in Cube for Scaler with special mapping
// no blending

#version 410
layout(triangles, invocations=3) in;
layout(triangle_strip, max_vertices=3) out;

uniform mat4 model_matrix_g[3];
uniform mat4 view_matrix_g[3];
uniform mat4 projection_matrix_g[3];

in VS_GS_VERTEX {
    vec4 normal;
    vec2 texCoord;
    vec4 color;
} vertex_in[];

out vec4 gs_Color;
out vec3 gs_Normal;
out vec2 gs_TexCoord;

void main()
{
    for (int i=0; i<gl_in.length(); i++)
    {
        gl_ViewportIndex = gl_InvocationID;
        
        // Normal is transformed using the model matrix.
        // Note that this assumes that there is no
        // shearing in the model matrix
        gs_Normal = (model_matrix_g[gl_InvocationID] * vertex_in[i].normal).xyz;
        
        gs_TexCoord = vertex_in[i].texCoord;
        gs_Color = vertex_in[i].color;

        // Finally, transform the vertex into position
        // and emit it.
        gl_Position = projection_matrix_g[gl_InvocationID] * view_matrix_g[gl_InvocationID] * (model_matrix_g[gl_InvocationID] * gl_in[i].gl_Position);
        
        EmitVertex();
    }
    EndPrimitive();
}
