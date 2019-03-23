#version 410 core

uniform sampler2D tex;

uniform float dirW;
uniform float dirH;
uniform float colorThreshold1;
uniform float colorThreshold2;
uniform int maxLength1;
uniform int maxLength2;

in vec2 tex_coord;
in vec4 col;

layout (location = 0) out vec4 color;

float colorDiff(vec3 p1, vec3 p2)
{
    float colorDiff, diff = 0;
    
    for(int colInd = 0; colInd < 3; colInd++)
    {
        colorDiff = abs(p1[colInd] - p2[colInd]);
        diff = (diff > colorDiff) ? diff : colorDiff;
    }
 
    return diff;
}

void main()
{
    // reference pixel
    vec3 p = texture(tex, tex_coord).rgb;
    int d = 1;

    // pixel value of p1 predecessor
    vec3 p2 = p;
    float w1 = tex_coord.x + dirW;
    float h1 = tex_coord.y + dirH;
    
    // test if p1 is still inside the picture
    bool inside = (0 <= h1) && (h1 < 1.0) && (0 <= w1) && (w1 < 1.0);

    if(inside)
    {
        bool colorCond = true;
        bool wLimitCond = true;
        bool fColorCond = true;
     
        while(colorCond && wLimitCond && fColorCond && inside)
        {
            vec3 p1 = texture(tex, vec2(w1, h1)).rgb;
            
            // Do p1, p2 and p have similar color intensities?
            colorCond = colorDiff(p, p1) < colorThreshold1 && colorDiff(p1, p2) < colorThreshold1;
            
            // Is window limit not reached?
            wLimitCond = d < maxLength1;
            
            // Better color similarities for farther neighbors?
            fColorCond = (d <= maxLength2) || (d > maxLength2 && colorDiff(p, p1) < colorThreshold2);
            
            p2 = p1;
            h1 += dirH;
            w1 += dirW;
            
            // test if p1 is still inside the picture
            inside = (0 <= h1) && (h1 < 1.0) && (0 <= w1) && (w1 < 1.0);

            d++;
        }
        
        d--;
    } else
    {
        discard;
    }

//    color = vec4(tex_coord.x, 0.0, 0.0, 1.0);
    color = vec4(float(d) - 1.0, 0.0, 0.0, 1.0);
}