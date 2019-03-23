//
// TessTerrain.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "TessTerrain.h"

#define STRINGIFY(A) #A

namespace tav
{
TessTerrain::TessTerrain(ShaderCollector* _shCol, unsigned int _quality) :
		shCol(_shCol), mQuality(_quality), mCull(true), mLod(true), mAnimate(
				true), mWireframe(false)
{
	int noiseSize = 256;
	int noiseSize3D = 64;

	testVAO = new VAO("position:3f", GL_STATIC_DRAW);
	GLfloat pos[] =
			{ -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f,
					0.5f, 0.0f };
	testVAO->upload(POSITION, pos, 4);
	GLuint ind[] =
	{ 0, 1, 3, 1, 3, 2 };
	testVAO->setElemIndices(6, ind);

	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 0.f,
			0.f, 1.f);  // color will be replace when rendering with blending on
	quad->translate(0.f, 0.f, 0.9999f);

	mLightDir = glm::normalize(glm::vec3(-1.0f, -0.25f, 1.0f));
	mParams.heightScale = 0.6f;
	mParams.invNoiseSize = 1.0f / noiseSize;
	mParams.invNoise3DSize = 1.0f / noiseSize3D;
	updateQuality(); // init mParams


	//----------------------

	loadShaders();

	// Create a single default program pipeline to handle binding and unbinding
	// separate shader objects
	glGenProgramPipelines(1, &mTerrainPipeline);
	glBindProgramPipeline(mTerrainPipeline);

	//create ubo and initialize it with the structure data
	glGenBuffers(1, &mUBO);
	glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
	glBufferData( GL_UNIFORM_BUFFER, sizeof(TessellationParams), &mParams,
			GL_STREAM_DRAW);

	//create simple single-vertex VBO
	float vtx_data[] =
	{ 0.0f, 0.0f, 0.0f, 1.0f };
	glGenBuffers(1, &mVBO);
	glBindBuffer( GL_ARRAY_BUFFER, mVBO);
	glBufferData( GL_ARRAY_BUFFER, sizeof(vtx_data), vtx_data, GL_STATIC_DRAW);

	srand(0);

	mRandTex = createNoiseTexture2D(noiseSize, noiseSize, GL_R16F);
	mRandTex3D = createNoiseTexture4f3D(noiseSize3D, noiseSize3D, noiseSize3D,
			GL_RGBA16F);

	glGenQueries(1, &mGPUQuery);

	getGlError();
}

//----------------------------------------------------

TessTerrain::~TessTerrain()
{
	delete quad;
}

//----------------------------------------------------

void TessTerrain::loadShaders()
{
	shdr_Header = "#version 440\n";
//        shdr_Header = "#version 440\n#pragma optimize(on)\n";
//        shdr_Header = "#version 440\n#pragma optimize(on)\n #extension GL_ARB_enhanced_layouts : enable\n";

	initUniformHeader();
	initTerrainFunc();
	initNoiseFunc();
	initNoise3DFunc();

	initGenTerrainVs();
	initGenTerrainFs();
	initTerrainVertex();
	initTerrainControl();
	initTerrainTess();
	initTerrainFrag();
	initWireFrameGeo();
	initWireFrameFrag();

	mTerrainVertexProg = createShaderPipelineProgram( GL_VERTEX_SHADER,
			terrain_vertex.c_str());
	mTerrainTessControlProg = createShaderPipelineProgram(
			GL_TESS_CONTROL_SHADER, terrain_control.c_str());
	mTerrainTessEvalProg = createShaderPipelineProgram(
			GL_TESS_EVALUATION_SHADER, terrain_tess.c_str());
	mWireframeGeometryProg = createShaderPipelineProgram( GL_GEOMETRY_SHADER,
			wireframe_geometry.c_str());
	mTerrainFragmentProg = createShaderPipelineProgram( GL_FRAGMENT_SHADER,
			terrain_frag.c_str());
	mWireframeFragmentProg = createShaderPipelineProgram( GL_FRAGMENT_SHADER,
			fragment_wireframe.c_str());

	GLint loc;

	getGlError();
	loc = glGetUniformLocation(mTerrainTessEvalProg, "terrainTex");
	getGlError();
	if (loc >= 0)
	{
		glProgramUniform1i(mTerrainTessEvalProg, loc, 2);
	}
	getGlError();

	mGenerateTerrainProg = shCol->addCheckShaderText("TessTerrain_genterrain",
			generate_terrain_vs.c_str(), generate_terrain_fs.c_str());
	getGlError();
	mGenerateTerrainProg->begin();
	mGenerateTerrainProg->setUniform1i("randTex", 0);
	mGenerateTerrainProg->end();
	getGlError();

	initSkyVs();
	initSkyFs();

	mSkyProg = shCol->addCheckShaderText("TessTerrain_sky", sky_vs.c_str(),
			sky_fs.c_str());

	mSkyProg->begin();
	mSkyProg->setUniform1i("randTex3D", 0);
	mSkyProg->end();
}

//----------------------------------------------------

