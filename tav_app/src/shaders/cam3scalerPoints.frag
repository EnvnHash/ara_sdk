// 3 Cameras in Cube for Scaler with special mapping
// no blending

#version 410

in vec4 gs_Color;
in vec3 gs_Normal;
in vec2 gs_TexCoord;

uniform float brightness;
out vec4 FragColor;

void main() {
    FragColor = gs_Color * brightness;
}
