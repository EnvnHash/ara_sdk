//
//  tav_types.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 25.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "headers/tav_types.h"

namespace tav
{
//                                          loc:        0         1           2        3            4           5      6       7       8       9         10-13
std::vector<std::string> stdAttribNames =
{ "position", "normal", "texCoord", "color", "texCorMod", "velocity", "aux0",
		"aux1", "aux2", "aux3", "modMatr" };
std::vector<int> coTypeStdSize =
{ 3, 3, 2, 4, 4, 4, 4, 4, 4, 4, 16 };
std::vector<int> coTypeFragSize =
{ 4, 3, 2, 4, 4, 4, 4, 4, 4, 4, 16 };

// muss mit coordtype konsistent sein
//                                          loc:               0             1               2            3                4               5           6
std::vector<std::string> stdRecAttribNames =
{ "rec_position", "rec_normal", "rec_texCoord", "rec_color", "rec_texCorMod",
		"rec_velocity", "rec_aux0",
		//                                          loc:           7            8           9              10
		"rec_aux1", "rec_aux2", "rec_aux3", "rec_modMatr" };
std::vector<int> recCoTypeFragSize =
{ 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 16 };

std::vector<std::string> stdMatrixNames =
{ "modelMatrix", "viewMatrix", "projectionMatrix", "normalMatrix" };
}