void TessTerrain::initTerrainControl()
{
	terrain_control =
			shdr_Header + "#extension GL_ARB_tessellation_shader : enable\n"
					+ uniforms_header + "#line 9\n"
					+ STRINGIFY(

					// control/hull shader
					// executed once per input patch, computes LOD (level of detail) and performs culling
							layout(vertices=1) out;\n \n in gl_PerVertex {\n vec4 gl_Position;\n } gl_in[];\n \n layout(location=1) in block {\n mediump vec2 texCoord;\n } In[];\n \n \n out gl_PerVertex {\n vec4 gl_Position;\n } gl_out[];\n \n layout(location=1) out block {\n mediump vec2 texCoord;\n vec2 tessLevelInner;\n } Out[];\n \n
							//layout(location=1) out vec2 tessLevelInner[];
							\n
							// test if sphere is entirely contained within frustum planes
							bool sphereInFrustum(vec3 pos, float r, vec4 plane[6])\n {\n for(int i=0; i<6; i++) {\n if (dot(vec4(pos, 1.0), plane[i]) + r < 0.0) {\n
							// sphere outside plane\n\n
							return false;\n }\n }\n return true;\n }\n

							// transform from world to screen coordinates
							vec2 worldToScreen(vec3 p)\n {\n vec4 r = ModelViewProjection * vec4(p, 1.0); \n // to clip space
							r.xy /= r.w; \n// project
							r.xy = r.xy*0.5 + 0.5;\n// to NDC
							r.xy *= viewport.zw;\n// to pixels
							return r.xy;\n } \n
							// calculate edge tessellation level from two edge vertices in screen space
							float calcEdgeTessellation(vec2 s0, vec2 s1)\n {\n float d = distance(s0, s1);\n return clamp(d / triSize, 1, 64);\n }\n \n vec2 eyeToScreen(vec4 p)\n {\n vec4 r = Projection * p;\n // to clip space
							r.xy /= r.w; \n// project
							r.xy = r.xy*0.5 + 0.5; \n// to NDC
							r.xy *= viewport.zw;\n// to pixels
							return r.xy;\n }\n \n
							// calculate tessellation level by fitting sphere to edge
							float calcEdgeTessellationSphere(vec3 w0, vec3 w1, float diameter)\n {\n vec3 centre = (w0 + w1) * 0.5;\n vec4 view0 = ModelView * vec4(centre, 1.0);\n vec4 view1 = view0 + vec4(diameter, 0, 0, 0);\n vec2 s0 = eyeToScreen(view0);\n vec2 s1 = eyeToScreen(view1);\n return calcEdgeTessellation(s0, s1);\n }\n \n void main() {\n gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n Out[gl_InvocationID].texCoord = In[gl_InvocationID].texCoord;\n \n
							// bounding sphere for patch\n
							vec3 spherePos = gl_in[gl_InvocationID].gl_Position.xyz + vec3(tileSize.x, heightScale, tileSize.z)*0.5;\n \n
							// test if patch is visible
							bool visible = sphereInFrustum(spherePos, tileBoundingSphereR, frustumPlanes);\n \n if (cull && !visible) {\n
							// cull patch\n
							gl_TessLevelOuter[0] = -1;\n gl_TessLevelOuter[1] = -1;\n gl_TessLevelOuter[2] = -1;\n gl_TessLevelOuter[3] = -1;\n

							gl_TessLevelInner[0] = -1;\n gl_TessLevelInner[1] = -1;\n } else {\n if (lod) {\n
							// compute automatic LOD

							// calculate edge tessellation levels
							// see tessellation diagram in OpenGL 4 specification for vertex order details
							vec3 v0 = gl_in[0].gl_Position.xyz;\n vec3 v1 = v0 + vec3(tileSize.x, 0, 0);\n vec3 v2 = v0 + vec3(tileSize.x, 0, tileSize.z);\n vec3 v3 = v0 + vec3(0, 0, tileSize.z);\n)
							+ "#if 0\n" + STRINGIFY(

							// use screen-space length of each edge
							// this method underestimates tessellation level when looking along edge
							vec2 s0 = worldToScreen(v0);\n
							vec2 s1 = worldToScreen(v1);\n
							vec2 s2 = worldToScreen(v2);\n
							vec2 s3 = worldToScreen(v3);\n

							gl_TessLevelOuter[0] = calcEdgeTessellation(s3, s0);\n
							gl_TessLevelOuter[1] = calcEdgeTessellation(s0, s1);\n
							gl_TessLevelOuter[2] = calcEdgeTessellation(s1, s2);\n
							gl_TessLevelOuter[3] = calcEdgeTessellation(s2, s3);\n)
							+ "#else\n" + STRINGIFY(
							// use screen space size of sphere fit to each edge
							float sphereD = tileSize.x*2.0f;
							gl_TessLevelOuter[0] = calcEdgeTessellationSphere(v3, v0, sphereD);\n
							gl_TessLevelOuter[1] = calcEdgeTessellationSphere(v0, v1, sphereD);\n
							gl_TessLevelOuter[2] = calcEdgeTessellationSphere(v1, v2, sphereD);\n
							gl_TessLevelOuter[3] = calcEdgeTessellationSphere(v2, v3, sphereD);\n)
							+ "#endif\n" + STRINGIFY(
							\n
							// calc interior tessellation level - use average of outer levels
							gl_TessLevelInner[0] = 0.5 * (gl_TessLevelOuter[1] + gl_TessLevelOuter[3]);\n
							gl_TessLevelInner[1] = 0.5 * (gl_TessLevelOuter[0] + gl_TessLevelOuter[2]);\n
							\n
							Out[gl_InvocationID].tessLevelInner = vec2(gl_TessLevelInner[0], gl_TessLevelInner[1]);\n
							\n
						}
						else
						{	\n
							gl_TessLevelOuter[0] = outerTessFactor;\n
							gl_TessLevelOuter[1] = outerTessFactor;\n
							gl_TessLevelOuter[2] = outerTessFactor;\n
							gl_TessLevelOuter[3] = outerTessFactor;\n
							\n
							gl_TessLevelInner[0] = innerTessFactor;\n
							gl_TessLevelInner[1] = innerTessFactor;\n
							\n
							Out[gl_InvocationID].tessLevelInner = vec2(innerTessFactor);\n
						}\n
					}\n
				});
}

//----------------------------------------------------

void TessTerrain::initTerrainVertex()
{
	terrain_vertex =
			shdr_Header + "#extension GL_EXT_shader_io_blocks : enable\n"
					+ uniforms_header
					+ STRINGIFY(
							layout(location=0) in vec4 vertex;\n \n out gl_PerVertex {\n vec4 gl_Position;\n };\n \n layout(location=1) out block {\n mediump vec2 texCoord;\n } Out;\n \n void main() {\n

							// position patch in 2D grid based on instance ID
							int ix = gl_InstanceID % gridW;\n int iy = gl_InstanceID / gridH;\n

							Out.texCoord = vec2(float(ix) / float(gridW), float(iy) / float(gridH));\n

							//gl_Position = vertex;
							vec3 pos = gridOrigin + vec3(float(ix)*tileSize.x, 0, float(iy)*tileSize.z);\n gl_Position = vec4(pos, 1.0f);\n });
}

//----------------------------------------------------

