// PartikelSystem vertex update Shader
//
// position -> gl_Position
// velocity : wird nicht benötigt, also ignoriert (einkommentieren?)
// aux0 -> x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

#version 410 core
#pragma optimize(on)

layout(location=0) in vec4 position;
layout(location=5) in vec4 velocity;
layout(location=6) in vec4 aux0;
layout(location=7) in vec4 aux1;

layout(location=0) out vec4 rec_position;
layout(location=5) out vec4 rec_velocity;
layout(location=6) out vec4 rec_aux0;
layout(location=7) out vec4 rec_aux1;

uniform float dt;
uniform float lifeTime;
uniform float aging;
uniform float friction;
uniform float ageFading;
uniform float ageSizing;
uniform vec3 gravity;
uniform samplerBuffer emitTbo0;
uniform samplerBuffer emitTbo1;
uniform samplerBuffer emitTbo2;
uniform samplerBuffer emitTbo3;
uniform sampler2D emitTex;

uniform mat4 m_pvm;

void main()
{
    float sizeFact = pow(1.0/(dt+1.0), 2.0);
    vec4 emitPos = texelFetch(emitTbo0, gl_VertexID);

    // wenn das emit-flag gesetzt, also die emit-positions-koordinate w grösser als 0 ist
    // emitiere ein neues partikel
    if ( emitPos.w > 0.0 )
    {
        vec4 emitVel = texelFetch(emitTbo1, gl_VertexID);
        vec4 emitAux0 = texelFetch(emitTbo2, gl_VertexID);
        vec4 emitAux1 = texelFetch(emitTbo3, gl_VertexID);

        rec_position = vec4(emitPos.x, emitPos.y, emitPos.z, 0.0);
        rec_velocity = vec4(emitVel.rgb + gravity, emitVel.w);
        rec_aux0 = vec4(max(emitAux0.x * (sizeFact * ageSizing + 1.0 -ageSizing), 0.005), emitAux0.y, emitAux0.z, emitAux0.w);
        rec_aux1 = vec4(lifeTime * emitPos.w, 1.0, 0.0, emitAux1.a); // aux1.a = alpha

    } else
    {
        if (aux1.x > 0.0)
        {
            rec_position = vec4(position.x, position.y, position.z, 1.0) + vec4(velocity.rg * dt, -0.2, 0.0);
            
            //rec_velocity = vec4(0.1, 0.0, 0.0, 0.0);
            rec_velocity = vec4((velocity.rgb + gravity) * friction, velocity.w);
            
            rec_aux0 = vec4(max(aux0.x * (sizeFact * ageSizing + 1.0 -ageSizing), 0.005),
                            aux0.y,
                            aux0.z + velocity.w,
                            aux0.w);
            rec_aux1.x = aux1.x - (dt * aging);
            rec_aux1.a = max(rec_aux1.x, ageFading); // copy initial lifetime, if agefading is disable this will stay 0

        } else
        {
            // wenn nicht, lösche das partikel
            rec_position = vec4(0.0, 0.0, 2.0, 0.0);
            rec_velocity = velocity;
            rec_aux0 = vec4(0);
            rec_aux1 = vec4(0);
        }
    }
    gl_Position = m_pvm * position;
}