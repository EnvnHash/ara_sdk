// ad shader
#version 410 core

uniform sampler2D leftImg;
uniform sampler2D rightImg;
uniform float lambdaAD;
uniform float lambdaCensus;
uniform float width_step;
uniform float height_step;
uniform int censHHalf;
uniform int censWHalf;

in vec2 tex_coord_l[8];
in vec2 tex_coord_r[8];
in vec4 col;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 color1;
layout (location = 2) out vec4 color2;
layout (location = 3) out vec4 color3;
layout (location = 4) out vec4 color4;
layout (location = 5) out vec4 color5;
layout (location = 6) out vec4 color6;
layout (location = 7) out vec4 color7;

vec3 colorRefL, colorRefR, colorLP, colorRP;
vec2 offs;

float[8] dist;
float[8] adDist;
float[8] censusDist;
bool diff;

void main()
{
    float censWHalfStep = censWHalf * width_step;
    float censHHalfStep = 0.0;
    
    for (int i=0;i<8;i++)
    {
        dist[i] = 0.0;
        
        if( tex_coord_l[i].x - censWHalfStep < 0
           || tex_coord_r[i].x + censWHalfStep >= 1.0)
        {
            discard;
        } else
        {
            // if one of the texture coordinates is outside the visible
            // range, skip calculation
            colorRefL = texture(leftImg, tex_coord_l[i]).rgb;
            colorRefR = texture(rightImg, tex_coord_r[i]).rgb;

            // compute Absolute Difference cost
            adDist[i] = 0.0;
            for(int colInd=0;colInd<3;colInd++)
                adDist[i] += abs(colorRefL[colInd] - colorRefR[colInd]);
            adDist[i] *= 84.99999;

            // compute Census cost
            censusDist[i] = 0.0;
            
            for (int h = -censHHalf; h <= censHHalf; ++h)
            {
                censHHalfStep = float(h) * height_step;

                if( tex_coord_l[i].y + censHHalfStep < 0 || tex_coord_l[i].y + censHHalfStep >= 1.0
                   || tex_coord_r[i].y + censHHalfStep < 0 || tex_coord_r[i].y + censHHalfStep >= 1.0)
                {
                    discard;
                } else
                {
                    for (int w = -censWHalf; w <= censWHalf; ++w)
                    {
                        offs = vec2(width_step * float(w), height_step * float(h));
                        colorLP = texture(leftImg, tex_coord_l[i] + offs).rgb;
                        colorRP = texture(rightImg, tex_coord_r[i] + offs).rgb;

                        diff = (colorLP.r - colorRefL.r) * (colorRP.r - colorRefR.r) < 0;
                        censusDist[i] += diff ? 1:0;
                        
                        diff = (colorLP.g - colorRefL.g) * (colorRP.g - colorRefR.g) < 0;
                        censusDist[i] += diff ? 1:0;
                        
                        diff = (colorLP.b - colorRefL.b) * (colorRP.b - colorRefR.b) < 0;
                        censusDist[i] += diff ? 1:0;
                    }
                }
           }
            
            // combine the two costs
            dist[i] = (1 - exp(-adDist[i] / lambdaAD));
            dist[i] += (1 - exp(-censusDist[i] / lambdaCensus));
            dist[i] *= 0.5;
        }
    }

    color = vec4(vec3(dist[0]), 1.0);
    color1 = vec4(vec3(dist[1]), 1.0);
    color2 = vec4(vec3(dist[2]), 1.0);
    color3 = vec4(vec3(dist[3]), 1.0);
    color4 = vec4(vec3(dist[4]), 1.0);
    color5 = vec4(vec3(dist[5]), 1.0);
    color6 = vec4(vec3(dist[6]), 1.0);
    color7 = vec4(vec3(dist[7]), 1.0);
}