void TessTerrain::initTerrainTess()
{
	terrain_tess =
			shdr_Header
					+ "#extension GL_ARB_tessellation_shader : enable\n #define PROCEDURAL_TERRAIN 1\n"
					+ uniforms_header + noise_func + terrain_func + "#line 8 \n"
					+ STRINGIFY(
							layout(quads, fractional_even_spacing, cw) in;\n \n in gl_PerVertex {\n vec4 gl_Position;\n } gl_in[];\n \n layout(location=1) in block {\n mediump vec2 texCoord;\n vec2 tessLevelInner;\n } In[];\n \n out gl_PerVertex {\n vec4 gl_Position;\n };\n \n layout(location=1) out block {\n vec3 vertex;\n vec3 vertexEye;\n vec3 normal;\n } Out;\n \n uniform sampler2D terrainTex;\n \n void main(){\n \n vec3 pos = gl_in[0].gl_Position.xyz;\n pos.xz += gl_TessCoord.xy * tileSize.xz;\n \n)
							+ "#if PROCEDURAL_TERRAIN\n" +
							STRINGIFY(
							// calculate terrain height procedurally
							float h = terrain(pos.xz);\n
							vec3 n = vec3(0, 1, 0);\n
							pos.y = h;\n
							\n
							// calculate normal
							vec2 triSize = tileSize.xz / In[0].tessLevelInner;\n
							vec3 pos_dx = pos.xyz + vec3(triSize.x, 0.0, 0.0);\n
							vec3 pos_dz = pos.xyz + vec3(0.0, 0.0, triSize.y);\n
							pos_dx.y = terrain(pos_dx.xz);\n
							pos_dz.y = terrain(pos_dz.xz);\n
							n = normalize(cross(pos_dz - pos.xyz, pos_dx - pos.xyz));\n
							\n)
							+ "#else\n" + STRINGIFY(
							\n
							// read from pre-calculated texture
							vec2 uv = In[0].texCoord + (vec2(1.0 / gridW, 1.0 / gridH) * gl_TessCoord.xy);\n
							vec4 t = texture2D(terrainTex, uv);\n
							float h = t.w;\n
							pos.y = t.w;\n
							vec3 n = t.xyz;\n)
							+ "#endif\n" + STRINGIFY(
							\n
							Out.normal = n;\n
							\n
							Out.vertex = pos;\n
							Out.vertexEye = vec3(ModelView * vec4(pos, 1)); \n// eye space
							\n
							gl_Position = ModelViewProjection * vec4(pos, 1);\n
						});
}

//----------------------------------------------------

void TessTerrain::initTerrainFrag()
{
	terrain_frag =
			shdr_Header + uniforms_header + noise_func + noise3D_func
					+ STRINGIFY(
							layout(location=1) in block {\n vec3 vertex;\n vec3 vertexEye;\n vec3 normal;\n } In;\n \n layout(location=0) out vec4 fragColor;\n \n float saturate(float v) {\n return clamp( v, 0.0, 1.0);\n }\n \n
							// cheaper than smoothstep
							float linearstep(float a, float b, float x)\n {\n return saturate((x - a) / (b - a));\n }\n\n)

							+ "#define smoothstep linearstep\n" + STRINGIFY(
									\n
									const vec3 sunColor = vec3(1.0, 1.0, 0.7);\n
									const vec3 lightColor = vec3(1.0, 1.0, 0.7)*1.5;\n
									const vec3 fogColor = vec3(0.7, 0.8, 1.0)*0.7;\n
									\n
									const float fogExp = 0.1;\n
									\n
									vec3 applyFog(vec3 col, float dist)\n
									{	\n
										float fogAmount = exp(-dist*fogExp);\n
										return mix(fogColor, col, fogAmount);\n
									}\n
									\n
									// fog with scattering effect
									// http://www.iquilezles.org/www/articles/fog/fog.htm
									vec3 applyFog(vec3 col, float dist, vec3 viewDir)\n
									{	\n
										float fogAmount = exp(-dist*fogExp);\n
										float sunAmount = max(dot(viewDir, lightDirWorld), 0.0);\n
										sunAmount = pow(sunAmount, 32.0);\n
										vec3 fogCol = mix(fogColor, sunColor, sunAmount);\n
										return mix(fogCol, col, fogAmount);\n
									}\n
									\n
									vec3 shadeTerrain(vec3 vertex,\n
											vec3 vertexEye,\n
											vec3 normal\n
									)\n
									{	\n
										const float shininess = 100.0;\n
										const vec3 ambientColor = vec3(0.05, 0.05, 0.15 );\n
										const float wrap = 0.3;\n
										\n
										vec3 rockColor = vec3(0.4, 0.4, 0.4 );\n
										vec3 snowColor = vec3(0.9, 0.9, 1.0 );\n
										vec3 grassColor = vec3(77.0 / 255.0, 100.0 / 255.0, 42.0 / 255.0 );\n
										vec3 brownColor = vec3(82.0 / 255.0, 70.0 / 255.0, 30.0 / 255.0 );\n
										vec3 waterColor = vec3(0.2, 0.4, 0.5 );\n
										vec3 treeColor = vec3(0.0, 0.2, 0.0 );\n
										\n
										//vec3 noisePos = vertex.xyz + vec3(translate.x, 0.0, translate.y);
										vec3 noisePos = vertex.xyz;\n
										float nois = noise(noisePos.xz)*0.5+0.5;\n
										\n
										float height = vertex.y;\n
										\n
										// snow\n
										float snowLine = 0.7;\n
										//float snowLine = 0.6 + nois*0.1;
										float isSnow = smoothstep(snowLine, snowLine+0.1, height * (0.5+0.5*normal.y));\n
										\n
										// lighting
										\n
										// world-space
										vec3 viewDir = normalize(eyePosWorld.xyz - vertex);\n
										vec3 h = normalize(-lightDirWorld + viewDir);\n
										vec3 n = normalize(normal);\n

										//float diffuse = saturate( dot(n, -lightDir));
										float diffuse = saturate( (dot(n, -lightDirWorld) + wrap) / (1.0 + wrap));\n// wrap
										//float diffuse = dot(n, -lightDir)*0.5+0.5;
										float specular = pow( saturate(dot(h, n)), shininess);\n\n)

								+ "#if 0\n" + STRINGIFY(
										\n
										// add some noise variation to colors
										grassColor = mix(grassColor*0.5, grassColor*1.5, nois);\n
										brownColor = mix(brownColor*0.25, brownColor*1.5, nois);\n\n
								)
								+ "#endif\n" + STRINGIFY(
										\n
										// choose material color based on height and normal
										\n
										vec3 matColor;\n
										matColor = mix(rockColor, grassColor, smoothstep(0.6, 0.8, normal.y));\n
										matColor = mix(matColor, brownColor, smoothstep(0.9, 1.0, normal.y ));\n
										// snow
										matColor = mix(matColor, snowColor, isSnow);\n
										\n
										float isWater = smoothstep(0.05, 0.0, height);\n
										matColor = mix(matColor, waterColor, isWater);\n
										\n
										vec3 finalColor = ambientColor*matColor + diffuse*matColor*lightColor + specular*lightColor*isWater;
										\n
										// fog\n
										float dist = length(vertexEye);\n
										//finalColor = applyFog(finalColor, dist);
										finalColor = applyFog(finalColor, dist, viewDir);\n
										\n
										return finalColor;\n
										\n
										//return vec3(dist);
										//return vec3(dnoise(vertex.xz).z*0.5+0.5);
										//return normal*0.5+0.5;
										//return vec3(normal.y);
										//return vec3(fogCoord);
										//return vec3(diffuse);
										//return vec3(specular);
										//return diffuse*matColor + specular.xxx
										//return matColor;
										//return vec3(occ);
										//return vec3(sun)*sunColor;
										//return noise2*0.5+0.5;
									}\n
									\n
									void main()\n
									{	\n
										fragColor = vec4(shadeTerrain(In.vertex.xyz, In.vertexEye.xyz, In.normal), 1.0);\n // shade per pixel
									});
}

