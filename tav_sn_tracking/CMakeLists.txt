cmake_minimum_required (VERSION 2.6)
project (tav_sn_tracking)

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

#Options
option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_OPENNI2 "Use OpenNI2" ON)
option (WITH_NITE2 "Use Nite2" ON)


# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/kinect")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/particle")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/typo")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/video")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src/headers")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_tracking/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_video/src")

file(GLOB tav_sn_tracking_SRC "src/*.cpp")

if(BUILD_STATIC)
  add_library(tav_sn_tracking STATIC ${tav_sn_tracking_SRC}) 
else()
  add_library(tav_sn_tracking SHARED ${tav_sn_tracking_SRC}) 
endif(BUILD_STATIC)

#cotire(tav_sn_tracking)


find_package (Freetype REQUIRED)
if (WITH_OPENNI2)
  find_package (OpenNI2 REQUIRED)
endif (WITH_OPENNI2)
if (WITH_NITE2)
  find_package (NITE2 REQUIRED)
endif (WITH_NITE2)


# Boost
if (BOOST_FOUND)
  include_directories(${BOOST_INCLUDE_DIRS})
  target_link_libraries (tav_sn_tracking ${BOOST_LIBRARIES})
endif (BOOST_FOUND)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_sn_tracking ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

# NITE2
if (WITH_NITE2 AND NITE2_FOUND)
  add_definitions(-DHAVE_NITE2)
  include_directories(${NITE2_INCLUDE_DIRS})
  target_link_libraries(tav_sn_tracking ${NITE2_LIBS})
endif (WITH_NITE2 AND NITE2_FOUND)

# OpenNI2
if (WITH_OPENNI2 AND OPENNI2_FOUND)
  add_definitions(-DHAVE_OPENNI2)
  include_directories(${OPENNI2_INCLUDE_DIRS})
  target_link_libraries(tav_sn_tracking ${OPENNI2_LIBS})
endif (WITH_OPENNI2 AND OPENNI2_FOUND)


target_link_libraries(tav_sn_tracking ${LIBS})

# tell GLFW to not include gl headers
add_definitions(-DGLFW_INCLUDE_NONE)

add_dependencies(tav_sn_tracking tav_core)

# add the install targets
install (TARGETS tav_sn_tracking DESTINATION "lib")

file( GLOB _HeadersCore "src/*.h" )
install(FILES ${_HeadersCore} DESTINATION "include/tav/tav_sn_tracking")
