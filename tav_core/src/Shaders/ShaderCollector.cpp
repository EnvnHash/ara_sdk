//
//  ShaderCollector.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "Shaders/ShaderCollector.h"

#define STRINGIFY(A) #A

namespace tav
{
ShaderCollector::ShaderCollector()
{
	shdr_Header = "#version 450\n#pragma optimize(on)\n";
	shaderCollection = std::map<std::string, Shaders*>();
}

//-----------------------------------------------------------------

ShaderCollector::~ShaderCollector()
{
	shaderCollection.clear();
}

//-----------------------------------------------------------------

void ShaderCollector::addShader(std::string _name, Shaders* _shader)
{
	shaderCollection[_name] = _shader;
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShader(std::string _name, const char* vert,
		const char* frag)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(vert, frag, false);
		shaderCollection[_name]->link();
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShader(std::string _name, const char* vert,
		const char* geom, const char* frag)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(vert, geom, frag, false);
		shaderCollection[_name]->link();
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShader(std::string _name, const char* vert,
		const char* cont, const char* eval, const char* geom, const char* frag)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders();
		shaderCollection[_name]->addVertSrc(vert, false);
		shaderCollection[_name]->addContrSrc(cont, false);
		shaderCollection[_name]->addEvalSrc(eval, false);
		shaderCollection[_name]->addGeomSrc(geom, false);
		shaderCollection[_name]->addFragSrc(frag, false);
		shaderCollection[_name]->link();
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderText(std::string _name, const char* comp)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(comp, false);
		shaderCollection[_name]->link();
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderText(std::string _name,
		const char* vert, const char* frag)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(vert, frag, false);
		shaderCollection[_name]->link();
	}
	else
	{
		std::cerr << "Warning ShaderCollector::addCheckShaderText trying to overwrite Shader "
				<< _name << std::endl;
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderText(std::string _name,
		const char* vert, const char* geom, const char* frag)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(vert, geom, frag, false);
		shaderCollection[_name]->link();
	}
	else
	{
		std::cerr << "Warning ShaderCollector::addCheckShaderText trying to overwrite Shader "
				<< _name << std::endl;
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderText(std::string _name,
		const char* vert, const char* cont, const char* eval, const char* geom,
		const char* frag)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(vert, cont, eval, geom, frag, false);
		shaderCollection[_name]->link();
	}
	else
	{
		std::cerr << "Warning ShaderCollector::addCheckShaderText trying to overwrite Shader "
				<< _name << std::endl;
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderTextNoLink(std::string _name, const char* comp)
{
	if (!hasShader(_name))
	{
		shaderCollection[_name] = new Shaders(comp, false);
	}

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderTextNoLink(std::string _name,
		const char* vert, const char* frag)
{
	if (!hasShader(_name))
		shaderCollection[_name] = new Shaders(vert, frag, false);

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::addCheckShaderTextNoLink(std::string _name,
		const char* vert, const char* geom, const char* frag)
{
	if (!hasShader(_name))
		shaderCollection[_name] = new Shaders(vert, geom, frag, false);

	return shaderCollection[_name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::get(std::string _name)
{
	Shaders* out = nullptr;

	if (shaderCollection.find(_name) != shaderCollection.end())
	{
		out = shaderCollection[_name];

	}
	else
	{
		if (std::strcmp(_name.c_str(), "perlin") == 0)
			out = getPerlin();
	}

	return out;
}

//-----------------------------------------------------------------

bool ShaderCollector::hasShader(std::string _name)
{
	if (shaderCollection.size() > 0 && shaderCollection.find(_name) != shaderCollection.end())
		return true;
	else
		return false;
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdClear(bool layered, int nrLayers)
{
	std::string shdr_Header_4_3 = "#version 430 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;
		uniform vec4 clearCol;
		out vec4 i_col;
		void main() {
			i_col = clearCol;
			gl_Position = position;
		});

	//---------------------------------------------------------

	std::string shdr_Header_4_3_g = shdr_Header_4_3 + "layout(triangles, invocations= ";
	shdr_Header_4_3_g += std::to_string(nrLayers);
	shdr_Header_4_3_g += ") in;\nlayout(triangle_strip, max_vertices = 3) out;\n";

	std::string geom = STRINGIFY(
		uniform mat4 m_pvm;
		in vec4 i_col[];
		out vec4 o_col;
		void main()\n
		{\n
			for (int i=0; i<gl_in.length(); i++)\n
			{\n
				gl_Layer = gl_InvocationID;
				o_col = i_col[0];
				gl_Position = m_pvm * gl_in[i].gl_Position;\n
				EmitVertex();\n
			}\n
			EndPrimitive();\n
		});

	std::string frag;

	if (layered)
		frag += "layout (location = 0) out vec4 color; in vec4 o_col;\n  void main() { color = o_col; }";
	else
		frag += "layout (location = 0) out vec4 color; in vec4 i_col;\n void main() { color = i_col; }";

	if (layered)
	{
		vert = "// clear shader, vert\n" + shdr_Header_4_3 + vert;
		geom = "// clear shader, geom\n" + shdr_Header_4_3_g + geom;
		frag = "// clear color shader, frag\n" + shdr_Header_4_3 + frag;

		if (!hasShader("std_clear_layer"))
		{
			shaderCollection["std_clear_layer"] = new Shaders(vert.c_str(), geom.c_str(), frag.c_str(), false);
			shaderCollection["std_clear_layer"]->link();
		}

		return shaderCollection["std_clear_layer"];

	}
	else
	{
		vert = "// clear shader, vert\n" + shdr_Header + vert;
		frag = "// clear color shader, frag\n" + shdr_Header + frag;
		if (!hasShader("std_clear"))
		{
			shaderCollection["std_clear"] = new Shaders(vert.c_str(), frag.c_str(), false);
			shaderCollection["std_clear"]->link();
		}

		return shaderCollection["std_clear"];
	}
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdCol()
{
	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 3 ) in vec4 color;
	uniform mat4 m_pvm;
	out vec4 col;
	void main() {
		col = color;
		gl_Position = m_pvm * position;
	});

	vert = "// basic color shader, vert\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
	in vec4 col;
	layout (location = 0) out vec4 color;
	void main() { color = col; });

	frag = "// basic color shader, frag\n" + shdr_Header + frag;

	if (!hasShader("std_color"))
	{
		shaderCollection["std_color"] = new Shaders(vert.c_str(), frag.c_str(), false);
		shaderCollection["std_color"]->link();
	}

	return shaderCollection["std_color"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdParCol()
{
	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	uniform mat4 m_pvm;
	void main() {
		gl_Position = m_pvm * position;
	});

	vert = "// basic color shader, vert\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
	uniform vec4 color;
	layout (location = 0) out vec4 frag_color;
	void main() { frag_color = color; });

	frag = "// basic color shader, frag\n" + shdr_Header + frag;

	if (!hasShader("std_par_color"))
	{
		shaderCollection["std_par_color"] = new Shaders(vert.c_str(), frag.c_str(), false);
		shaderCollection["std_par_color"]->link();
	}

	return shaderCollection["std_par_color"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdColAlpha()
{
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;
		uniform mat4 m_pvm;
		void main() {
			gl_Position = m_pvm * position;
		});

	vert = "// basic color alpha shader, vert\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
		uniform vec4 col;
		layout (location = 0) out vec4 color;
		void main() {
			color = col;
		});

	frag = "// basic color alpha  shader, frag\n" + shdr_Header + frag;

	if (shaderCollection.find("std_color_alpha") == shaderCollection.end())
	{
		shaderCollection["std_color_alpha"] = new Shaders(vert.c_str(),
				frag.c_str(), false);
		shaderCollection["std_color_alpha"]->link();
	}

	return shaderCollection["std_color_alpha"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdColBorder()
{
	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; uniform mat4 m_pvm; out vec2 tex_coord;

					void main() { tex_coord = texCoord; gl_Position = m_pvm * position; });

	vert = "// basic color shader, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					in vec2 tex_coord; layout (location = 0) out vec4 color; uniform vec2 borderWidth; uniform float borderShape; uniform vec4 borderColor; uniform vec4 col;

					void main() {
					// 0.9999 wegen rundungsfehlern...
					vec2 border = vec2( tex_coord.x < borderWidth.x ? 1.0 : tex_coord.x > (1.0 - borderWidth.x) ? 1.0 : 0.0, tex_coord.y < borderWidth.y ? 1.0 : tex_coord.y > (1.0 - borderWidth.y) ? 1.0 : 0.0);

					color = mix(col, borderColor, max(border.x, border.y));
					//color = vec4( border, 0.0, 1.0);
					});

	frag = "// basic color shader, frag\n" + shdr_Header + frag;

	if (shaderCollection.find("std_color_border") == shaderCollection.end())
	{
		shaderCollection["std_color_border"] = new Shaders(vert.c_str(),
				frag.c_str(), false);
		shaderCollection["std_color_border"]->link();
	}

	return shaderCollection["std_color_border"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdDirLight()
{
	std::string vert =
			STRINGIFY(
#ifndef __EMSCRIPTEN__
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec3 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; out vec4 Color; out vec3 Normal; // interpolate the normalized surface normal
					out vec2 tex_coord;
#else
					precision mediump float;
					attribute vec4 position;
					attribute vec3 normal;
					attribute vec2 texCoord;
					attribute vec4 color;
					varying vec4 Color;
					varying vec3 Normal; // interpolate the normalized surface normal
					varying vec2 tex_coord;
#endif
					uniform mat4 m_pvm; uniform mat3 m_normal; void main () { Color = color;
					// transform the normal, without perspective, and normalize it
					Normal = normalize(m_normal * normal); tex_coord = texCoord; gl_Position = m_pvm * position; });
#ifndef __EMSCRIPTEN__
	vert = "// basic directional light shader, vert\n" + shdr_Header + vert;
#endif

	std::string frag =
			STRINGIFY(
#ifndef __EMSCRIPTEN__
					uniform sampler2D tex_diffuse0; uniform vec4 ambient; uniform vec4 diffuse; uniform vec4 lightColor; uniform vec3 lightDirection; // direction toward the light
					uniform vec3 halfVector;// surface orientation for shiniest spots
					uniform float shininess;// exponent for sharping highlights
					uniform float strength;// extra factor to adjust shininess

					vec4 tex0;

					in vec4 Color; in vec3 Normal;// surface normal, interpolated between vertices
					in vec2 tex_coord; out vec4 gl_FragColor;
#else
					precision mediump float;
					uniform sampler2D tex_diffuse0;
					uniform vec4 ambient;
					uniform vec4 diffuse;
					uniform vec4 lightColor;
					uniform vec3 lightDirection;   // direction toward the light
					uniform vec3 halfVector;// surface orientation for shiniest spots
					uniform float shininess;// exponent for sharping highlights
					uniform float strength;// extra factor to adjust shininess

					varying mediump vec4 Color;
					varying mediump vec3 Normal;// surface normal, interpolated between vertices
					varying mediump vec2 tex_coord;

					vec4 tex0;
#endif
					void main() {
#ifndef __EMSCRIPTEN__
					tex0 = texture(tex_diffuse0, tex_coord);
#else
					tex0 = texture2D(tex_diffuse0, tex_coord);
#endif
					// compute cosine of the directions, using dot products,
					// to see how much light would be reflected
					float diffuseAmt = max(0.0, dot(Normal, lightDirection)); float specular = max(0.0, dot(Normal, halfVector));

					// surfaces facing away from the light (negative dot products)
					// won’t be lit by the directional light
					if (diffuseAmt == 0.0) specular = 0.0; else specular = pow(specular, shininess);// sharpen the highlight

					vec3 scatteredLight = vec3(ambient) + vec3(diffuse) * diffuseAmt; vec3 reflectedLight = vec3(lightColor) * specular * strength;

					// don’t modulate the underlying color with reflected light,
					// only with scattered light
					vec3 rgb = min((Color.rgb + tex0.rgb) * scatteredLight + reflectedLight, vec3(1.0)); gl_FragColor = vec4(rgb * 1.3, Color.a); });

#ifndef __EMSCRIPTEN__
	frag = "// basic directional light shader, frag\n" + shdr_Header + frag;
#endif

	if (shaderCollection.find("std_dir_light") == shaderCollection.end())
	{
		shaderCollection["std_dir_light"] = new Shaders(vert.c_str(),
				frag.c_str(), false);

// for GLES and WEBGL bind the standard attribute locations
#ifdef __EMSCRIPTEN__
		glBindAttribLocation(shaderCollection["std_dir_light"]->getProgram(), 0, "position");
		glBindAttribLocation(shaderCollection["std_dir_light"]->getProgram(), 1, "normal");
		glBindAttribLocation(shaderCollection["std_dir_light"]->getProgram(), 2, "texCoord");
		glBindAttribLocation(shaderCollection["std_dir_light"]->getProgram(), 3, "color");
#endif
		shaderCollection["std_dir_light"]->link();
	}

	return shaderCollection["std_dir_light"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdTex()
{
#ifndef __EMSCRIPTEN__
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec4 normal;\n
		layout( location = 2 ) in vec2 texCoord;\n
		layout( location = 3 ) in vec4 color;\n
		uniform mat4 m_pvm;\n
		out vec2 tex_coord;\n
		void main(){\n
			tex_coord = texCoord;\n
			gl_Position = m_pvm * position;\n
		});

	vert = "// basic texture shader, vert\n" + shdr_Header + vert;



	std::string frag = STRINGIFY(
		uniform sampler2D tex;\n
		in vec2 tex_coord;\n
		layout (location = 0) out vec4 color;\n
		vec4 outCol;
		void main(){\n
			outCol = texture(tex, tex_coord);\n
			if (outCol.a < 0.001){\n
				discard;\n
			} else {\n
				color = outCol;\n
			}\n
		});

	frag = "// basic texture shader, frag\n" + shdr_Header + frag;

#else
	std::string vert = STRINGIFY(attribute vec4 position;
			attribute vec2 texCoord;
			uniform mat4 m_pvm;
			varying vec2 tex_coord;
			void main()
			{
				tex_coord = texCoord;
				gl_Position = m_pvm * position;
			});

	vert = "// basic texture shader, vert\n"+vert;

	std::string frag = STRINGIFY(precision lowp float;
			varying vec2 tex_coord;
			uniform sampler2D tex;
			vec4 outCol;
			void main(){
				outCol = texture2D(tex, tex_coord);
				if (outCol.a < 0.01){
					discard;
				} else {
					gl_FragColor = outCol;
				}
			});

	frag = "// basic texture shader, frag\n"+frag;
#endif

	if (shaderCollection.find("std_tex") == shaderCollection.end())
	{
		shaderCollection["std_tex"] = new Shaders(vert.c_str(), frag.c_str(),
				false);
#ifdef __EMSCRIPTEN__
		glBindAttribLocation(shaderCollection["std_tex"]->getProgram(), 0, "position");
		glBindAttribLocation(shaderCollection["std_tex"]->getProgram(), 1, "normal");
		glBindAttribLocation(shaderCollection["std_tex"]->getProgram(), 2, "texCoord");
		glBindAttribLocation(shaderCollection["std_tex"]->getProgram(), 3, "color");
#endif
		shaderCollection["std_tex"]->link();
	}
	return shaderCollection["std_tex"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdRec()
{
#ifndef __EMSCRIPTEN__
	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; layout (location=4) in vec4 texCorMod; layout (location=10) in mat4 modMatr;

					uniform int useInstancing; uniform int texNr;

					uniform mat4 modelMatrix; uniform mat4 projectionMatrix;

					out vec4 rec_position; out vec3 rec_normal; out vec4 rec_texCoord; out vec4 rec_color;

					mat4 MVM;

					void main(void) { rec_color = color; vec2 tc = useInstancing == 0 ? texCoord : texCoord * vec2(texCorMod.b, texCorMod.a) + vec2(texCorMod.r, texCorMod.g); rec_texCoord = vec4(tc.x, tc.y, float(texNr), 0.0);

					MVM = (useInstancing == 0 ? modelMatrix : modMatr); rec_position = MVM * position; rec_normal = normalize((MVM * vec4(normal, 0.0)).xyz);

					gl_Position = rec_position; });

	vert = "// standard record fragment shader, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout (location = 0) out vec4 color; void main() { color = vec4(1.0); });

	frag = "// standard record fragment shader, frag\n" + shdr_Header + frag;

#endif

	if (shaderCollection.find("std_rec") == shaderCollection.end())
	{
		shaderCollection["std_rec"] = new Shaders(vert.c_str(), frag.c_str(),
				false);
		shaderCollection["std_rec"]->link();
	}
	return shaderCollection["std_rec"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdTexMulti()
{
#ifndef __EMSCRIPTEN__
	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position;\n layout( location = 1 ) in vec4 normal;\n layout( location = 2 ) in vec2 texCoord;\n layout( location = 3 ) in vec4 color;\n uniform mat4 m_pvm;\n out vec2 tex_coord;\n void main(){\n tex_coord = texCoord;\n gl_Position = m_pvm * position;\n });

	vert = "// basic multisample texture shader, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform sampler2DMS tex;\n in vec2 tex_coord;\n uniform vec2 fboSize;\n uniform int nrSamples;\n layout (location = 0) out vec4 color;\n vec4 sampCol; void main(){\n sampCol = vec4(0); for (int i=0;i<nrSamples;i++) sampCol += texelFetch(tex, ivec2(fboSize * tex_coord), i); color = sampCol / float(nrSamples);\n });

	frag = "// basic multisample texture shader, frag\n" + shdr_Header + frag;

#else
	std::string vert = STRINGIFY(attribute vec4 position;
			attribute vec2 texCoord;
			uniform mat4 m_pvm;
			varying vec2 tex_coord;
			void main()
			{
				tex_coord = texCoord;
				gl_Position = m_pvm * position;
			});

	vert = "// basic texture shader, vert\n"+vert;

	std::string frag = STRINGIFY(precision lowp float;
			varying vec2 tex_coord;
			uniform sampler2D tex;
			void main()
			{
				gl_FragColor = texture2D(tex, tex_coord);
			});

	frag = "// basic texture shader, frag\n"+frag;
#endif

	if (!hasShader("std_tex_multi"))
	{
		shaderCollection["std_tex_multi"] = new Shaders(vert.c_str(),
				frag.c_str(), false);
#ifdef __EMSCRIPTEN__
		glBindAttribLocation(shaderCollection["std_tex_multi"]->getProgram(), 0, "position");
		glBindAttribLocation(shaderCollection["std_tex_multi"]->getProgram(), 1, "normal");
		glBindAttribLocation(shaderCollection["std_tex_multi"]->getProgram(), 2, "texCoord");
		glBindAttribLocation(shaderCollection["std_tex_multi"]->getProgram(), 3, "color");
#endif
		shaderCollection["std_tex_multi"]->link();
	}
	return shaderCollection["std_tex_multi"];
}



//-----------------------------------------------------------------

Shaders* ShaderCollector::getStdTexAlpha(bool multiSampTex)
{
#ifndef __EMSCRIPTEN__
	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec4 normal;\n
		layout( location = 2 ) in vec2 texCoord;\n
		layout( location = 3 ) in vec4 color;\n
		uniform mat4 m_pvm;\n
		out vec2 tex_coord;\n

		void main(){\n
			tex_coord = texCoord;\n
			gl_Position = m_pvm * position;\n
		});

	vert = "// basic alpha texture shader, vert\n" + shdr_Header + vert;


	std::string frag = "";

	if (multiSampTex)
	{
		frag += STRINGIFY(
			uniform float alpha;
			uniform sampler2DMS tex;
			uniform int nrSamples;\n
			uniform vec2 scr_size;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outColMS;

			void main(){\n
				for(int i=0;i<nrSamples;i++)
					outColMS += texelFetch(tex, ivec2(scr_size * tex_coord), 0) / float(nrSamples);\n
					color = outColMS;
					color.a = alpha;\n
			});
	}
	else
	{
		frag += STRINGIFY(
			uniform float alpha;
			uniform sampler2D tex;
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n

			void main(){\n
				color = texture(tex, tex_coord);\n
				color.a *= alpha;\n
			});
	}

	frag = "// basic alpha texture shader, frag\n" + shdr_Header + frag;

#else
	std::string vert = STRINGIFY(attribute vec4 position;
			attribute vec2 texCoord;
			uniform mat4 m_pvm;
			varying vec2 tex_coord;
			void main()
			{
				tex_coord = texCoord;
				gl_Position = m_pvm * position;
			});

	vert = "// basic alpha texture shader, vert\n"+vert;

	std::string frag = STRINGIFY(precision lowp float;
			varying vec2 tex_coord;
			uniform sampler2D tex;
			uniform float alpha;

			void main()
			{
				vec4 col = texture2D(tex, tex_coord);
				gl_FragColor = vec4(col.rgb. col.a * alpha);
			});

	frag = "// basic alpha texture shader, frag\n"+frag;
#endif

	std::string name;

	if (multiSampTex)
		name = "std_tex_alpha_ms";
	else
		name = "std_tex_alpha";

	if (!hasShader(name))
	{
		shaderCollection[name] = new Shaders(vert.c_str(), frag.c_str(), false);
#ifdef __EMSCRIPTEN__
		glBindAttribLocation(shaderCollection[name]->getProgram(), 0, "position");
		glBindAttribLocation(shaderCollection[name]->getProgram(), 1, "normal");
		glBindAttribLocation(shaderCollection[name]->getProgram(), 2, "texCoord");
		glBindAttribLocation(shaderCollection[name]->getProgram(), 3, "color");
#endif
		shaderCollection[name]->link();
	}
	return shaderCollection[name];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getEdgeDetect()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; uniform float stepX; uniform float stepY; uniform mat4 m_pvm; out vec2 le; out vec2 ri; out vec2 tex_coord; void main() { le = texCoord + vec2(-stepX, 0.0); ri = texCoord + vec2(stepX, 0.0); tex_coord = texCoord; gl_Position = m_pvm * position; });

	vert = "// Standard edge detect vertex shader\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D tex; in vec2 le; in vec2 ri; in vec2 tex_coord; layout (location = 0) out vec4 color; layout (location = 1) out vec4 shape; bool leftDet; bool rightDet; float outVal; void main() { vec4 center = texture(tex, tex_coord); vec4 left = texture(tex, le); vec4 right = texture(tex, ri);

					leftDet = (left.r == 0.0 && center.r > 0.0) || (left.r > 0.0 && center.r == 0.0); rightDet = (right.r == 0.0 && center.r > 0.0) || (right.r > 0.0 && center.r == 0.0);

					outVal = leftDet || rightDet ? 1.0 : 0.0; outVal = tex_coord.x > 0.015 ? outVal : 0; outVal = tex_coord.x < 0.995 ? outVal : 0;

					color = vec4(outVal); });

	frag = "// Standard Edge detect fragment shader\n" + shdr_Header + frag;

	if (!hasShader("std_edge_detect"))
	{
		shaderCollection["std_edge_detect"] = new Shaders(vert.c_str(),
				frag.c_str(), false);
		shaderCollection["std_edge_detect"]->link();
	}
	return shaderCollection["std_edge_detect"];
}

//-----------------------------------------------------------------
// takes color rgb Values as input (specialized version)

Shaders* ShaderCollector::getStdHeightMapSobel()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout (location=0) in vec4 position; layout (location=1) in vec3 normal; layout (location=2) in vec2 texCoord; layout (location=3) in vec4 color; out vec2 tex_coord; void main(void) { tex_coord = texCoord; gl_Position = position; });
	vert = "//Standard sobel filter: generate normals from height map. Vert\n"
			+ shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					layout(location = 0) out vec4 norm_tex; in vec2 tex_coord; uniform sampler2D heightMap; uniform float heightFact; uniform vec2 texGridStep;

					vec3 posTop; vec3 posBottom; vec3 posCenter; vec3 posLeft; vec3 posRight; vec3 norms[2];

					vec3 pixToHeight(vec3 inPix, vec2 tPos, float heightFact) { vec3 pos = vec3(tPos * 2.0 - vec2(1.0), (inPix.r + inPix.g + inPix.b) * heightFact); return pos; }

					void main() {
					// read neighbour positions left, right, top, bottom
					posTop = pixToHeight(texture(heightMap, vec2(tex_coord.x, tex_coord.y + texGridStep.y) ).xyz, vec2(tex_coord.x, tex_coord.y + texGridStep.y), heightFact); posBottom = pixToHeight(texture(heightMap, vec2(tex_coord.x, tex_coord.y - texGridStep.y)).xyz, vec2(tex_coord.x, tex_coord.y - texGridStep.y), heightFact); posCenter = pixToHeight(texture(heightMap, tex_coord).xyz, tex_coord, heightFact);

					posLeft = pixToHeight(texture(heightMap, vec2(tex_coord.x - texGridStep.x, tex_coord.y)).xyz, vec2(tex_coord.x - texGridStep.x, tex_coord.y), heightFact); posRight = pixToHeight(texture(heightMap, vec2(tex_coord.x + texGridStep.x, tex_coord.y)).xyz, vec2(tex_coord.x + texGridStep.x, tex_coord.y), heightFact);

					norms[0] = normalize(cross((posTop - posCenter), (posLeft - posCenter))); norms[1] = normalize(cross((posBottom - posCenter), (posRight - posCenter)));

					for(int i=0;i<2;i++) norms[i] = norms[i].z > 0.0 ? norms[i] : norms[i] * -1.0;

					norms[0] = normalize((norms[0] + norms[1]) * 0.5); norm_tex = vec4(norms[0], 1.0); });
	frag = "// Standard sobel filter: generate normals from height map frag\n"
			+ shdr_Header + frag;

	if (!hasShader("std_hm_sobel"))
	{
		shaderCollection["std_hm_sobel"] = new Shaders(vert.c_str(),
				frag.c_str(), false);
		shaderCollection["std_hm_sobel"]->link();
	}
	return shaderCollection["std_hm_sobel"];
}

//-----------------------------------------------------------------

Shaders* ShaderCollector::getPerlin()
{
	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 3 ) in vec4 color; uniform mat4 m_pvm; out vec4 col;

					void main() { col = color; gl_Position = m_pvm * position; });

