#version 410 core

uniform sampler2D patternTex;
//uniform sampler2D alphaTex;
uniform sampler2D blurTex;

uniform float colMult;
//uniform float blurTexAmt;

uniform vec2 scale;
uniform vec2 offset;

uniform vec2 brushSize;

in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;

vec4 pattern, alpha, blur;

void main()
{
    blur = texture(blurTex, tex_coord);
    //alpha = texture(alphaTex, tex_coord);
    pattern = texture(patternTex, vec2(tex_coord.x * (1.0 / scale.x) + offset.x,
                                       tex_coord.y * (1.0 / scale.y) + offset.y));
    
    color = vec4(pattern.rgb, 1.0);
    color.a = blur.r;
//    color.a = (blur.r *blurTexAmt + 1.0 - blurTexAmt) * alpha.r;
}