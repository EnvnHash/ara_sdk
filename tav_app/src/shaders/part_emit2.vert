// GLSLParticleSystem2 simple Emit-Vertex-Shader
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
uniform float ageSizing;
uniform vec3 gravity;

uniform mat4 m_pvm;

void main()
{
    float sizeFact = pow(1.0/(dt+1.0), 2.0);

    rec_position = vec4(position.x, position.y, position.z, 0.0);
    rec_velocity = vec4(velocity.rgb + gravity, velocity.w);
    rec_aux0 = vec4(max(aux0.x * (sizeFact * ageSizing + 1.0 -ageSizing), 0.005), aux0.y, aux0.z, aux0.w);
    rec_aux1 = vec4(lifeTime * position.w, 1.0, 0.0, aux1.a); // aux1.a = alpha
    
    gl_Position = m_pvm * position;
}