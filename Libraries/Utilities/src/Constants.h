//
// Created by hahne on 01.07.2025.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

//
// Created by user on 5/16/25.
//

#pragma once


#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647693
#endif

#ifndef M_TWO_PI
#define M_TWO_PI 6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI 1.57079632679489661923
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI / 180.0)
#endif

#ifndef ONE_THIRD
#define ONE_THIRD (1.0f / 3.0f)
#endif

#ifndef FOUR_THIRDS
#define FOUR_THIRDS (4.0f / 3.0f)
#endif

#ifndef SEL_LAYER_COUNT
#define SEL_LAYER_COUNT 2
#endif

#ifndef _WIN32
#ifndef MAX_PATH
#define MAX_PATH 256
#endif
#else
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#endif

#define GLSG_DONT_CARE (-1)