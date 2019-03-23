cmake_minimum_required (VERSION 2.6)
project (tav_sn_testing)

#include(cotire)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE Release)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

# the RPATH to be used when installing
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# don't add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# The version number.
set (Tutorial_VERSION_MAJOR 0)
set (Tutorial_VERSION_MINOR 1)

option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_CUDA "Use Cuda" ON)
option (WITH_FLEX "Use Nvidia Flex Physical Simulation" OFF)
option (WITH_OPENNI2 "Use OpenNI2" ON)
option (WITH_NITE2 "Use Nite2" ON)
option (WITH_AUDIO "Build Tav Audio Module" ON)
option (WITH_TRACKING "Build Tav Tracking Module" ON)

if(WITH_AUDIO)
	add_definitions(-DWITH_AUDIO)
endif(WITH_AUDIO)
	

#find_package (Boost 1.55 REQUIRED thread)
if(WITH_CUDA)
	if (WITH_FLEX)
		find_package (Flex)
	endif(WITH_FLEX)
  find_package (CUDA)
endif(WITH_CUDA)
find_package (Freetype REQUIRED)
find_package (HarfBuzz REQUIRED)
if(WITH_OPENNI2)
  find_package (OpenNI2 REQUIRED)
endif(WITH_OPENNI2)
if(WITH_NITE2)
  find_package (NITE2 REQUIRED)
endif(WITH_NITE2)

if (WITH_CUDA AND FLEX_FOUND)
  add_definitions(-DHAVE_FLEX)
endif(WITH_CUDA AND FLEX_FOUND)


# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src/headers")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_tracking/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_video/src")


# Source Files
file(GLOB tav_sn_testing_flex_SRC "")
file(GLOB tav_sn_testing_SRC "src/*.cpp")

if (WITH_CUDA)
  file(GLOB tav_sn_testing_flex_SRC "src/flex/*.cpp")
endif (WITH_CUDA)

if(WITH_OPENNI2)
  file(GLOB tav_sn_testing_kinect_SRC "src/kinect/*.cpp")
endif(WITH_OPENNI2)

file(GLOB tav_sn_testing_particle_SRC "src/particle/*.cpp")
file(GLOB tav_sn_testing_typo_SRC "src/typo/*.cpp")

if(WITH_VIDEO)
	file(GLOB tav_sn_testing_video_SRC "src/video/*.cpp")
endif(WITH_VIDEO)

set(TAV_SN_TESTING_ALL_SRC ${tav_sn_testing_SRC} ${tav_sn_testing_kinect_SRC} ${tav_sn_testing_flex_SRC} ${tav_sn_testing_particle_SRC} ${tav_sn_testing_typo_SRC} ${tav_sn_testing_video_SRC})




if(BUILD_STATIC)
  add_library(tav_sn_testing STATIC ${TAV_SN_TESTING_ALL_SRC}) 
else()
  add_library(tav_sn_testing SHARED ${TAV_SN_TESTING_ALL_SRC}) 
endif(BUILD_STATIC)

#cotire(tav_sn_testing)


# Cuda
if (WITH_CUDA AND CUDA_FOUND)
  add_definitions(-DHAVE_CUDA)
  include_directories(${CUDA_INCLUDE_DIRS})
  target_link_libraries (tav_sn_testing ${CUDA_LIBRARIES})
endif (WITH_CUDA AND CUDA_FOUND)

if (WITH_CUDA AND FLEX_FOUND)
  add_definitions(-DHAVE_FLEX)
  include_directories(${FLEX_INCLUDE_DIRS})
  target_link_libraries (tav_sn_testing ${FLEX_LIBRARIES})
endif (WITH_CUDA AND FLEX_FOUND)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_sn_testing ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

# HarfBuzz
if (HARFBUZZ_FOUND)
  include_directories(${HARFBUZZ_INCLUDE_DIRS})
  target_link_libraries (tav_sn_testing ${HARFBUZZ_LIBRARIES})
endif (HARFBUZZ_FOUND)

# NITE2
if (WITH_NITE2 AND NITE2_FOUND)
  add_definitions(-DHAVE_NITE2)
  include_directories(${NITE2_INCLUDE_DIRS})
  target_link_libraries(tav_sn_testing ${NITE2_LIBS})
endif (WITH_NITE2 AND NITE2_FOUND)

# OpenNI2
if (WITH_OPENNI2 AND OPENNI2_FOUND)
  add_definitions(-DHAVE_OPENNI2)
  include_directories(${OPENNI2_INCLUDE_DIRS})
  target_link_libraries(tav_sn_testing ${OPENNI2_LIBS})
endif (WITH_OPENNI2 AND OPENNI2_FOUND)


target_link_libraries(tav_sn_testing ${LIBS})


# tell GLFW to not include gl headers
add_definitions(-DGLFW_INCLUDE_NONE)


add_dependencies(tav_sn_testing tav_core)
if (WITH_VIDEO)
	add_dependencies(tav_sn_testing tav_video)
endif(WITH_VIDEO)
if (WITH_TRACKING)
	add_dependencies(tav_sn_testing tav_tracking)
endif(WITH_TRACKING)

# add the install targets
install (TARGETS tav_sn_testing DESTINATION "lib")

file( GLOB _HeadersTesting "src/*.h" )
install(FILES ${_HeadersTesting} DESTINATION "include/tav/tav_sn_testing")
