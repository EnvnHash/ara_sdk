/*
 *  math_utils.cpp
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 09.09.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 */

#include "pch.h"
#include "string_utils.h"

namespace tav
{
std::string ReplaceString(std::string subject, const std::string& search,
		const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

//--------------------------------------------------------------------

void ReplaceStringInPlace(std::string& subject, const std::string& search,
		const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

//--------------------------------------------------------------------

std::vector<std::string> splitByNewline(const std::string &s)
{
	std::vector<std::string> elems;
	std::istringstream ss;
	ss.str(s);
	for (std::string line; std::getline(ss, line);)
	{
		elems.push_back(line);
	}
	return elems;
}

//--------------------------------------------------------------------

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}

//--------------------------------------------------------------------

std::vector<std::string> split(const std::string &s, std::string delim)
{
	std::string strCpy = s;
	std::vector<std::string> elems;
	size_t fPos = strCpy.find(delim);

	while (fPos != std::string::npos)
	{
		elems.push_back(strCpy.substr(0, fPos));
		strCpy = strCpy.substr(fPos + std::strlen(delim.c_str()),
				std::strlen(strCpy.c_str()) - 1);
		fPos = strCpy.find(delim);
	}

	elems.push_back(strCpy);

	return elems;
}
}
