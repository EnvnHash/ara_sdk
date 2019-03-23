#version 410 core

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 3) out;

uniform sampler2DRect u_heightMapTexture;
uniform sampler2DRect u_normalMapTexture;

uniform mat4 u_tmvpMatrix;
uniform mat4 u_movMatr;
uniform mat4 u_rotMatr;

uniform vec3 u_lightDirection;

out vec2 v_texCoord;
out float v_intensity;


void main(void)
{
	ivec2 heightMapTextureSize = textureSize(u_heightMapTexture);

	vec4 heightMapPosition;
    vec4 heightMapPosition2;
	
	vec3 normal;
	
    vec2 offsPos;
    
	for(int i = 0; i < gl_in.length(); ++i)
	{
        heightMapPosition2 = u_movMatr * u_rotMatr * gl_in[i].gl_Position;
        
        offsPos = vec2(mod(heightMapPosition2.x, 1.0) * heightMapTextureSize.x,
                       mod(heightMapPosition2.z, 1.0) * heightMapTextureSize.y);
        
        v_texCoord = vec2(mod(heightMapPosition2.x, 1.0),
                          mod(heightMapPosition2.z, 1.0));
        
        // Normals in the normal map represent world space coordinates ...
        normal = texture(u_normalMapTexture, offsPos).xyz * 2.0 - 1.0;
        
        // ... so calculations are in world space
        v_intensity = max(dot(normalize(normal), u_lightDirection), 0.0) + 0.2;
        
        
        heightMapPosition = gl_in[i].gl_Position;
        heightMapPosition.y = texture(u_heightMapTexture, offsPos).r;

        gl_Position = u_tmvpMatrix * heightMapPosition;
        
		EmitVertex();
	}

	EndPrimitive();
}
