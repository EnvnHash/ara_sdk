cmake_minimum_required (VERSION 2.6)
project (tav_sn_audio)

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

option (BUILD_STATIC "Build Static Library" OFF)


# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")

file(GLOB tav_sn_audio_SRC "src/*.cpp")

if(BUILD_STATIC)
  add_library(tav_sn_audio STATIC ${tav_sn_audio_SRC}) 
else()
  add_library(tav_sn_audio SHARED ${tav_sn_audio_SRC}) 
endif(BUILD_STATIC)

#cotire(tav_sn_audio)


#find_package (assimp REQUIRED)
#find_package (Boost 1.55 REQUIRED thread)
find_package (Freetype REQUIRED)
#find_package (GLEW REQUIRED)
#find_package (GLM REQUIRED)
#find_package (HarfBuzz REQUIRED)
#find_package (OpenGL REQUIRED)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_sn_audio ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)


target_link_libraries(tav_sn_audio ${LIBS})

# tell GLFW to not include gl headers
add_definitions(-DGLFW_INCLUDE_NONE)

# make sure tav_core has been built before
add_dependencies(tav_sn_audio tav_core tav_audio)

# add the install targets
install (TARGETS tav_sn_audio DESTINATION "lib")

file( GLOB _HeadersCore "src/*.h" )
install(FILES ${_HeadersCore} DESTINATION "include/tav/tav_sn_audio")