//----------------------------------------------------

void TessTerrain::initGenTerrainFs()
{
	generate_terrain_fs =
			shdr_Header + uniforms_header + noise_func + terrain_func
					+ STRINGIFY(
							in block { mediump vec2 texCoord; } In;

							layout(location=0) out vec4 fragColor;

							void main() { vec3 pos = gridOrigin + vec3(In.texCoord.x * float(gridW) * tileSize.x, 0.0, In.texCoord.y * float(gridH) * tileSize.z); float h = terrain(pos.xz); pos.y = h;

							// calculate normal
							vec2 triSize = tileSize.xz / 64.0; vec3 pos_dx = pos.xyz + vec3(triSize.x, 0.0, 0.0); vec3 pos_dz = pos.xyz + vec3(0.0, 0.0, triSize.y); pos_dx.y = terrain(pos_dx.xz); pos_dz.y = terrain(pos_dz.xz); vec3 n = normalize(cross(pos_dz - pos.xyz, pos_dx - pos.xyz));

							fragColor = vec4(n, h); });
}

//----------------------------------------------------

void TessTerrain::initWireFrameGeo()
{
	wireframe_geometry =
			shdr_Header + uniforms_header
					+ STRINGIFY(
							layout(triangles, invocations = 1) in;\n layout(line_strip, max_vertices = 3) out;\n \n in gl_PerVertex {\n vec4 gl_Position;\n } gl_in[];\n \n layout(location=1) in block {\n vec3 vertex;\n vec3 vertexEye;\n vec3 normal;\n } In[];\n \n out gl_PerVertex\n {\n vec4 gl_Position;\n };\n \n void main()\n {\n gl_Position = gl_in[0].gl_Position;\n EmitVertex();\n \n gl_Position = gl_in[1].gl_Position;\n EmitVertex();\n \n gl_Position = gl_in[2].gl_Position;\n EmitVertex();\n \n EndPrimitive();\n });
}

//----------------------------------------------------

void TessTerrain::initWireFrameFrag()
{
	fragment_wireframe =
			shdr_Header + uniforms_header
					+ STRINGIFY(
							layout(location=0) out vec4 fragColor;\n void main()\n {\n fragColor = vec4(0.0, 0.0, 0.0, 1.0);\n });
}

//----------------------------------------------------

void TessTerrain::initUniformHeader()
{
	uniforms_header =
			STRINGIFY(
					layout(std140, binding=1) uniform TessellationParams {\n float innerTessFactor;\n float outerTessFactor;\n

					float noiseFreq;\n int noiseOctaves;\n float invNoiseSize;\n float invNoise3DSize;\n float heightScale;\n

					float triSize;\n vec4 viewport;\n

					mat4 ModelView;\n mat4 ModelViewProjection;\n mat4 Projection;\n mat4 InvProjection;\n mat4 InvView;\n

					bool smoothNormals;\n bool cull;\n bool lod;\n vec3 lightDir;\n vec3 lightDirWorld;\n vec4 eyePosWorld;\n

					vec4 frustumPlanes[6];\n

					float time;\n vec2 translate;\n

					int gridW;\n int gridH;\n vec3 tileSize;\n vec3 gridOrigin;\n float tileBoundingSphereR;\n

					float invFocalLen;\n };\n);
}

//----------------------------------------------------

