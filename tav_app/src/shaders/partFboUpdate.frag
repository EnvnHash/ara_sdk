// GLSLParticleSystemFBO Update Shader
// pos.w = lifetime
#version 410 core
#pragma optimize(on)

uniform sampler2D pos_tex;
uniform sampler2D vel_tex;
uniform sampler2D col_tex;
uniform sampler2D aux0_tex;

uniform float dt;
uniform float doAging;
uniform float doAgeFading;
uniform float friction;
uniform int texSize;

in vec2 tex_coord;

layout (location = 0) out vec4 pos;
layout (location = 1) out vec4 vel;
layout (location = 2) out vec4 color;
layout (location = 3) out vec4 aux0;

vec4 getCol, getVel;

// pos.w = actual lifetime (normalized 0-1)
// pos.x = lifetime decrement factor
void main()
{
    ivec2 itex_coord = ivec2(int(tex_coord.x * texSize), int(tex_coord.y * texSize));
    pos = texelFetch(pos_tex, itex_coord, 0);

    // border check, limit is higher than 1.02 because of double buffering and discard (0 won´t be written twice)
    int drawCond = int(pos.x >= 1.02) + int(pos.x <= -1.02) + int(pos.y >= 1.02) + int(pos.y <= -1.02);
    // switching doesn´t work, since we are double buffering...
    drawCond += int(pos.a < 0.002);
    
    if (drawCond == 0) // ist eigentlich unnötig, macht der shader von allein...
    {
        // velocity has to be written with a > 0.f  (a == 0.f is ignored by the shader)
        getVel = texelFetch(vel_tex, itex_coord, 0) * friction;
        vel = getVel;
    
        // movement
        pos += vec4(getVel.xyz, 0.0) * dt;
        
        getCol = texelFetch(col_tex, itex_coord, 0);
        aux0 = texelFetch(aux0_tex, itex_coord, 0);
        
        // lifetime agefading
        color = (getCol - (aux0.x * dt * doAging)) * doAgeFading
                + (1.0 - doAgeFading) * getCol;
    } else
    {
        discard;
    }
}