	vert = "// perlin noise, vert\n" + shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform vec2 noiseScale; uniform vec2 noiseScale2; uniform float width; uniform float height; in vec4 col; layout (location = 0) out vec4 color;

					int i; int nrIts = 3; float pi = 3.1415926535897932384626433832795; float newNoise = 0.0;

					vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }

					vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }

					vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }

					vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

					vec3 fade(vec3 t) { return t*t*t*(t*(t*6.0-15.0)+10.0); }

					// Classic Perlin noise
					float cnoise(vec3 P) { vec3 Pi0 = floor(P); // Integer part for indexing
					vec3 Pi1 = Pi0 + vec3(1.0);// Integer part + 1
					Pi0 = mod289(Pi0); Pi1 = mod289(Pi1); vec3 Pf0 = fract(P);// Fractional part for interpolation
					vec3 Pf1 = Pf0 - vec3(1.0);// Fractional part - 1.0
					vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x); vec4 iy = vec4(Pi0.yy, Pi1.yy); vec4 iz0 = Pi0.zzzz; vec4 iz1 = Pi1.zzzz;

					vec4 ixy = permute(permute(ix) + iy); vec4 ixy0 = permute(ixy + iz0); vec4 ixy1 = permute(ixy + iz1);

					vec4 gx0 = ixy0 * (1.0 / 7.0); vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5; gx0 = fract(gx0); vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0); vec4 sz0 = step(gz0, vec4(0.0)); gx0 -= sz0 * (step(0.0, gx0) - 0.5); gy0 -= sz0 * (step(0.0, gy0) - 0.5);

					vec4 gx1 = ixy1 * (1.0 / 7.0); vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5; gx1 = fract(gx1); vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1); vec4 sz1 = step(gz1, vec4(0.0)); gx1 -= sz1 * (step(0.0, gx1) - 0.5); gy1 -= sz1 * (step(0.0, gy1) - 0.5);

					vec3 g000 = vec3(gx0.x,gy0.x,gz0.x); vec3 g100 = vec3(gx0.y,gy0.y,gz0.y); vec3 g010 = vec3(gx0.z,gy0.z,gz0.z); vec3 g110 = vec3(gx0.w,gy0.w,gz0.w); vec3 g001 = vec3(gx1.x,gy1.x,gz1.x); vec3 g101 = vec3(gx1.y,gy1.y,gz1.y); vec3 g011 = vec3(gx1.z,gy1.z,gz1.z); vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

					vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110))); g000 *= norm0.x; g010 *= norm0.y; g100 *= norm0.z; g110 *= norm0.w; vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111))); g001 *= norm1.x; g011 *= norm1.y; g101 *= norm1.z; g111 *= norm1.w;

					float n000 = dot(g000, Pf0); float n100 = dot(g100, vec3(Pf1.x, Pf0.yz)); float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z)); float n110 = dot(g110, vec3(Pf1.xy, Pf0.z)); float n001 = dot(g001, vec3(Pf0.xy, Pf1.z)); float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z)); float n011 = dot(g011, vec3(Pf0.x, Pf1.yz)); float n111 = dot(g111, Pf1);

					vec3 fade_xyz = fade(Pf0); vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z); vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y); float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); return 2.2 * n_xyz; }

					// Classic Perlin noise, periodic variant
					float pnoise(vec3 P, vec3 rep) { vec3 Pi0 = mod(floor(P), rep); // Integer part, modulo period
					vec3 Pi1 = mod(Pi0 + vec3(1.0), rep);// Integer part + 1, mod period
					Pi0 = mod289(Pi0); Pi1 = mod289(Pi1); vec3 Pf0 = fract(P);// Fractional part for interpolation
					vec3 Pf1 = Pf0 - vec3(1.0);// Fractional part - 1.0
					vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x); vec4 iy = vec4(Pi0.yy, Pi1.yy); vec4 iz0 = Pi0.zzzz; vec4 iz1 = Pi1.zzzz;

					vec4 ixy = permute(permute(ix) + iy); vec4 ixy0 = permute(ixy + iz0); vec4 ixy1 = permute(ixy + iz1);

					vec4 gx0 = ixy0 * (1.0 / 7.0); vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5; gx0 = fract(gx0); vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0); vec4 sz0 = step(gz0, vec4(0.0)); gx0 -= sz0 * (step(0.0, gx0) - 0.5); gy0 -= sz0 * (step(0.0, gy0) - 0.5);

					vec4 gx1 = ixy1 * (1.0 / 7.0); vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5; gx1 = fract(gx1); vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1); vec4 sz1 = step(gz1, vec4(0.0)); gx1 -= sz1 * (step(0.0, gx1) - 0.5); gy1 -= sz1 * (step(0.0, gy1) - 0.5);

					vec3 g000 = vec3(gx0.x,gy0.x,gz0.x); vec3 g100 = vec3(gx0.y,gy0.y,gz0.y); vec3 g010 = vec3(gx0.z,gy0.z,gz0.z); vec3 g110 = vec3(gx0.w,gy0.w,gz0.w); vec3 g001 = vec3(gx1.x,gy1.x,gz1.x); vec3 g101 = vec3(gx1.y,gy1.y,gz1.y); vec3 g011 = vec3(gx1.z,gy1.z,gz1.z); vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

					vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110))); g000 *= norm0.x; g010 *= norm0.y; g100 *= norm0.z; g110 *= norm0.w; vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111))); g001 *= norm1.x; g011 *= norm1.y; g101 *= norm1.z; g111 *= norm1.w;

					float n000 = dot(g000, Pf0); float n100 = dot(g100, vec3(Pf1.x, Pf0.yz)); float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z)); float n110 = dot(g110, vec3(Pf1.xy, Pf0.z)); float n001 = dot(g001, vec3(Pf0.xy, Pf1.z)); float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z)); float n011 = dot(g011, vec3(Pf0.x, Pf1.yz)); float n111 = dot(g111, Pf1);

					vec3 fade_xyz = fade(Pf0); vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z); vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y); float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); return 2.2 * n_xyz; }

					void main() { vec2 posCo = vec2(gl_FragCoord) / vec2(width, height) * noiseScale; float noiseC = 0.0;

					for (i=0;i<nrIts;i++) { newNoise = (cnoise( vec3(posCo *pow(2.0, float(i+2)) *32.0, 0.0) ) +1.0) *0.25; noiseC += newNoise / pow(2.0, float(i)); }

					noiseC = pow(noiseC, 4.0) * 4.0; color = vec4(noiseC, noiseC, noiseC, 1.0); });

	frag = "// perlin noise shader, frag\n" + shdr_Header + frag;

	if (!hasShader("perlin"))
	{
		shaderCollection["perlin"] = new Shaders(vert.c_str(), frag.c_str(),
				false);
		shaderCollection["perlin"]->link();
	}
	return shaderCollection["perlin"];
}

//-----------------------------------------------------------------

std::string ShaderCollector::getShaderHeader(){
	return shdr_Header;
}

}