void TessTerrain::initNoiseFunc()
{
	noise_func =
			STRINGIFY(uniform mediump sampler2D randTex;\n \n
			// smooth interpolation curve
					vec2 fade(vec2 t)\n {\n
					//return t * t * (3 - 2 * t); // old curve (quadratic)
					return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);\n// new curve (quintic)
					}\n \n
					// derivative of fade function
					vec2 dfade(vec2 t)\n {\n return 30.0*t*t*(t*(t-2.0)+1.0); \n // new curve (quintic)
					}\n \n
					// 2D noise using 2D texture lookup
					// note - artifacts may be visible at low frequencies due to hardware interpolation precision

					// returns value in [-1, 1]
					float noise(vec2 p)\n {\n return texture(randTex, p * invNoiseSize).x*2.0-1.0;\n }\n \n
					// with smooth software interpolation

					float smoothnoise(vec2 p)\n {\n vec2 i = floor(p);\n vec2 f = p - float(i);\n f = fade(f);\n \n
					// use texture gather - returns 4 neighbouring texels in a single lookup
					// note weird ordering of returned components!
					vec4 n = textureGather(randTex, i * invNoiseSize)*2.0-1.0;\n return mix( mix( n.w, n.z, f.x),\n mix( n.x, n.y, f.x), f.y);\n }\n

					// 2D noise with derivatives
					// returns derivative in xy, normal noise value in z
					// http://www.iquilezles.org/www/articles/morenoise/morenoise.htm
					\n vec3 dnoise(vec2 p)\n {\n vec2 i = floor(p);\n vec2 u = p - i;\n \n vec2 du = dfade(u);\n u = fade(u);\n \n
					// get neighbouring noise values
					vec4 n = textureGather(randTex, i * invNoiseSize)*2.0-1.0;\n \n
					// rename components for convenience
					float a = n.w;\n float b = n.z;\n float c = n.x;\n float d = n.y;\n \n float k0 = a;\n float k1 = b - a;\n float k2 = c - a;\n float k3 = a - b - c + d;\n \n vec3 r;\n
					// noise derivative
					r.xy = (vec2(k1, k2) + k3*u.yx) * du;\n
					// noise value
					r.z = k0 + k1*u.x + k2*u.y + k3*u.x*u.y;\n return r;\n }\n \n
					// rotate octaves to avoid axis-aligned artifacts
					const mat2 rotateMat = mat2(1.6, -1.2, 1.2, 1.6);\n \n
					// fractal sum
					float fBm(vec2 p, int octaves, float lacunarity, float gain)\n {\n float amp = 0.5;\n float sum = 0.0;\n for(int i=0; i<octaves; i++) {\n sum += noise(p)*amp;\n p = rotateMat * p;\n
					//p *= lacunarity;\n
					amp *= gain;\n }\n return sum;\n }\n \n
					// fbm with gradients (iq style)
					float fBmGrad(vec2 p, int octaves, float lacunarity, float gain)\n {\n float amp = 0.5;\n vec2 d = vec2(0.0);\n float sum = 0.0;\n for(int i=0; i<octaves; i++) {\n vec3 n = dnoise(p);\n d += n.xy;\n sum += n.z*amp / (1.0 + dot(d, d));\n // sum scaled by gradient
					amp *= gain;\n
					//p *= lacunarity;
					p = rotateMat * p;\n }\n return sum;\n }\n \n float turbulence(vec2 p, int octaves, float lacunarity, float gain)\n {\n float sum = 0.0;\n float amp = 0.5;\n for(int i=0; i<octaves; i++) {\n sum += abs(noise(p))*amp;\n
					//p *= lacunarity;
					p = rotateMat * p;\n amp *= gain;\n }\n return sum;\n }\n \n
					// Ridged multifractal
					// See "Texturing & Modeling, A Procedural Approach", Chapter 12
					float ridge(float h, float offset)\n {\n h = abs(h); \n // create creases
					h = offset - h;\n// invert so creases are at top
					h = h * h; \n// sharpen creases
					return h;\n }\n \n float ridgedMF(vec2 p, int octaves, float lacunarity, float gain, float offset)\n {\n float sum = 0.0;\n float freq = 1.0;\n float amp = 0.5;\n float prev = 1.0;\n for(int i=0; i<octaves; i++) {\n float n = ridge(smoothnoise(p*freq), offset);\n
					//sum += n*amp;\n
					sum += n*amp*prev;\n// scale by previous octave
					prev = n;\n freq *= lacunarity;\n amp *= gain;\n }\n return sum;\n }\n \n
					// mixture of ridged and fbm noise
					float hybridTerrain(vec2 x, int octaves)\n {\n float h = ridgedMF(x, 2, 2.0, 0.5, 1.1);\n float f = fBm(x * 4.0, octaves - 2, 2.0, 0.5) * 0.5;\n return h + f*h;\n }\n);
}

//----------------------------------------------------

void TessTerrain::initNoise3DFunc()
{
	noise3D_func =
			STRINGIFY(uniform mediump sampler3D randTex3D;\n \n
			// smooth interpolation curve
					vec3 fade(vec3 t)\n {\n
					//return t * t * (3 - 2 * t); // old curve (quadratic)
					return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); \n// new curve (quintic)
					}\n \n
					// returns value in [-1, 1]
					float noise(vec3 p)\n {\n return texture(randTex3D, p * invNoise3DSize).x*2.0-1.0;\n }\n \n vec4 noise4f(vec3 p)\n {\n return texture(randTex3D, p * invNoise3DSize)*2.0-1.0;\n }\n \n
					// fractal sum
					float fBm(vec3 p, int octaves, float lacunarity, float gain)\n {\n float amp = 0.5;\n float sum = 0.0;\n for(int i=0; i<octaves; i++) {\n sum += noise(p)*amp;\n
					//p = rotateMat * p;
					p *= lacunarity;\n amp *= gain;\n }\n return sum;\n }\n \n vec4 fBm4f(vec3 p, int octaves, float lacunarity, float gain)\n {\n float amp = 0.5;\n vec4 sum = vec4(0.0);\n for(int i=0; i<octaves; i++) {\n sum += noise4f(p)*amp;\n p *= lacunarity;\n amp *= gain;\n }\n return sum;\n }\n \n float turbulence(vec3 p, int octaves, float lacunarity, float gain)\n {\n float amp = 0.5;\n float sum = 0.0;\n for(int i=0; i<octaves; i++) {\n sum += abs(noise(p))*amp;\n
					//p = rotateMat * p;
					p *= lacunarity;\n amp *= gain;\n }\n return sum;\n }\n \n vec4 turbulence4f(vec3 p, int octaves, float lacunarity, float gain)\n {\n float amp = 0.5;\n vec4 sum = vec4(0.0);\n for(int i=0; i<octaves; i++) {\n sum += abs(noise4f(p))*amp;\n p *= lacunarity;\n amp *= gain;\n }\n return sum;\n }\n\n);
}

//----------------------------------------------------

void TessTerrain::initTerrainFunc()
{
	terrain_func =
			STRINGIFY(// terrain height function, returns height at given position
					float terrain(vec2 p) { p += translate;\n p *= noiseFreq;\n

					//float h = sin(p.x)*cos(p.y);
					//float h = texture2D(randTex, p * invNoiseSize).x;
					//float h = noise(p);
					//float h = fBm(p, noiseOctaves);
					//float h = fBm(p, noiseOctaves)*0.5+0.5;
					//float h = 0.6 - turbulence(p, noiseOctaves);
					//float h = ridgedMF(p, noiseOctaves);
					//float h = dnoise(p);
					//float h = hybridTerrain(p, noiseOctaves)-0.2;
					float h = fBmGrad(p, noiseOctaves, 2.0, 0.5) + 0.2;\n

					//h = 1.0 - h;
					//h = h*h;
					//h = sqrt(h);

					h *= heightScale;\n

					const float waterLevel = 0.05;\n float land = smoothstep(waterLevel, waterLevel+0.2, h);\n
					// scale down terrain at shore
					h *= 0.1 + 0.9*land;\n

					return h;\n }\n);
}

