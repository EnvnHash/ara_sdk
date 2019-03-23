// GLSLFluid apply impulse
#version 410
#pragma optimize(on)

uniform vec2    Point;
uniform float   Radius;
uniform vec3    Value;
uniform int     isVel;
in vec2 tex_coord;
layout(location = 0) out vec4 fragColor;

void main()
{
    float d = distance(Point, tex_coord);
    if (d < Radius) {
        float a = (Radius - d) * 0.5;
        a = min(a, 1.0);
        fragColor = vec4(Value, a);
    } else {
        fragColor = vec4(0);
    }
}