// Optical Flow Shader
// negative werte werden hier aus irgendwelchen gründen nicht gespeichert...
// deshalb vorzeichen kodieren, z = vorzeichen x, w = vorzeichen y (0.0 = negativ, 1.0 = positiv)
#version 410 core

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D last;

uniform vec2 scale;
uniform vec2 offset;
uniform float amp;
uniform float lambda;
uniform float median;

in vec2 tex_coord;
in vec4 col;
layout (location = 0) out vec4 color;


vec4 getColorCoded(float x, float y, vec2 scale)
{
    vec2 xout = vec2( max(x,0.), abs(min(x,0.)) ) *scale.x;
    vec2 yout = vec2( max(y,0.), abs(min(y,0.)) ) *scale.y;
    float dirY = 1.0;
    if (yout.x > yout.y)  dirY=0.90;
    return vec4(xout.xy, max(yout.x,yout.y), dirY);
}


vec4 getGrayScale(vec4 col)
{
    float gray = dot(vec3(col.x, col.y, col.z), vec3(0.3, 0.59, 0.11));
    return vec4(gray,gray,gray,1.0);
}


vec4 texture2DRectGray(sampler2D tex, vec2 coord)
{
    return getGrayScale(texture(tex, coord));
}


void main()
{
    vec4 a = texture2DRectGray(tex0, tex_coord);
    vec4 b = texture2DRectGray(tex1, tex_coord);
    vec2 x1 = vec2(offset.x,0.);
    vec2 y1 = vec2(0.,offset.y);

    //get the difference
    vec4 curdif = b-a;
    
    //calculate the gradient
    //for X________________
    vec4 gradx = texture2DRectGray(tex1, tex_coord+x1) -texture2DRectGray(tex1, tex_coord-x1);
    gradx += texture2DRectGray(tex0, tex_coord+x1) -texture2DRectGray(tex0, tex_coord-x1);
    
    
    //for Y________________
    vec4 grady = texture2DRectGray(tex1, tex_coord+y1) -texture2DRectGray(tex1, tex_coord-y1);
    grady += texture2DRectGray(tex0, tex_coord+y1) -texture2DRectGray(tex0, tex_coord-y1);
    
    vec4 gradmag = sqrt((gradx*gradx)+(grady*grady)+vec4(lambda));
    
    vec4 vx = curdif*(gradx/gradmag);
    vec4 vy = curdif*(grady/gradmag);
    
    vec4 lastFrame = texture(last, tex_coord);

//    color = vec4(vx.r, vy.r, 0.0, 1.0) * 3.0;
    color = vec4(pow(abs(vx.r) * 6.0, 1.7) * sign(vx.r) * amp,
                 pow(abs(vy.r) * 6.0, 1.7) * sign(vy.r) * amp,
                 0.0, 1.0);
}