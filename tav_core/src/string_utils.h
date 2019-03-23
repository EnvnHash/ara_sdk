/*
 *  math_utils.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 09.09.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>

namespace tav
{
std::string ReplaceString(std::string subject, const std::string& search,
		const std::string& replace);
void ReplaceStringInPlace(std::string& subject, const std::string& search,
		const std::string& replace);
std::vector<std::string> splitByNewline(const std::string &s);
std::vector<std::string> split(const std::string &s, std::string delim);
std::vector<std::string> split(const std::string &s, char delim);
}
