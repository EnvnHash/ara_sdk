// PartikelSystem record geometrie shader für blending
// GLSLParticleSystem2 update Shader
#version 410 core
#pragma optimize(on)

layout (points) in;
layout (points, max_vertices = 1) out;

in VS_GS_VERTEX {
    vec4 position;
    vec4 velocity;
    vec4 aux0;
    vec4 aux1;
} vertex_in[];

uniform int checkBounds;
uniform float dt;
uniform float lifeTime;
uniform float aging;
uniform float friction;
uniform float ageFading;
uniform float ageSizing;
uniform vec3 gravity;

layout(location=0) out vec4 rec_position;
layout(location=5) out vec4 rec_velocity;
layout(location=6) out vec4 rec_aux0;
layout(location=7) out vec4 rec_aux1;

// aux0 -> aux0: (x: size, y: farbIndex (0-1), z: angle, w: textureUnit)
// aux1 -> aux1: x: lifetime, y: min-Lifetime (um aging an und aus zu schalten), z:, w: alpha

void main()
{
    float sizeFact = pow(1.0/(dt+1.0), 2.0);
    float newLifeTime = vertex_in[0].aux1.x - (dt * aging);
    vec4 newPos = vec4(vertex_in[0].position.x, vertex_in[0].position.y, vertex_in[0].position.z, 1.0)
                + vec4(vertex_in[0].velocity.rgb * dt, 0.0);
    
    // if the particle still lives, emit it
    int bounds = int(newPos.x > 1.0) + int(newPos.x < -1.0) + int(newPos.y < -1.0) + int(newPos.y > 1.0) + int(newLifeTime <= 0.0);
    
    if (bounds *checkBounds == 0)
    {
        rec_position = newPos;
        
        rec_velocity = vec4((vertex_in[0].velocity.rgb + gravity) * friction, vertex_in[0].velocity.w);
        
        rec_aux0 = vec4(max(vertex_in[0].aux0.x * (sizeFact * ageSizing + 1.0 -ageSizing), 0.005),
                        vertex_in[0].aux0.y,
                        vertex_in[0].aux0.z + vertex_in[0].velocity.w,
                        vertex_in[0].aux0.w);
        
        rec_aux1.x = newLifeTime;
        
        rec_aux1.a = max(rec_aux1.x, ageFading); // copy initial lifetime, if agefading is disable this will stay 0
        
        gl_Position = rec_position;
        
        EmitVertex();
        EndPrimitive();
    }
}