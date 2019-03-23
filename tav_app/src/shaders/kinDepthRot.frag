// basic texShader
#version 410 core
#pragma optimize(on)

uniform sampler2D tex;
uniform float deeperThres;
uniform float nearerThres;
uniform float depthWidth;
uniform float depthHeight;
uniform float cropRight;
uniform float cropLeft;
uniform float cropUp;
uniform float cropDown;
uniform vec2 kinFov;
uniform mat4 invRotX;

in vec2 tex_coord;
in vec4 pos;
in vec4 col;

layout (location = 0) out vec4 color;


vec3 getKinRealWorldCoord(vec3 inCoord)
{
    // asus xtion tends to measure lower depth with increasing distance
    // experimental correction
    float depthScale = 1.0 + pow(inCoord.z * 0.00033, 5.3);
    float scaledDepth = inCoord.z * depthScale;
    
    float xzFactor = tan(kinFov.x * 0.5) * 2.0;  // stimmt noch nicht... wieso?
    float yzFactor = tan(kinFov.y * 0.5) * 2.0;  // stimmt!!!
    
    return vec3(inCoord.x * 0.5 * scaledDepth * xzFactor,
                inCoord.y * 0.5 * scaledDepth * yzFactor,
                scaledDepth);
}


void main()
{
    vec4 depth = texture(tex, tex_coord);
    vec3 rwPos = vec3(pos.x, pos.y, depth.r * 65535.0);
    rwPos = getKinRealWorldCoord(rwPos);
    vec4 rotPos = invRotX * vec4(rwPos.x, rwPos.y, rwPos.z, 1.0);
    float deep = rotPos.z < deeperThres ? rotPos.z : 0.0;
    deep = deep > nearerThres ? 1.0 : 0.0;

    deep = (pos.x < cropRight) ? deep : 0.0;
    deep = (pos.x > cropLeft) ? deep : 0.0;
//    deep = (pos.y < cropUp) ? deep : 0.0;
//    deep = (pos.y > cropDown) ? deep : 0.0;

    color = vec4(deep, deep, deep, 1.0);
//    color = vec4(depth.r * 35535.0, 0.0, 0.0, 1.0);
}