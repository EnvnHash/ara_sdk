#version 410 core

in vec3 R;
uniform samplerCube cubeMap;
out vec4 color;

void main()
{
    vec4 reflectedColor = texture(cubeMap, R);    
    color = reflectedColor;
}