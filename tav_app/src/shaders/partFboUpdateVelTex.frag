// GLSLParticleSystemFBO Update Shader with external Velocity Texture
// pos.w = lifetime
#version 410 core
#pragma optimize(on)

uniform sampler2D pos_tex;
uniform sampler2D vel_tex;
uniform sampler2D col_tex;
uniform sampler2D aux0_tex;
uniform sampler2D extVel_tex;   // emitvel text is supposed to have only valid x and y values
uniform sampler2D extForce_tex; // external force, like wind...

uniform float dt;
uniform float doAging;
uniform float doAgeFading;
uniform float friction;
uniform float extVelScale;
uniform int texSize;
uniform vec2 extFOffs;

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
    //int drawCond = int(pos.x >= 1.02) + int(pos.x <= -1.02) + int(pos.y >= 1.02) + int(pos.y <= -1.02);

    // switching doesn´t work, since we are double buffering...
    int drawCond = int(pos.a < 0.001);
    
    if (drawCond == 0) // ist eigentlich unnötig, macht der shader von allein...
    {
        // movement
        // convert Particel position to velocity_texture_coordinate
        vec2 velTexCoord = vec2(pos.x * 0.5 + 0.5, 1.0 - (pos.y * 0.5 + 0.5));
        vec4 extVel = vec4(texture(extVel_tex, velTexCoord).xy * extVelScale, 0.0, 0.0);
        vec4 extVelRight = texture(extVel_tex, vec2(velTexCoord.x + 0.05, velTexCoord.y));

        vec2 extForceCoord = vec2((pos.x + extFOffs.x) * 0.1, (pos.y + extFOffs.y) * 0.1);
        vec4 extForce = texture(extForce_tex, extForceCoord);

        getCol = texelFetch(col_tex, itex_coord, 0);
        //getCol.rgb -= (dt * extForce.xyz) * 0.25;

        extForce -= 0.5;
        extForce.z *= 2.0;
        extForce += 0.3;
    
        // velocity has to be written with a > 0.f  (a == 0.f is ignored by the shader)
        pos += ((extVel * -1.0) + extForce * 0.4) * dt;

        aux0 = texelFetch(aux0_tex, itex_coord, 0);
        // angle offset
        float angleOffset = dot(extVel.xy, extVelRight.xy);
        aux0.z += angleOffset * 0.05;
        
        // lifetime and agefading
        color = (getCol - (aux0.x * dt * doAging)) * doAgeFading
                + (1.0 - doAgeFading) * getCol;
    } else
    {
        discard;
    }
}