cmake_minimum_required (VERSION 2.6)
project (tav_audio)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE Release)

include(cotire)

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


option (BUILD_STATIC "Build Static Library" OFF)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

set(TAV_AUDIO_SRCS src/AubioAnalyzer.cpp src/PaPhaseLockLoop.cpp src/FFT.cpp src/MultiChanFFT.cpp src/SoundFilePlayer.cpp src/PAudio.cpp)

if(BUILD_STATIC)
add_library(tav_audio STATIC src/AubioAnalyzer.cpp src/PaPhaseLockLoop.cpp src/FFT.cpp src/MultiChanFFT.cpp src/SoundFilePlayer.cpp src/PAudio.cpp) 
else(NOT BUILD_STATIC)
add_library(tav_audio SHARED src/AubioAnalyzer.cpp src/PaPhaseLockLoop.cpp src/FFT.cpp src/MultiChanFFT.cpp src/SoundFilePlayer.cpp src/PAudio.cpp) 
endif(BUILD_STATIC)  
cotire(tav_audio)

find_package (Boost 1.55 REQUIRED thread)
find_package (GLEW REQUIRED)
find_package (OpenGL REQUIRED)
find_package (Portaudio REQUIRED)
find_package (Aubio REQUIRED)
find_package (LibSndFile REQUIRED)


# Aubio
if (AUBIO_FOUND)
  include_directories(${AUBIO_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${AUBIO_LIBRARIES})
endif (AUBIO_FOUND)

# Boost
if (Boost_FOUND)
  include_directories(${BOOST_INCLUDE_DIRS})
  target_link_libraries (tav_audio ${BOOST_LIBRARIES})
endif (Boost_FOUND)

# GLEW
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIRS})
  target_link_libraries (tav_audio ${GLEW_LIBRARIES})
endif (GLEW_FOUND)

# OpenGL
if (OpenGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries (tav_audio ${OPENGL_LIBRARIES})
endif (OpenGL_FOUND)

if (LIBSNDFILE_FOUND)
  include_directories(${LIBSNDFILE_INCLUDE_DIR})
  target_link_libraries (tav_audio ${LIBSNDFILE_LIBRARIES})
endif (LIBSNDFILE_FOUND)


# Portaudio
if (PORTAUDIO_FOUND)
  include_directories(${PORTAUDIO_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${PORTAUDIO_LIBRARIES})
endif (PORTAUDIO_FOUND)

#add_dependencies(tav_audio tav_core)

target_link_libraries(tav_audio ${LIBS})


# add the install targets
install (TARGETS tav_audio DESTINATION "lib")
install (FILES src/AubioAnalyzer.h src/FFT.h src/MultiChanFFT.h src/PaPhaseLockLoop.h src/PAudio.h DESTINATION "include/tav/tav_audio")
