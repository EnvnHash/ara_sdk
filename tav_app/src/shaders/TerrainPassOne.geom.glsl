#version 410 core

// input has to be a grid centered in (0|0)
// with the size (1|1) !!!!

layout(points, invocations = 1) in;
layout(points, max_vertices = 64) out;

uniform float u_halfDetailStepS;
uniform float u_halfDetailStepT;
uniform uint u_detailLevel;
uniform float u_fovRadius;

uniform vec4 u_positionTextureSpace;
uniform vec3 u_leftNormalTextureSpace;
uniform vec3 u_rightNormalTextureSpace;
uniform vec3 u_backNormalTextureSpace;

uniform mat4 u_movMatr;
uniform mat4 u_rotMatr;

out vec2 v_vertex;

bool isOutside(vec4 point, vec4 viewPoint, float stepX, float stepY)
{
	float bias = 0.1;

	vec3 viewVector = vec3(point - viewPoint);

	float boundingRadius = sqrt(stepX*stepX+stepY*stepY);

	// Outside field of view
	if (length(viewVector) - boundingRadius > u_fovRadius)
	{
		return true;
	}

	// Behind the viewer
	if (dot(viewVector, u_backNormalTextureSpace) > boundingRadius + bias)
	{
		return true;
	}

	// Left side of field of view
	if (dot(viewVector, u_leftNormalTextureSpace) > boundingRadius + bias)
	{
		return true;
	}

	// Right side of field of view 
	if (dot(viewVector, u_rightNormalTextureSpace) > boundingRadius + bias)
	{
		return true;
	}
    
	return false;
}

vec4 getModVec4(vec4 inVec)
{
    vec4 modVec = u_movMatr * inVec;
    modVec.x = mod(modVec.x + 0.5, 1.0) - 0.5;
    modVec.z = mod(modVec.z + 0.5, 1.0) - 0.5;
    modVec = u_rotMatr * modVec;
    return modVec;
}

vec2 getModVec2(vec4 inVec)
{
    vec4 modVec = getModVec4(inVec);
    return vec2(modVec.x, modVec.z);
}

void main(void)
{
    /*
     int i;
     for (i=0;i<gl_in.length();i++)// gl_in.length() = 1 though!
     {
     vec4 pos = u_movMatr * gl_in[i].gl_Position;
     v_vertex = vec2(pos.x, pos.z);
     
     gl_Position = gl_in[i].gl_Position;
     EmitVertex();
     }
     EndPrimitive();
*/
    
    uint steps = uint(pow(2.0, float(u_detailLevel)));
    
    float finalDetailStepS = u_halfDetailStepS * 2.0 / float(steps);
    //float halfFinalDetailStepS = finalDetailStepS / 2.0;
    
    float finalDetailStepT = u_halfDetailStepT * 2.0 / float(steps);
    //float halfFinalDetailStepT = finalDetailStepT / 2.0;
    
//    vec4 centerPoint;
    float xFloat;
    float zFloat;
    
    vec4 movePoint = u_movMatr * gl_in[0].gl_Position;
    
    // check if the transformed point will be wrapped, if yes, skip it
    vec4 leftLowerPoint = vec4(movePoint.x - u_halfDetailStepS, 0.0, movePoint.z - u_halfDetailStepT, 1.0);
    leftLowerPoint.x = mod(leftLowerPoint.x + 0.5, 1.0) - 0.5;
    leftLowerPoint.z = mod(leftLowerPoint.z + 0.5, 1.0) - 0.5;

    vec4 rightUpperPoint = vec4(movePoint.x + u_halfDetailStepS, 0.0, movePoint.z + u_halfDetailStepT, 1.0);
    rightUpperPoint.x = mod(rightUpperPoint.x + 0.5, 1.0) - 0.5;
    rightUpperPoint.z = mod(rightUpperPoint.z + 0.5, 1.0) - 0.5;

    if ((leftLowerPoint.x > 0.0 && rightUpperPoint.x < 0.0)
        || (leftLowerPoint.z > 0.0 && rightUpperPoint.z < 0.0))
    {
        return;
    }
    
    if (isOutside(getModVec4(gl_in[0].gl_Position), u_positionTextureSpace, u_halfDetailStepS, u_halfDetailStepT))
	{
		return;
	}
    
	for (uint z = 0; z < steps; z++)
	{
		zFloat = float(z);
        
		for (uint x = 0; x < steps; x++)
		{	
			xFloat = float(x);


//			centerPoint = vec4(gl_in[0].gl_Position.x + xFloat * finalDetailStepS - u_halfDetailStepS + halfFinalDetailStepS,
//                               0.0,
//                               gl_in[0].gl_Position.z + zFloat * finalDetailStepT - u_halfDetailStepT + halfFinalDetailStepT,
//                               1.0);
/*
            leftLowerPoint = vec4(gl_in[0].gl_Position.x + xFloat * finalDetailStepS - u_halfDetailStepS,
                                  0.0,
                                  gl_in[0].gl_Position.z + zFloat * finalDetailStepT - u_halfDetailStepT,
                                  1.0);
			if (isOutside(getModVec4(leftLowerPoint), u_positionTextureSpace, halfFinalDetailStepS, halfFinalDetailStepT))
			{
				continue;
			}
            
            rightUpperPoint = vec4(gl_in[0].gl_Position.x + (xFloat + 1.0) * finalDetailStepS - u_halfDetailStepS,
                                   0.0,
                                   gl_in[0].gl_Position.z + (zFloat + 1.0) * finalDetailStepT - u_halfDetailStepT,
                                   1.0);
            if (isOutside(getModVec4(rightUpperPoint), u_positionTextureSpace, halfFinalDetailStepS, halfFinalDetailStepT))
            {
                continue;
            }
*/
            
			v_vertex = getModVec2(vec4(gl_in[0].gl_Position.x + xFloat * finalDetailStepS - u_halfDetailStepS,
                                       0.0,
                                       gl_in[0].gl_Position.z + zFloat * finalDetailStepT - u_halfDetailStepT,
                                       1.0));
			EmitVertex();
			EndPrimitive();

            v_vertex = getModVec2(vec4(gl_in[0].gl_Position.x + (xFloat + 1.0) * finalDetailStepS - u_halfDetailStepS,
                                       0.0,
                                       gl_in[0].gl_Position.z + zFloat * finalDetailStepT - u_halfDetailStepT,
                                       1.0));
			EmitVertex();
			EndPrimitive();

            v_vertex = getModVec2(vec4(gl_in[0].gl_Position.x + (xFloat + 1.0) * finalDetailStepS - u_halfDetailStepS,
                                       0.0,
                                       gl_in[0].gl_Position.z + (zFloat + 1.0) * finalDetailStepT - u_halfDetailStepT,
                                       1.0));
			EmitVertex();
			EndPrimitive();

            v_vertex = getModVec2(vec4(gl_in[0].gl_Position.x + xFloat * finalDetailStepS - u_halfDetailStepS,
                                       0.0,
                                       gl_in[0].gl_Position.z + (zFloat + 1.0) * finalDetailStepT - u_halfDetailStepT,
                                       1.0));
			EmitVertex();
			EndPrimitive();
		}
	}
}
