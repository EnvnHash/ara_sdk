#version 410 core

in vec3 viewDirection;
in vec3 normalDirection;

uniform samplerCube cubeMap;

out vec4 color;

void main()
{
    vec3 reflectedDirection = reflect(-viewDirection, normalize(normalDirection));
//    color = vec4(1.0, 0.0, 0.0, 1.0);
    color = texture(cubeMap, reflectedDirection);
}