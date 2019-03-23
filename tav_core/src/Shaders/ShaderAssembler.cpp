//
//  ShaderAssembler.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "ShaderAssembler.h"

namespace tav
{
ShaderAssembler::ShaderAssembler()
{
	// create the std::strings for the resulting code
	resShader = new std::string[SHTP_COUNT];
	descr = new shaderDescr[SHTP_COUNT];

	// add standard main functions
	for (int i = 0; i < SHTP_COUNT; i++)
	{
		wD = &descr[i];
		wD->version = "#version 410\n";
		wD->type = (shaderType) i;
	}
	wD = &descr[VERTEX];
	lastShdrType = VERTEX;
}

ShaderAssembler::~ShaderAssembler()
{
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::setWorkShader(shaderType _type)
{
	lastShdrType = wD->type;
	wD = &descr[_type];
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::setVersion(std::string& _in)
{
	wD->version = _in;
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::addLayoutIn(std::string _arg, std::string _type,
		std::string _name)
{
	wD->layoutInputs.push_back(new layoutInput());
	wD->layoutInputs.back()->arg = _arg;
	wD->layoutInputs.back()->type = _type;
	wD->layoutInputs.back()->name = _name;
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::addLayoutOut(std::string _arg, std::string _type,
		std::string _name)
{
	wD->layoutOutputs.push_back(new layoutOutput());
	wD->layoutOutputs.back()->arg = _arg;
	wD->layoutOutputs.back()->type = _type;
	wD->layoutOutputs.back()->name = _name;
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::addUniform(std::string _type, std::string _name)
{
	wD->uniformInputs.push_back(new uniformInput());
	wD->uniformInputs.back()->name = _name;
	wD->uniformInputs.back()->type = _type;
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::addCode(std::string _code)
{
	wD->code = _code;
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::setEnableInstancing()
{
	wD->uniformInputs.push_back(new uniformInput());
	wD->uniformInputs.back()->name = "useInstancing";
	wD->uniformInputs.back()->type = "int";
}

void ShaderAssembler::setInstModMatr()
{
	addLayoutIn("location=" + std::to_string(TEXCORMOD), "vec4",
			stdAttribNames[TEXCORMOD]);
	addLayoutIn("location=" + std::to_string(MODMATR), "mat4",
			stdAttribNames[MODMATR]);
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::setVertStdInput()
{
	wD = &descr[VERTEX];

	addLayoutIn("location=" + std::to_string(POSITION), "vec4",
			stdAttribNames[POSITION]);
	addLayoutIn("location=" + std::to_string(NORMAL), "vec3",
			stdAttribNames[NORMAL]);
	addLayoutIn("location=" + std::to_string(TEXCOORD), "vec4",
			stdAttribNames[TEXCOORD]);
	addLayoutIn("location=" + std::to_string(COLOR), "vec4",
			stdAttribNames[COLOR]);
}

void ShaderAssembler::setGeomStdInput()
{
}

void ShaderAssembler::setFragStdInput()
{
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::setUniformSeparateMVP()
{
	for (std::vector<std::string>::iterator it = stdMatrixNames.begin();
			it != stdMatrixNames.end(); ++it)
	{
		wD->uniformInputs.push_back(new uniformInput());
		wD->uniformInputs.back()->name = (*it);
		if ((*it).compare("normalMatrix") == 0)
			wD->uniformInputs.back()->type = "mat3";
		else
			wD->uniformInputs.back()->type = "mat4";
	}
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::asmblMultiCamGeo(int nrCams)
{
	// delete the $S_projectionMatrix from the vertex Shader;
	setWorkShader(VERTEX);
	removeStdProj();

	setWorkShader(GEOMETRY);

	addLayoutIn("triangles, invocations=" + std::to_string(nrCams), "", "");
	addLayoutOut("triangle_strip, max_vertices=3", "", "");
	addUniform("mat4", "model_matrix_g[" + std::to_string(nrCams) + "]");
	addUniform("mat4", "view_matrix_g[" + std::to_string(nrCams) + "]");
	addUniform("mat4", "projection_matrix_g[" + std::to_string(nrCams) + "]");
	addUniform("mat3", "normal_matrix_g[" + std::to_string(nrCams) + "]");

	// get outputs vertex shader and write them into the outputs
	getOutputsFromVert();

	// set outputs from vertex outputs
	for (std::vector<varType*>::iterator it = wD->outputs.begin();
			it != wD->outputs.end(); ++it)
		wD->code += "out " + (*it)->type + " " + (*it)->name + ";\n";

	wD->code +=
//            "const vec4 $G_colors[4] = vec4[4] (vec4(1.0, 0.7, 0.3, 1.0), vec4(1.0, 0.2, 0.3, 1.0), vec4(0.1, 0.6, 1.0, 1.0), vec4(0.3, 0.7, 0.5, 1.0));\n"
			"void main() {\n"
					"for (int i=0; i<gl_in.length(); i++)\n"
					"{\n"
// Set the viewport index for every vertex.
					"gl_ViewportIndex = gl_InvocationID;\n";

	for (std::vector<varType*>::iterator it = wD->outputs.begin();
			it != wD->outputs.end(); ++it)
	{
		std::string corIn = (*it)->name;
		boost::replace_all(corIn, "$O_", "$I_");

		if (!(*it)->name.compare("$O_Normal"))
		{
			// Normal is transformed using the model matrix.
			// Note that this assumes that there is no
			// shearing in the model matrix.
			wD->code +=
					"$O_Normal = (model_matrix_g[gl_InvocationID] * vec4($I_Normal[i], 0.0)).xyz;\n";

		}
		else if (!(*it)->name.compare("$O_Color"))
		{
			wD->code += (*it)->name + " = " + corIn + "[i];\n";
//                wD->code += "$O_Color = $G_colors[gl_InvocationID];";
		}
		else
		{
			wD->code += (*it)->name + " = " + corIn + "[i];\n";
		}
	}

	// Finally, transform the vertex into position and emit it.
	wD->code +=
			"gl_Position = projection_matrix_g[gl_InvocationID] * view_matrix_g[gl_InvocationID] * (model_matrix_g[gl_InvocationID] * gl_in[i].gl_Position);\n"
			// momentan wird nichts mit der model matritze gemacht, deshalb auskommentiert...
//2        wD->code += "gl_Position = projection_matrix_g[gl_InvocationID] * view_matrix_g[gl_InvocationID] * gl_in[i].gl_Position;\n"
					"EmitVertex();"
					"}"
					"}";
}

//------------------------------------------------------------------------------------------------

// remove std projetion matrix uses from vertex shader
void ShaderAssembler::removeStdProj()
{
	boost::replace_all(wD->code, "$S_projectionMatrix *", "");
	boost::replace_all(wD->code, "$S_projectionMatrix*", "");

	std::vector<uniformInput*>::iterator toKill;
	for (std::vector<uniformInput*>::iterator it = wD->uniformInputs.begin();
			it != wD->uniformInputs.end(); ++it)
		if ((*it)->name.compare("projectionMatrix") == 0)
			toKill = it;

	wD->uniformInputs.erase(toKill);

	boost::replace_all(wD->code, "$S_viewMatrix *", "");
	boost::replace_all(wD->code, "$S_viewMatrix*", "");
	boost::replace_all(wD->code, "* $S_viewMatrix", "");
	boost::replace_all(wD->code, "*$S_viewMatrix", "");

	for (std::vector<uniformInput*>::iterator it = wD->uniformInputs.begin();
			it != wD->uniformInputs.end(); ++it)
		if ((*it)->name.compare("viewMatrix") == 0)
			toKill = it;

	wD->uniformInputs.erase(toKill);

	boost::replace_all(wD->code, "$S_normalMatrix *", "");
	boost::replace_all(wD->code, "$S_normalMatrix*", "");
	boost::replace_all(wD->code, "* $S_normalMatrix", "");
	boost::replace_all(wD->code, "*$S_normalMatrix", "");

	for (std::vector<uniformInput*>::iterator it = wD->uniformInputs.begin();
			it != wD->uniformInputs.end(); ++it)
		if ((*it)->name.compare("normalMatrix") == 0)
			toKill = it;

	wD->uniformInputs.erase(toKill);
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::assemble(shaderType _shdrType, bool assembleRaw)
{
	setWorkShader(_shdrType);

	shdr.str(std::string());    // clear shdr
	shdr << wD->version;        // set version
	std::string inPrfx, outPrfx;

	if (!assembleRaw)
	{
		switch (wD->type)
		{
		case VERTEX:
			outPrfx = "vs_";
			break;
		case GEOMETRY:
			outPrfx = "gs_";
			break;
		case FRAGMENT:
			outPrfx = "fs_";
			break;
		default:
			outPrfx = "vs_";
			break;
		}

		switch (lastShdrType)
		{
		case VERTEX:
			inPrfx = "vs_";
			break;
		case GEOMETRY:
			inPrfx = "gs_";
			break;
		default:
			inPrfx = "vs_";
			break;
		}
	}

	// set layout inputs
	for (std::vector<layoutInput*>::iterator it = wD->layoutInputs.begin();
			it != wD->layoutInputs.end(); ++it)
		shdr << "layout(" << (*it)->arg << ") in " << (*it)->type << " "
				<< (*it)->name << ";\n";

	// set layout outputs
	for (std::vector<layoutOutput*>::iterator it = wD->layoutOutputs.begin();
			it != wD->layoutOutputs.end(); ++it)
		shdr << "layout(" << (*it)->arg << ") out " << (*it)->type << " "
				<< (*it)->name << ";\n";

	// set uniform inputs
	for (std::vector<uniformInput*>::iterator it = wD->uniformInputs.begin();
			it != wD->uniformInputs.end(); ++it)
		shdr << "uniform " << (*it)->type << " " << (*it)->name << ";\n";

	// get inputs from previous shader stage
	if (!assembleRaw)
		if (wD->type > VERTEX)
			setInputs(descr[lastShdrType].code, inPrfx, shdr);

	// parse code, replace symbols
	std::string pCode = wD->code;

	// replace prefixes from output and global variables
	if (!assembleRaw)
	{
		boost::replace_all(pCode, "$O_", outPrfx);
		boost::replace_all(pCode, "$I_", inPrfx);
	}

	boost::replace_all(pCode, "$G_", "");
	boost::replace_all(pCode, "$S_", "");

	shdr << pCode;

	// write the result into a std::string
	resShader[_shdrType] = shdr.str();
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::setInputs(std::string& srcString, std::string inPrfx,
		std::stringstream& dst)
{
	boost::smatch m;
	boost::regex e("\\b(out)([^;]*)");   // matches words beginning by "out"
	std::string s = srcString;               // make a copy of std::string
	std::vector<std::string> outputs;

	// collect all outputs from the previous shader stage
	while (boost::regex_search(s, m, e))
	{
		std::string found = *(m.begin());
		boost::replace_all(found, "$O_", inPrfx);
		boost::replace_all(found, "out", "in");
		outputs.push_back(found);

		s = m.suffix().str();
	}

	// add them to the current stage
	for (std::vector<std::string>::iterator it = outputs.begin();
			it != outputs.end(); ++it)
	{
		dst << (*it);
		if (wD->type == GEOMETRY)
			dst << "[]";
		dst << ";";
	}
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::getOutputsFromVert()
{
	std::string s = descr[lastShdrType].code;    // make a copy of std::string
	boost::regex e("out([^;]*)");

	boost::sregex_token_iterator iter(s.begin(), s.end(), e, 0);
	boost::sregex_token_iterator end;

	for (; iter != end; ++iter)
	{
		std::vector<std::string> strs;
		std::string foundS = static_cast<std::string>(*iter);
		boost::split(strs, foundS, boost::is_any_of(" "));

		wD->outputs.push_back(new varType());
		wD->outputs.back()->dir = strs[0];
		wD->outputs.back()->type = strs[1];
		wD->outputs.back()->name = strs[2];
	}
}

//------------------------------------------------------------------------------------------------

// add one line defining the edge blend value in relation to the screencoords
// and multiply the standard fragment color output by it
void ShaderAssembler::addEdgeBlendCodeHead2Head()
{
	setWorkShader(FRAGMENT);

	std::string s = wD->code;           // make a copy of the actual source code
	boost::regex e("(\\$S_FragColor)([^;]*)");
	std::vector<std::string> outputs;
	std::string found;

	boost::sregex_token_iterator iter(s.begin(), s.end(), e, 0);
	boost::sregex_token_iterator end;

	for (; iter != end; ++iter)
	{
		found = static_cast<std::string>(*iter);
	}

	boost::replace_all(wD->code, found,
			"float edgeBlend = min((1.0 - gl_FragCoord.y / scrHeight) / overlapV, 1.0);"
					+ found + "*edgeBlend");
}

//------------------------------------------------------------------------------------------------

// add one line defining the edge blend value in relation to the screencoords
// and multiply the standard fragment color output by it
void ShaderAssembler::addEdgeBlendCode2()
{
	setWorkShader(FRAGMENT);

	std::string s = wD->code;           // make a copy of the actual source code
	boost::regex e("(\\$S_FragColor)([^;]*)"); // matches words up to the next ;
	std::vector<std::string> outputs;
	std::string found;

	boost::sregex_token_iterator iter(s.begin(), s.end(), e, 0);
	boost::sregex_token_iterator end;

	// collect all outputs from the previous shader stage
	for (; iter != end; ++iter)
	{
		found = static_cast<std::string>(*iter);
	}

	boost::replace_all(wD->code, found,
			"float edgeBlend = min(abs((gl_FragCoord.x / scrWidth) - 0.5) * overlapV, 1.0);"
					+ found + "*edgeBlend");
}

//------------------------------------------------------------------------------------------------

// add one line defining the edge blend value in relation to the screencoords
// and multiply the standard fragment color output by it
void ShaderAssembler::addEdgeBlendCode3()
{
	setWorkShader(FRAGMENT);

	std::string s = wD->code;           // make a copy of the actual source code
	boost::regex e("(\\$S_FragColor)([^;]*)"); // matches words up to the next ;
	std::vector<std::string> outputs;
	std::string found;

	boost::sregex_token_iterator iter(s.begin(), s.end(), e, 0);
	boost::sregex_token_iterator end;

	// collect all outputs from the previous shader stage
	for (; iter != end; ++iter)
	{
		found = static_cast<std::string>(*iter);
	}

	boost::replace_all(wD->code, found,
			"float norX = (gl_FragCoord.x / scrWidth) * 2.0; float edgeBlend = min((abs(1.333-norX*2.0) - max(norX-1.0,0.0)*4.0 + max(norX-1.3333, 0)*4.0) * overlapV, 1.0);"
					+ found + "*edgeBlend");
}

//------------------------------------------------------------------------------------------------

// add one line defining the edge blend value in relation to the screencoords
// and multiply the standard fragment color output by it
void ShaderAssembler::addEdgeBlendCodeN(int nrCams)
{
	setWorkShader(FRAGMENT);

	std::string s = wD->code;           // make a copy of the actual source code
	boost::regex e("(\\$S_FragColor)([^;]*)"); // matches words up to the next, sind die beiden \\ richtig? ;
	std::vector<std::string> outputs;
	std::string found;

	boost::sregex_token_iterator iter(s.begin(), s.end(), e, 0);
	boost::sregex_token_iterator end;

	// collect all outputs from the previous shader stage
	for (; iter != end; ++iter)
	{
		found = static_cast<std::string>(*iter);
	}

	std::string addCode =
			"float norX = (gl_FragCoord.x / scrWidth);\nfloat edgeBlend = ";
	for (auto i = 0; i < (nrCams - 1); i++)
	{
		float xPos = static_cast<float>(i + 1)
				/ static_cast<float>(std::max(nrCams, 1));
		addCode += "min(abs((norX - " + std::to_string(xPos) + ") * overlapV"
				+ std::to_string(i) + "), 1.0)";
		if (i != nrCams - 2)
			addCode += " * ";
	}
	//  addCode += ";\n";
	addCode += ";\n" + found + " * edgeBlend";

	boost::replace_all(wD->code, found, addCode);
}

//------------------------------------------------------------------------------------------------

const char* ShaderAssembler::getVertShader()
{
	return resShader[VERTEX].c_str();
}

const char* ShaderAssembler::getFragShader()
{
	return resShader[FRAGMENT].c_str();
}

const char* ShaderAssembler::getGeomShader()
{
	return resShader[GEOMETRY].c_str();
}

//------------------------------------------------------------------------------------------------

void ShaderAssembler::print()
{
	std::cout << "assembled vertex Shader: " << shdr.str() << std::endl;
}

/*
 //------------------------------------------------------------------------------------------------

 std::string ShaderAssembler::parseCode(boost::regex_constants::error_type etype) {
 switch (etype) {
 case boost::regex_constants::error_collate:
 return "error_collate: invalid collating element request";
 case boost::regex_constants::error_ctype:
 return "error_ctype: invalid character class";
 case boost::regex_constants::error_escape:
 return "error_escape: invalid escape character or trailing escape";
 case boost::regex_constants::error_backref:
 return "error_backref: invalid back reference";
 case boost::regex_constants::error_brack:
 return "error_brack: mismatched bracket([ or ])";
 case boost::regex_constants::error_paren:
 return "error_paren: mismatched parentheses(( or ))";
 case boost::regex_constants::error_brace:
 return "error_brace: mismatched brace({ or })";
 case boost::regex_constants::error_badbrace:
 return "error_badbrace: invalid range inside a { }";
 case boost::regex_constants::error_range:
 return "erro_range: invalid character range(e.g., [z-a])";
 case boost::regex_constants::error_space:
 return "error_space: insufficient memory to handle this regular expression";
 case boost::regex_constants::error_badrepeat:
 return "error_badrepeat: a repetition character (*, ?, +, or {) was not preceded by a valid regular expression";
 case boost::regex_constants::error_complexity:
 return "error_complexity: the requested match is too complex";
 case boost::regex_constants::error_stack:
 return "error_stack: insufficient memory to evaluate a match";
 default:
 return "";
 }
 }
 */
}
