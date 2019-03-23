//
// Fragment shader for environment mapping with an
// equirectangular 2D texture and refraction mapping
// with a background texture blended together using
// the fresnel terms

#version 410 core


const vec3 Xunitvec = vec3 (1.0, 0.0, 0.0);
const vec3 Yunitvec = vec3 (0.0, 1.0, 0.0);

uniform vec3  BaseColor;
uniform float Depth;
uniform float MixRatio;

uniform float alpha;

// need to scale our framebuffer - it has a fixed width/height of 2048
//uniform float FrameWidth;
//uniform float FrameHeight;
//uniform float textureWidth;
//uniform float textureHeight;

uniform samplerCube EnvMap;
//uniform sampler2D RefractionMap;

in vec4  col;
in vec3  Normal;
in vec3  EyeDir;
in vec4  EyePos;
in float LightIntensity;

layout (location = 0) out vec4 color;

void main (void)
{
    // Compute reflection vector
    vec3 reflectDir = reflect(EyeDir, Normal);
    
    // Do a lookup into the environment map.
    vec3 envColor = vec3(texture(EnvMap, reflectDir));
    
    // calc fresnels term.  This allows a view dependant blend of reflection/refraction
    float fresnel = abs(dot(normalize(EyeDir), Normal));
    fresnel *= MixRatio;
    fresnel = clamp(fresnel, 0.1, 0.9);
    
    // calc refraction
    vec3 refractionDir = normalize(EyeDir) - normalize(Normal);
    // vec3 RefractionColor = vec3 (texture(RefractionMap, refractionDir));
    vec3 RefractionColor = vec3 (0.0, 0.0, 0.0);
    
    // Add lighting to base color and mix
    vec3 base = LightIntensity * BaseColor;
    envColor = mix(envColor, RefractionColor, fresnel);
    envColor = mix(envColor, base, 0.2);
    
    color = vec4 (envColor, 1.0);
    //    color = vec4 (0.2, 0.0, 0.0, 0.5);
}