// -------------------------------------------------------------------------------------

void TessTerrain::initGenTerrainVs()
{
	generate_terrain_vs =
			shdr_Header
					+ STRINGIFY(
							layout(location=0) in vec4 vertex;\n layout(location=8) in vec2 texCoord;\n

							out gl_PerVertex {\n vec4 gl_Position;\n };\n

							out block {\n mediump vec2 texCoord;\n } Out;\n

							void main()\n {\n Out.texCoord = texCoord;\n gl_Position = vertex;\n });
}

// -------------------------------------------------------------------------------------

void TessTerrain::initSkyVs()
{
	sky_vs =
			shdr_Header + uniforms_header + noise3D_func
					+ STRINGIFY(layout(location=0) in vec4 vertex;

					out gl_PerVertex { vec4 gl_Position; };

					out block { vec4 pos; vec4 posEye; } Out;

					void main() {
					// transform from clip back to eye space
							Out.posEye = InvProjection * vertex; Out.posEye /= Out.posEye.w;

							Out.pos = InvView * Out.posEye;// world space

							gl_Position = vertex; });
}

// -------------------------------------------------------------------------------------

void TessTerrain::initSkyFs()
{
	sky_fs =
			shdr_Header + uniforms_header + noise3D_func
					+ STRINGIFY(
							in block { vec4 pos; vec4 posEye; } In;

							layout(location=0) out vec4 fragColor;

							const vec3 skyColor = vec3(0.7, 0.8, 1.0)*0.7; const vec3 fogColor = vec3(0.8, 0.8, 1.0); const vec3 cloudColor = vec3(1.0); const vec3 sunColor = vec3(1.0, 1.0, 0.3); uniform float skyHeight = 5.0; uniform float skyTop = 6.0; const int cloudSteps = 8; const float cloudStepSize = 1.0; const float cloudDensity = 0.25;

							vec4 cloudMap(vec3 p) { float d = turbulence(p*0.1 + vec3(time*0.05, -time*0.05, 0), 4, 2.0, 0.5); d = smoothstep(0.2, 0.5, d); // threshold density

							float c = smoothstep(skyHeight, skyTop, p.y)*0.5+0.5;// darken base
							return vec4(c, c, c, d*cloudDensity); }

							vec4 rayMarchClouds(vec3 ro, vec3 rd, float stepsize) { vec4 sum = vec4(0); vec3 p = ro; vec3 step = rd * stepsize;

							// ray march front to back
							for(int i=0; i<cloudSteps; i++) { vec4 col = cloudMap(p); col.rgb *= col.a; // pre-multiply alpha
							sum = sum + col*(1.0 - sum.a); p += step; }

							return sum; }

							float intersectPlane(vec3 n, float d, vec3 ro, vec3 rd) { return (-d - dot(ro, n)) / dot(rd, n); }

							vec4 sky(vec3 ro, vec3 rd) {
							// intersect ray with sky plane
							float t = intersectPlane(vec3(0.0, -1.0, 0.0), skyHeight, ro, rd); float tfar = intersectPlane(vec3(0.0, -1.0, 0.0), skyTop, ro, rd);

							float stepsize = (tfar - t) / float(cloudSteps);

							vec4 c = vec4(0.0); if (t > 0.0 && rd.y > 0.0) { vec3 hitPos = ro.xyz + t*rd; hitPos.xz += translate; c = rayMarchClouds(hitPos, rd, stepsize); }

							// fade with angle
							c *= smoothstep(0.0, 0.1, rd.y);// clouds

							// add sky
							vec3 sky = mix(skyColor, fogColor, pow(min(1.0, 1.0-rd.y), 10.0));

							// add sun under clouds
							float sun = pow( max(0.0, dot(rd, -lightDirWorld)), 500.0); sky += sunColor*sun;

							vec4 skyA = vec4(sky, 1.0);

							//c.rgb = c.rgb + sky*(1.0 - c.a);
							c = mix(c, skyA, 1.0 - c.a);

							return c; }

							void main() {
							// calculate ray in world space
							vec3 eyePos = eyePosWorld.xyz; vec3 viewDir = normalize(In.pos.xyz - eyePos);

							fragColor = sky(eyePos, viewDir); });
}

// -------------------------------------------------------------------------------------

GLuint TessTerrain::createShaderPipelineProgram(GLuint target, const char* src)
{
	GLuint object;
	GLint status;
	const char* m_shaderPrefix = "";
//        const char* m_shaderPrefix = "#version 440\n";

	const GLchar* fullSrc[2] =
	{ m_shaderPrefix, src };
	object = glCreateShaderProgramv(target, 2, fullSrc); // with this command GL_PROGRAM_SEPARABLE is set to true
	glGetProgramiv(object, GL_LINK_STATUS, &status);

	if (!status)
	{
		GLint charsWritten, infoLogLength;
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &infoLogLength);
		char * infoLog = new char[infoLogLength];
		glGetProgramInfoLog(object, infoLogLength, &charsWritten, infoLog);
		std::cerr << "Error compiling " << GetShaderStageName(target)
				<< std::endl;
		std::cerr << "Log: " << infoLog << std::endl;
		delete[] infoLog;

		glDeleteProgram(object);
		object = 0;
	}

	return object;
}

//----------------------------------------------------

void TessTerrain::checkProgPipeline()
{
	GLint status;

//    	    	glBindProgramPipeline(m_programPipeline);
	//  	    	glUseProgramStages(m_programPipeline, GL_COMPUTE_SHADER_BIT, object);
	glValidateProgramPipeline(mTerrainPipeline);
	glGetProgramPipelineiv(mTerrainPipeline, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE)
	{
		GLint logLength;
		glGetProgramPipelineiv(mTerrainPipeline, GL_INFO_LOG_LENGTH,
				&logLength);
		char *log = new char[logLength];
		glGetProgramPipelineInfoLog(mTerrainPipeline, logLength, 0, log);
		printf("Shader pipeline not valid:\n%s\n", log);
		delete[] log;
	}

	//	glBindProgramPipeline(0);
	getGlError();
}

