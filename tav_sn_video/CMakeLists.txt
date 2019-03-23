cmake_minimum_required (VERSION 2.6)
project (tav_sn_video)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE Release)
set(OpenGL_GL_PREFERENCE "LEGACY")

#include(cotire)

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

# supress deprecated warnings
SET(GXX_COVERAGE_COMPILE_FLAGS "-Wno-deprecated -Wno-deprecated-declarations")
add_definitions(${GXX_COVERAGE_COMPILE_FLAGS})
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

#Option
option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_OPENCV "Use OpenCV" ON)
option (WITH_AUDIO "Build Tav Audio Module" ON)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_video/src")

file(GLOB tav_sn_video_SRC "src/*.cpp")

if (NOT WITH_AUDIO)
  list(REMOVE_ITEM tav_sn_video_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/SNAudioRingV.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNVideoTileAudio3d.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNVideoTileAudio.cpp")
endif (NOT WITH_AUDIO)

if (NOT WITH_TRACKING)
 list(REMOVE_ITEM tav_sn_video_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/SNIkpiMallPlaza.cpp")
endif (NOT WITH_TRACKING)


if(BUILD_STATIC)
  add_library(tav_sn_video STATIC ${tav_sn_video_SRC}) 
else()
  add_library(tav_sn_video SHARED ${tav_sn_video_SRC}) 
endif(BUILD_STATIC)

#cotire(tav_sn_video)


find_package (Boost 1.55 REQUIRED thread)
find_package (GLEW REQUIRED)
find_package (OpenGL REQUIRED)
if(WITH_OPENCV)
  find_package (OpenCV REQUIRED)
endif(WITH_OPENCV)
find_package (FFmpeg REQUIRED)
find_package (Freetype REQUIRED)


# OpenCV
if (WITH_OPENCV AND OPENCV_FOUND)
  add_definitions(-DHAVE_OPENCV)
  include_directories(${OPENCV_INCLUDE_DIRS})
  target_link_libraries(tav_sn_video ${OpenCV_LIBS})
  target_link_libraries(tav_sn_video opencv_dnn)
endif (WITH_OPENCV AND OPENCV_FOUND)

if (NOT WITH_OPENCV)
  remove_definitions(-DHAVE_OPENCV)  
endif (NOT WITH_OPENCV)


# Boost
if (BOOST_FOUND)
  include_directories(${BOOST_INCLUDE_DIRS})
  target_link_libraries (tav_sn_video ${BOOST_LIBRARIES})
endif (BOOST_FOUND)

if (FFMPEG_FOUND)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  target_link_libraries(tav_sn_video ${FFMPEG_LIBRARIES})
endif (FFMPEG_FOUND)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_sn_video ${FREETYPE_LIBRARIES} ${LIB_LINK_OPT})
endif (FREETYPE_FOUND)

# GLEW
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIRS})
  target_link_libraries (tav_sn_video ${GLEW_LIBRARIES})
endif (GLEW_FOUND)

# OpenGL
if (OpenGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries (tav_sn_video ${OPENGL_LIBRARIES})
endif (OpenGL_FOUND)

add_dependencies(tav_sn_video tav_core)
add_dependencies(tav_sn_video tav_video)


# add the install targets
install (TARGETS tav_sn_video DESTINATION "lib")

file( GLOB _VideoHeaders "src/*.h" )


#if (NOT WITH_TRACKING)
# list(REMOVE_ITEM _VideoHeaders "${CMAKE_CURRENT_SOURCE_DIR}/src/SNIkpiMallPlaza.h")
#endif (NOT WITH_TRACKING)

install(FILES ${_VideoHeaders} DESTINATION "include/tav/tav_sn_video")
