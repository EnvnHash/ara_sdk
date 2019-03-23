cmake_minimum_required (VERSION 2.6)
project (tav_tracking)

#include(cotire)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE Release)
set(OpenGL_GL_PREFERENCE "LEGACY")

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

# The version number.
set (Tutorial_VERSION_MAJOR 0)
set (Tutorial_VERSION_MINOR 1)

option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_CUDA "Use Cuda" ON)
option (WITH_OPENCV "Use OpenCV" ON)
option (WITH_OPENNI2 "Use OpenNI2" ON)
option (WITH_NITE2 "Use Nite2" ON)
option (WITH_FREENECT2 "Use libfreenect2" ON)

# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_tracking/src")


# add glm experimental flag
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)


file(GLOB tav_tracking_freenect2_SRC "")
file(GLOB tav_tracking_kinect_SRC "")
file(GLOB tav_tracking_nite_SRC "")
file(GLOB tav_tracking_SRC "src/*.cpp")

if (WITH_FREENECT2)
  add_definitions(-DUSE_FREENECT2)
  file(GLOB tav_tracking_freenect2_SRC "src/Freenect2/*.cpp")
endif(WITH_FREENECT2)

if (WITH_OPENNI2)
  add_definitions(-DUSE_OPENNI2)
  file(GLOB tav_tracking_kinect_SRC "src/KinectInput/*.cpp")
endif(WITH_OPENNI2)

if (WITH_NITE2 AND WITH_OPENCV)
  file(GLOB tav_tracking_nite_SRC "src/NiTE/*.cpp")
endif(WITH_NITE2 AND WITH_OPENCV)

#file(GLOB tav_tracking_stereo_SRC "src/StereoMatching/*.cpp")

if (NOT WITH_OPENCV)
  list(REMOVE_ITEM tav_tracking_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/VideoActivityRange.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/TrackObj.cpp")
  list(REMOVE_ITEM tav_tracking_kinect_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/KinectInput/KinectPointCloud.cpp")
endif(NOT WITH_OPENCV)

  
set(TAV_TRACKING_ALL_SRC ${tav_tracking_SRC} ${tav_tracking_freenect2_SRC} ${tav_tracking_kinect_SRC} ${tav_tracking_nite_SRC} ${tav_tracking_stereo_SRC})

add_dependencies(tav_tracking tav_core)

if(BUILD_STATIC)
add_library(tav_tracking STATIC ${TAV_TRACKING_ALL_SRC})
else()
add_library(tav_tracking SHARED ${TAV_TRACKING_ALL_SRC})
endif(BUILD_STATIC)

#cotire(tav_tracking)


#find packages
find_package (Boost 1.55 REQUIRED thread)
find_package (GLEW REQUIRED)
find_package (GLM REQUIRED)

if (WITH_FREENECT2)
  find_package (Freenect2 REQUIRED)
endif (WITH_FREENECT2)

find_package (Freetype REQUIRED)
find_package (OpenGL REQUIRED)

if (WITH_OPENCV)
  find_package (OpenCV)
endif(WITH_OPENCV)

if(WITH_OPENNI2)
  find_package (OpenNI2 REQUIRED)
endif(WITH_OPENNI2)

if(WITH_NITE2)
  find_package (NITE2 REQUIRED)
endif(WITH_NITE2)



# glm
if (GLM_FOUND)
  include_directories(${GLM_INCLUDE_DIRS})
endif (GLM_FOUND)

# flex
if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_tracking ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

# NITE2
if (WITH_NITE2 AND NITE2_FOUND)
  include_directories(${NITE2_INCLUDE_DIRS})
  target_link_libraries(tav_tracking ${NITE2_LIBRARIES})
endif (WITH_NITE2 AND NITE2_FOUND)

# OpenNI2
if (WITH_OPENNI2 AND OPENNI2_FOUND)
  include_directories(${OPENNI2_INCLUDE_DIRS})
  target_link_libraries(tav_tracking ${OPENNI2_LIBRARIES})
endif (WITH_OPENNI2 AND OPENNI2_FOUND)

# Freenect2
if (WITH_FREENECT2 AND FREENECT2_FOUND)
  include_directories(${FREENECT2_INCLUDE_DIRS})
  target_link_libraries(tav_tracking ${FREENECT2_LIBRARIES})
endif (WITH_FREENECT2 AND FREENECT2_FOUND)

# OpenCV
if (WITH_OPENCV AND OPENCV_FOUND)
  add_definitions(-DHAVE_OPENCV)
  include_directories(${OPENCV_INCLUDE_DIRS})
  target_link_libraries(tav_tracking ${OpenCV_LIBRARIES})
endif (WITH_OPENCV AND OPENCV_FOUND)

# Boost
if (Boost_FOUND)
  include_directories(${BOOST_INCLUDE_DIRS})
  target_link_libraries (tav_tracking ${BOOST_LIBRARIES})
endif (Boost_FOUND)

# GLEW
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIRS})
  target_link_libraries (tav_tracking ${GLEW_LIBRARIES})
endif (GLEW_FOUND)

# OpenGL
if (OpenGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries (tav_tracking ${OPENGL_LIBRARIES})
endif (OpenGL_FOUND)


# add the install targets
install (TARGETS tav_tracking DESTINATION "lib")

file( GLOB _TrackingHeaders "src/*.h" )
install(FILES ${_TrackingHeaders} DESTINATION "include/tav/tav_tracking")

if (WITH_FREENECT2 AND FREENECT2_FOUND)
  file( GLOB _TrackingFreenect2Headers "src/Freenect2/*.h" )
  install(FILES ${_TrackingFreenect2Headers} DESTINATION "include/tav/tav_tracking/Freenect2")
endif (WITH_FREENECT2 AND FREENECT2_FOUND)
  
if (WITH_OPENNI2 AND OPENNI2_FOUND)
  file( GLOB _TrackingKinectHeaders "src/KinectInput/*.h" )
  install(FILES ${_TrackingKinectHeaders} DESTINATION "include/tav/tav_tracking/KinectInput")
endif (WITH_OPENNI2 AND OPENNI2_FOUND)

if (WITH_NITE2 AND NITE2_FOUND)
  file( GLOB _TrackingNiteHeaders "src/NiTE/*.h" )
  install(FILES ${_TrackingNiteHeaders} DESTINATION "include/tav/tav_tracking/NiTE")
endif (WITH_NITE2 AND NITE2_FOUND)

file( GLOB _TrackingStereoHeaders "src/StereoMatching/*.h" )
install(FILES ${_TrackingStereoHeaders} DESTINATION "include/tav/tav_tracking/StereoMatching")