//----------------------------------------------------

const char* TessTerrain::GetShaderStageName(GLenum target)
{
	switch (target)
	{
	case GL_VERTEX_SHADER:
		return "VERTEX_SHADER";
		break;
	case GL_GEOMETRY_SHADER_EXT:
		return "GEOMETRY_SHADER";
		break;
	case GL_FRAGMENT_SHADER:
		return "FRAGMENT_SHADER";
		break;
	case GL_TESS_CONTROL_SHADER:
		return "TESS_CONTROL_SHADER";
		break;
	case GL_TESS_EVALUATION_SHADER:
		return "TESS_EVALUATION_SHADER";
		break;
	case GL_COMPUTE_SHADER:
		return "COMPUTE_SHADER";
		break;
	}
	return "";
}

//----------------------------------------------------

/*
 void TessTerrain::updateTerrainTex()
 {
 mTerrainFbo->bind();
 getGlError();
 glDisable(GL_DEPTH_TEST);
 getGlError();
 glDisable(GL_BLEND);

 getGlError();
 // bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
 glBindBufferBase( GL_UNIFORM_BUFFER, 1, mUBO);
 glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
 getGlError();
 glBufferSubData( GL_UNIFORM_BUFFER, 0, sizeof(TessellationParams), &mParams);
 getGlError();

 mGenerateTerrainProg->begin();
 getGlError();

 glActiveTexture(GL_TEXTURE0);
 getGlError();
 glBindTexture(GL_TEXTURE_2D, mRandTex);
 getGlError();

 // NvDrawQuadGL(0, 8);
 quad->draw();
 getGlError();

 mGenerateTerrainProg->end();
 getGlError();
 }
 */

//----------------------------------------------------
void TessTerrain::computeFrustumPlanes(glm::mat4 &viewMatrix,
		glm::mat4 &projMatrix, glm::vec4 *plane)
{
	glm::mat4 viewProj = projMatrix * viewMatrix;
	plane[0] = glm::row(viewProj, 3) + glm::row(viewProj, 0);   // left
	plane[1] = glm::row(viewProj, 3) - glm::row(viewProj, 0);   // right
	plane[2] = glm::row(viewProj, 3) + glm::row(viewProj, 1);   // bottom
	plane[3] = glm::row(viewProj, 3) - glm::row(viewProj, 1);   // top
	plane[4] = glm::row(viewProj, 3) + glm::row(viewProj, 2);   // far
	plane[5] = glm::row(viewProj, 3) - glm::row(viewProj, 2);   // near
	// normalize planes
	for (int i = 0; i < 6; i++)
	{
		float l = glm::length(glm::vec3(plane[i]));
		plane[i] = plane[i] / l;
	}
}

//----------------------------------------------------

bool TessTerrain::sphereInFrustum(glm::vec3 pos, float r, glm::vec4 *plane)
{
	glm::vec4 hp = glm::vec4(pos, 1.0f);
	for (int i = 0; i < 6; i++)
	{
		if (dot(hp, plane[i]) + r < 0.0f)
		{
			// sphere outside plane
			return false;
		}
	}
	return true;

}

//----------------------------------------------------

void TessTerrain::drawTerrain(TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->pauseAndOffsetBuf(GL_TRIANGLES);
		glBeginQuery(GL_PRIMITIVES_GENERATED_EXT, mGPUQuery);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glUseProgram(0); // disable all shaders, otherwise the prog pipeline doesnt work
	testVAO->bind(); // vertex daten fuer das patch

	//set up the program stage independently
	glBindProgramPipeline(mTerrainPipeline);

	glUseProgramStages(mTerrainPipeline, GL_VERTEX_SHADER_BIT,
			mTerrainVertexProg);
	//checkProgPipeline();

	glUseProgramStages(mTerrainPipeline, GL_TESS_CONTROL_SHADER_BIT,
			mTerrainTessControlProg);
	//checkProgPipeline();

	glUseProgramStages(mTerrainPipeline, GL_TESS_EVALUATION_SHADER_BIT,
			mTerrainTessEvalProg);
	//checkProgPipeline();

	if (mWireframe)
	{
		glUseProgramStages(mTerrainPipeline, GL_GEOMETRY_SHADER_BIT,
				mWireframeGeometryProg);
		glUseProgramStages(mTerrainPipeline, GL_FRAGMENT_SHADER_BIT,
				mWireframeFragmentProg);
	}
	else
	{
		glUseProgramStages(mTerrainPipeline, GL_GEOMETRY_SHADER_BIT, 0);
		//checkProgPipeline();

		glUseProgramStages(mTerrainPipeline, GL_FRAGMENT_SHADER_BIT,
				mTerrainFragmentProg);
		//checkProgPipeline();

	}
	getGlError();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mRandTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, mRandTex3D);

	//draw patches
	glPatchParameteri( GL_PATCH_VERTICES, 1);

	int instances = mParams.gridW * mParams.gridH;
	glDrawArraysInstanced( GL_PATCHES, 0, 1, instances);

	glBindProgramPipeline(0);

	glDisable(GL_CULL_FACE);

	// wenn für transform feedback gerendert wird
	if (_tfo)
	{
		glEndQuery(GL_PRIMITIVES_GENERATED_EXT);
		glGetQueryObjectuiv(mGPUQuery, GL_QUERY_RESULT, &mNumPrimitives);
		_tfo->incCounters(mNumPrimitives);
	}
}

//----------------------------------------------------

void TessTerrain::drawSky(TFO* _tfo)
{
	glEnable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mRandTex3D);

	mSkyProg->begin();
	mSkyProg->setUniform1f("skyTop", skyTop);
	mSkyProg->setUniform1f("skyHeight", skyHeight);

	quad->draw(_tfo);

	mSkyProg->end();

	glBindTexture(GL_TEXTURE_3D, 0);
}

//----------------------------------------------------

