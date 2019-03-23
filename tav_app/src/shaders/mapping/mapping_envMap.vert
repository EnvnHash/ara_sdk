// basic texShader
#version 410 core

layout( location = 0 ) in vec4 position;
layout( location = 1 ) in vec4 normal;
layout( location = 2 ) in vec2 texCoord;
layout( location = 3 ) in vec4 color;

uniform float lookAngle;
uniform float zObjPos;

uniform mat4 persp_matr;

uniform mat4 viewMatrix; // world to view transformation
uniform mat4 viewMatrixInverse;
uniform mat4 modelMatrix; // world to view transformation
uniform mat4 m_pvm;
uniform mat3 m_normal;

vec4 pos;

// view to world transformation
out vec3 viewDirection; // direction in world space
// in which the viewer is looking
out vec3 normalDirection; // normal vector in world space

void main()
{
    vec4 positionInViewSpace = viewMatrix * modelMatrix * position;
    
    // transformation of gl_Vertex from object coordinates
    // to view coordinates
    vec4 viewDirectionInViewSpace = positionInViewSpace - vec4(0.0, 0.0, 0.0, 1.0);
    
    // camera is always at (0,0,0,1) in view coordinates;
    // this is the direction in which the viewer is looking
    // (not the direction to the viewer)
    viewDirection = vec3(viewMatrixInverse * viewDirectionInViewSpace);
    
    // transformation from view coordinates to world coordinates
    vec3 normalDirectionInViewSpace = m_normal * vec3(normal);
    
    // transformation of gl_Normal from object coordinates
    // to view coordinates
    normalDirection = normalize( vec3( vec4(normalDirectionInViewSpace, 0.0) * viewMatrix ) );
    

    //tex_coord = texCoord;
    
    pos = m_pvm * position;
    pos.y += tan(lookAngle) * (pos.z - zObjPos);
        
    gl_Position = persp_matr * pos;
}