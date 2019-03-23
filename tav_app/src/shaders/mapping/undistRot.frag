// KinectReproTools Undistortion + Rotation + Thresholding
#version 410 core
//#pragma optimize(on)

layout (location = 0) out vec4 color;

uniform sampler2D tex;

uniform float deeperThres;
uniform float nearerThres;
uniform float depthWidth;
uniform float depthHeight;
uniform float cropRight;
uniform float cropLeft;
uniform vec2 kinFov;

uniform mat4 invRotX;
uniform mat4 de_dist;

in vec2 tex_coord;
in vec4 col;
//in vec4 pos;

float xzFactor, yzFactor;
float multDiff, upperCorr, scaledDepth;
float eps=1.19209290e-07;


vec3 getKinRealWorldCoord(vec3 inCoord)
{
    // asus xtion tends to measure lower depth with increasing distance
    // experimental correction
    multDiff = inCoord.z - 950.0;
    upperCorr = max(multDiff, 0) * 0.02;
    scaledDepth = inCoord.z + (multDiff * 0.0102) + upperCorr;
    
    xzFactor = tan(kinFov.x * 0.5) * 2.0;
    yzFactor = tan(kinFov.y * 0.5) * 2.0;
    
    return vec3(inCoord.x * 0.5 * scaledDepth * xzFactor,
                inCoord.y * 0.5 * scaledDepth * yzFactor,
                scaledDepth);
}


// geht noch nicht...
vec2 getUnwarpTexCoord(vec2 in_tex)
{
    float x, y, w;
    vec2 outV;

    x = (in_tex.x) * depthWidth;
    y = (in_tex.y) * depthHeight;
    
    w = x * de_dist[0][2] + y * de_dist[1][2] + de_dist[2][2];
    
    if( abs(w) > eps )
    {
        w = 1.0 / w;
        outV.x = (x * de_dist[0][0] + y * de_dist[1][0] + de_dist[2][0]) *w;
        outV.y = (x * de_dist[0][1] + y * de_dist[1][1] + de_dist[2][1]) *w;
        
    } else {
        outV.x = 0.0;
        outV.y = 0.0;
    }
    
    outV.x = (outV.x / depthWidth);
    outV.y = 1.0 - (outV.y / depthHeight);
    
    return outV;
}


void main()
{
    /*
    vec2 modTexCoord = getUnwarpTexCoord(tex_coord);
    vec4 depth = texture(tex, modTexCoord);
    vec2 normModTexCoord = vec2(modTexCoord.x * 2.0 - 1.0,
                                modTexCoord.y * 2.0 - 1.0);
     vec3 rwPos = vec3(normModTexCoord.x, normModTexCoord.y, depth.r * 65535.0);
    */

    vec4 depth = texture(tex, tex_coord);
    vec2 normTexCoord = vec2(tex_coord.x * 2.0 - 1.0,
                             tex_coord.y * 2.0 - 1.0);

    vec3 rwPos = vec3(normTexCoord.x, normTexCoord.y, depth.r * 65535.0);
    rwPos = getKinRealWorldCoord(rwPos);
    vec4 rotPos = invRotX * vec4(rwPos.x, rwPos.y, rwPos.z, 1.0);
    float deep = rotPos.z < deeperThres ? rotPos.z : 0.0;
    deep = deep > nearerThres ? 1.0 : 0.0;
    
    deep = (normTexCoord.x < cropRight) ? deep : 0.0;
    deep = (normTexCoord.x > cropLeft) ? deep : 0.0;
    
    color = vec4(deep, deep, deep, 1.0);
//    color = vec4(depth.r * 30.0, 0.0, 0.0, 1.0);
}