void TessTerrain::updateQuality()
{
	switch (mQuality)
	{
	case 0:
		mParams.gridW = mParams.gridH = 16;
		mParams.tileSize = glm::vec3(1.0f, 0.0f, 1.0f);
		mParams.noiseOctaves = 8;
		break;
	case 1:
		mParams.gridW = mParams.gridH = 32;
		mParams.tileSize = glm::vec3(0.5f, 0.0f, 0.5f);
		mParams.noiseOctaves = 9;
		break;
	case 2:
		mParams.gridW = mParams.gridH = 64;
		mParams.tileSize = glm::vec3(0.25f, 0.0f, 0.25f);
		mParams.noiseOctaves = 10;
		break;
	case 3:
		mParams.gridW = mParams.gridH = 128;
		mParams.tileSize = glm::vec3(0.125f, 0.0f, 0.125f);
		mParams.noiseOctaves = 11;
		break;
	}

	mParams.gridOrigin = glm::vec3(
			-mParams.tileSize.x * float(mParams.gridW) * 0.5f, 0.0f,
			-mParams.tileSize.z * float(mParams.gridH) * 0.5f);

	glm::vec3 halfTileSize = glm::vec3(mParams.tileSize.x, mParams.heightScale,
			mParams.tileSize.z) * 0.5f;
	mParams.tileBoundingSphereR = glm::length(halfTileSize);

	// update texture
	//initTerrainFbo();
	//updateTerrainTex();
}

//----------------------------------------------------

void TessTerrain::draw(double time, double dt, glm::mat4& projMat,
		glm::mat4& viewMat, glm::mat4& modelMat, glm::vec2 actFboSize,
		TFO* _tfo)
{
	glEnable(GL_DEPTH_TEST);

	glm::mat4 invProjection = glm::inverse(projMat);
	glm::mat4 viewMatrix = viewMat * modelMat;
	glm::mat4 invView = glm::inverse(viewMatrix);

	// compute frustum planes for culling
	glm::vec4 frustumPlanes[6];
	computeFrustumPlanes(viewMatrix, projMat, frustumPlanes);

	glBindProgramPipeline(0);

	// update struct representing UBO
	//
	mParams.ModelView = viewMatrix;
	mParams.ModelViewProjection = projMat * viewMatrix;
	mParams.Projection = projMat;
	mParams.InvProjection = invProjection;
	mParams.InvView = invView;

	mParams.cull = mCull;
	mParams.lod = mLod;
	mParams.viewport = glm::vec4(0.f, 0.f, actFboSize.x, actFboSize.y);
	mParams.lightDirWorld = mLightDir;
	mParams.lightDir = glm::vec3(
			viewMatrix * glm::vec4(normalize(mLightDir), 0.0)); // transform to eye space
	mParams.time = float(time);
	mParams.eyePosWorld = invView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	mParams.heightScale = heightScale;

	if (mAnimate)
	{
		mParams.translate.y -= float(dt) * animSpeed;
	}

	for (int i = 0; i < 6; i++)
	{
		mParams.frustumPlanes[i] = frustumPlanes[i];
	}

	// bind the buffer for the UBO, and update it with the latest values from the CPU-side struct
	glBindBufferBase( GL_UNIFORM_BUFFER, 1, mUBO);
	glBindBuffer( GL_UNIFORM_BUFFER, mUBO);
	glBufferSubData( GL_UNIFORM_BUFFER, 0, sizeof(TessellationParams),
			&mParams);

	// enable / disable wireframe
	//glPolygonMode( GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);

	drawTerrain(_tfo);
	drawSky(_tfo);
}

//---------------------------------------------------------

// create 2D texture containing random values
GLuint TessTerrain::createNoiseTexture2D(int w, int h, GLint internalFormat,
		bool mipmap)
{
	float *data = new float[w * h];
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			data[y * w + x] = frand();
		}
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RED, GL_FLOAT,
			data);
	if (mipmap)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	delete[] data;
	return tex;
}

//---------------------------------------------------------

// create 2D noise texture with neighbouring values in RGBA
GLuint TessTerrain::createNoiseTexture2DNeighbours(int w, int h,
		GLuint internalFormat)
{
	// 2d random texture
	float *v = new float[w * h * 4];
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			v[0 + 4 * (x + w * y)] = (rand() / (float) RAND_MAX);
			//v(x, y, 0) = (rand() / (float) RAND_MAX)*2.0f-1.0f;
		}
	}

	// store neighbouring values in rgba components
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			v[1 + 4 * (x + w * y)] = v[0 + 4 * ((x + 1) + w * y)];
			v[2 + 4 * (x + w * y)] = v[0 + 4 * (x + w * (y + 1))];
			v[3 + 4 * (x + w * y)] = v[0 + 4 * ((x + 1) + w * (y + 1))];
		}
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, GL_FLOAT,
			v);

	delete[] v;
	return tex;
}

//---------------------------------------------------------

// create 3D texture
GLuint TessTerrain::createNoiseTexture3D(int w, int h, int d,
		GLint internalFormat, bool mipmap)
{
	float *data = new float[w * h * d];
	for (int z = 0; z < d; z++)
	{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				data[z * (w * h) + (y * w) + x] = frand();
			}
		}
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);

	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER,
			mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RED, GL_FLOAT,
			data);
	if (mipmap)
	{
		glGenerateMipmap(GL_TEXTURE_3D);
	}

	delete[] data;
	return tex;
}

//---------------------------------------------------------

GLuint TessTerrain::createNoiseTexture4f3D(int w, int h, int d,
		GLint internalFormat, bool mipmap)
{
	float *data = new float[w * h * d * 4];
	float *ptr = data;
	for (int z = 0; z < d; z++)
	{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				*ptr++ = frand();
				*ptr++ = frand();
				*ptr++ = frand();
				*ptr++ = frand();
			}
		}
	}

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);

	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER,
			mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RGBA,
			GL_FLOAT, data);
	if (mipmap)
	{
		glGenerateMipmap(GL_TEXTURE_3D);
	}

	delete[] data;
	return tex;
}

//---------------------------------------------------------

void TessTerrain::setHeight(float _val)
{
	heightScale = _val;
}

//---------------------------------------------------------

void TessTerrain::setAnimSpeed(float _val)
{
	animSpeed = _val;
}

//---------------------------------------------------------

void TessTerrain::setSkyTop(float _val)
{
	skyTop = _val;
}

//---------------------------------------------------------

void TessTerrain::setSkyHeight(float _val)
{
	skyHeight = _val;
}
}
