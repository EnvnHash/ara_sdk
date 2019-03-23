# Copyright 2015, Sven Hahne
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#  FLEX_FOUND - system has Flex
#  FLEX_INCLUDE_DIRS - the Flex include directory
#  FLEX_LIBRARIES

SET(FLEX_FOUND FALSE)
SET(FLEX_INCLUDE_DIRS)
SET(FLEX_LIBRARIES)

find_path(FLEX_INCLUDE_DIRS NAMES NvFlex.h
	HINTS
	/usr/local/include/flex/
	/usr/include/flex
	/usr/local/include/
	/usr/include/
	}
)
 
find_library(FLEX_CUDA NAMES NvFlexReleaseCUDA_x64.a)
find_library(FLEX_DEVICE NAMES NvFlexDeviceRelease_x64.a)
find_library(FLEX_EXT NAMES NvFlexExtRelease_x64.a)

IF(FLEX_CUDA AND FLEX_DEVICE AND FLEX_EXT)
  SET(FLEX_LIBRARIES ${FLEX_CUDA} ${FLEX_DEVICE} ${FLEX_EXT})
ENDIF()

if(FLEX_INCLUDE_DIRS AND FLEX_LIBRARIES)
  set(FLEX_FOUND TRUE)
endif()

if (FLEX_FOUND)
  mark_as_advanced(FLEX_INCLUDE_DIRS FLEX_LIBRARIES)
endif()
