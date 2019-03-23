#version 330 core

uniform sampler2D image;

uniform float width;
uniform float height;

in vec2 tex_coord;
out vec4 FragmentColor;

uniform float offset[3];
//uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

void main(void)
{

    FragmentColor = texture( image, tex_coord ) * weight[0];
//    if (FragmentColor.r < 0.01)
//        discard;
    
    for (int i=1; i<3; i++) {
        FragmentColor += texture( image, tex_coord +vec2(offset[i], 0.0) ) * weight[i];
        FragmentColor += texture( image, tex_coord -vec2(offset[i], 0.0) ) * weight[i];
    }
}