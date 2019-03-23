cmake_minimum_required (VERSION 2.6)
project (tav_video)

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

# cotire doesn't user ffmpeg precompiled header if this flag isn't set 
add_definitions(-D__DEPRECATED)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

#Options
option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_AUDIO "Build Tav Audio Module" ON)
option (WITH_CUDA "Use Cuda" ON)
option (WITH_GSTREAMER "Build with GStreamer" OFF)
option (WITH_NVVIDEO "Build with NVIDIA Video SDK Decoder" OFF)
option (WITH_OPENCV "Use OpenCV" OFF)
option (WITH_ACTRANGE_TRACKING "Use the ActiveRange Video Tracking Module" ON)


if (WITH_AUDIO)
  add_definitions(-DWITH_AUDIO)
endif (WITH_AUDIO)

if(BUILD_STATIC)
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")
endif(BUILD_STATIC)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_tracking/src")

if (WITH_NVVIDEO)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/nvidiaDecoder")
endif (WITH_NVVIDEO)


file(GLOB tav_video_SRC "src/*.cpp")
file(GLOB tav_video_nvvideo_SRC "")

if (NOT WITH_ACTRANGE_TRACKING)
  list(REMOVE_ITEM tav_video_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoTextureCvActRange.cpp")
endif (NOT WITH_ACTRANGE_TRACKING)

if (WITH_NVVIDEO)
  file(GLOB tav_video_nvvideo_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/nvidiaDecoder/*.cpp")
endif (WITH_NVVIDEO)

if (NOT WITH_GSTREAMER)
  list(REMOVE_ITEM tav_video_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoTextureGst.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoTextureGStreamer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoTextureGstUtils.cpp")
endif (NOT WITH_GSTREAMER)

 list(REMOVE_ITEM tav_video_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoTextureGst.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoTextureGstUtils.cpp")

if (WITH_TRACKING)
  add_definitions(-DHAVE_TRACKING)
endif (WITH_TRACKING)


if (UNIX)
  file(GLOB tav_video_v4l_SRC "src/V4L/*.cpp")
  set(TAV_VIDEO_ALL_SRCS ${tav_video_SRC} ${tav_video_nvvideo_SRC} ${tav_video_v4l_SRC})
else()
  set(TAV_VIDEO_ALL_SRCS ${tav_video_SRC} ${tav_video_nvvideo_SRC})
endif(UNIX)

if(BUILD_STATIC)
  add_library(tav_video STATIC ${TAV_VIDEO_ALL_SRCS})
else()
  add_library(tav_video SHARED ${TAV_VIDEO_ALL_SRCS})
endif(BUILD_STATIC)
#cotire(tav_video)

find_package (Boost 1.55 REQUIRED thread)
find_package (FFmpeg REQUIRED)
if (WITH_GSTREAMER)
  find_package (GLIB)
  find_package (GStreamer)
endif(WITH_GSTREAMER)
find_package (GLEW REQUIRED)
find_package (LibJpegTurbo REQUIRED)
find_package (OpenGL REQUIRED)
if (WITH_OPENCV)
  find_package (OpenCV)
endif(WITH_OPENCV)
if (WITH_CUDA)
  find_package (CUDA)
endif(WITH_CUDA)

if(BUILD_STATIC)
  set(LIB_LINK_OPT, "-static")
else()
  set(LIB_LINK_OPT, "")
endif(BUILD_STATIC)


if (LibJpegTurbo_FOUND)
  include_directories(${LibJpegTurbo_INCLUDE_DIR})
  target_link_libraries(tav_video ${LibJpegTurbo_LIBRARY} ${LIB_LINK_OPT})
endif (LibJpegTurbo_FOUND)

# OpenCV
if (WITH_OPENCV AND OPENCV_FOUND)
  add_definitions(-DHAVE_OPENCV)
  include_directories(${OPENCV_INCLUDE_DIRS})
  target_link_libraries(tav_video ${OpenCV_LIBS} ${LIB_LINK_OPT})
endif (WITH_OPENCV AND OPENCV_FOUND)

# Boost
if (Boost_FOUND)
  include_directories(${BOOST_INCLUDE_DIRS})
  target_link_libraries (tav_video ${BOOST_LIBRARIES} ${LIB_LINK_OPT})
endif (Boost_FOUND)

# GLEW
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIRS})
  target_link_libraries (tav_video ${GLEW_LIBRARIES} ${LIB_LINK_OPT})
endif (GLEW_FOUND)

# GSTREAMER
if (GSTREAMER_FOUND)
	message("gstreamer found ")
endif(GSTREAMER_FOUND)

if (WITH_GSTREAMER AND GSTREAMER_FOUND)
  add_definitions(-DHAVE_GSTREAMER)
  include_directories(${GSTREAMER_INCLUDE_DIRS})
  target_link_libraries (tav_video ${GSTREAMER_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_GSTREAMER AND GSTREAMER_FOUND)

if (WITH_GSTREAMER AND GLIB_FOUND)
  include_directories(${GLIB_INCLUDE_DIRS})
  target_link_libraries (tav_video ${GLIB_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_GSTREAMER AND GLIB_FOUND)


#  CUDA
if (CUDA_FOUND)
 	add_definitions(-DHAVE_CUDA)
	include_directories(${CUDA_INCLUDE_DIRS})
	target_link_libraries (tav_video ${CUDA_LIBRARIES})
endif (CUDA_FOUND)

# FFMPEG
if (FFMPEG_FOUND)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  # if we are linking statically we need the static libraries of ffmpeg
  if(BUILD_STATIC)
    STRING(REGEX REPLACE ".so" ".a" FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES})
    target_link_libraries(tav_video -Wl,--whole-archive ${FFMPEG_LIBRARIES} -Wl,--no-whole-archive ${LIB_LINK_OPT})
  else()
    target_link_libraries(tav_video ${FFMPEG_LIBRARIES} ${LIB_LINK_OPT})    
  endif(BUILD_STATIC)
endif (FFMPEG_FOUND)

# OpenGL
if (OpenGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries (tav_video ${OPENGL_LIBRARIES} ${LIB_LINK_OPT})
endif (OpenGL_FOUND)

# NVIDIA Video Codec SDK 
if (WITH_NVVIDEO)
	add_definitions(-DINIT_CUDA_GL=1)
 	add_definitions(-DHAVE_NVVIDEO)
endif (WITH_NVVIDEO)

add_dependencies(tav_video tav_core)

if (NOT WITH_OPENCV)
  remove_definitions(-DHAVE_OPENCV)
endif (NOT WITH_OPENCV)


# add the install targets
install (TARGETS tav_video DESTINATION "lib")

file( GLOB _VideoHeaders "src/*.h" )
install(FILES ${_VideoHeaders} DESTINATION "include/tav/tav_video")

if (UNIX)
file( GLOB _VideoHeadersV4L "src/V4L/*.h" )
install(FILES ${_VideoHeadersV4L} DESTINATION "include/tav/tav_video/V4L")
endif(UNIX)
