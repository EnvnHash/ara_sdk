#version 410 core

uniform sampler2D tex;

uniform int width;
uniform int height;

in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

void main()
{
    vec2 blendPos = vec2(gl_FragCoord) / vec2(width, height);
    
    vec4 top = texture(tex, vec2( tex_coord.x, (tex_coord.y +0.5) *0.5 + 0.25 ) );
    vec4 bottom = texture(tex, vec2( tex_coord.x, (tex_coord.y +0.5) *0.5 - 0.25 ) );
    
    color = top * sqrt(blendPos.y) + bottom * sqrt(1.0 - blendPos.y);
}