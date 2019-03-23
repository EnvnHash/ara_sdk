#version 330 core

uniform sampler2D image;
uniform sampler2D old;

uniform float width;
uniform float height;
uniform float oldFdbk;
uniform float newFdbk;

in vec2 tex_coord;
out vec4 FragmentColor;

uniform float offset[3];
uniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

void main(void)
{
    FragmentColor = (texture(image,tex_coord) *newFdbk + texture(old,tex_coord)*oldFdbk) * weight[0];
    
	for (int i=1; i<3; i++) {
		FragmentColor += (texture(image, tex_coord+vec2(0.0, offset[i])) * newFdbk
                          + texture(old, tex_coord+vec2(0.0, offset[i])) * oldFdbk)
                         * weight[i];
		FragmentColor += (texture(image, tex_coord-vec2(0.0, offset[i])) * newFdbk
                          +texture(old, tex_coord-vec2(0.0, offset[i])) * oldFdbk)
                         * weight[i];
	}
}
