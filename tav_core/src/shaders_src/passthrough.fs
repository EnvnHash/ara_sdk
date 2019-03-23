#version 330 core

uniform sampler2D image;

out vec4 FragmentColor;

void main(void)
{
    tex_coord = texCoord;

	FragmentColor = texture2D( image, vec2(gl_FragCoord)/1024.0 );
}
