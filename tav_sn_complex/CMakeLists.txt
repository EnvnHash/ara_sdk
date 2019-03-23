cmake_minimum_required (VERSION 2.6)
project (tav_sn_complex)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE Release)

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


option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_AUDIO "Build Tav Audio Module" ON)
option (WITH_CUDA "Use Cuda" ON)
option (WITH_OPENNI2 "Use OpenNI2" ON)
option (WITH_NITE2 "Use Nite2" ON)
option (WITH_OPENCV "Use OpenCV" ON)
option (WITH_PROJ_ABSOLUT "Build the scene corresponding to project absolut" OFF)
option (WITH_PROJ_CAMCHAL "Build the scene corresponding to project camchal" OFF)
option (WITH_PROJ_COKE "Build the scene corresponding to project coca cola" OFF)
option (WITH_PROJ_ENEL "Build the scene corresponding to project enel" OFF)
option (WITH_PROJ_IKPI "Build the scene corresponding to project ikpi" ON)
option (WITH_PROJ_GAM "Build the scene corresponding to project gam" OFF)
option (WITH_PROJ_TRUST "Build the scene corresponding to project trust" OFF)
option (WITH_GSTREAMER "Build with GStreamer" OFF)

if(WITH_AUDIO)
	add_definitions(-DWITH_AUDIO)
endif(WITH_AUDIO)

# tell GLFW to not include gl headers
add_definitions(-DGLFW_INCLUDE_NONE)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src/headers")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_tracking/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_video/src")

file(GLOB tav_sn_complex_absolut_SRC "")
file(GLOB tav_sn_complex_camchal_SRC "")
file(GLOB tav_sn_complex_coke_SRC "")
file(GLOB tav_sn_complex_enel_SRC "")
file(GLOB tav_sn_complex_ikpi_SRC "")
file(GLOB tav_sn_complex_trust_SRC "")
file(GLOB tav_sn_complex_gam_mustakis_danza_SRC "")
file(GLOB tav_sn_complex_gam_mustakis_musica_SRC "")
file(GLOB tav_sn_complex_SRC "src/*.cpp")

if (NOT WITH_OPENNI2)
  list(REMOVE_ITEM tav_sn_complex_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/SNAutumnLeavesVelTex.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNAutumnLeaves.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNKinectPainting.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNKinectPainting2.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNTKinTouch.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNRotatingObjMapping.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNRotatingObjMapping2.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNTunnelCokeBubbles.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNTTexPartFluid.cpp")
endif (NOT WITH_OPENNI2)

if (NOT WITH_NVIDIA_TYPO)
  list(REMOVE_ITEM tav_sn_complex_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/SNSingleTextBlock.cpp")
endif (NOT WITH_NVIDIA_TYPO)

if (WITH_PROJ_ABSOLUT AND WITH_OPENNI2)
  file(GLOB tav_sn_complex_absolut_SRC "src/absolut/*.cpp")
endif(WITH_PROJ_ABSOLUT AND WITH_OPENNI2)

if (WITH_PROJ_ENEL AND WITH_OPENNI2)
  file(GLOB tav_sn_complex_enel_SRC "src/Enel/*.cpp")
endif(WITH_PROJ_ENEL AND WITH_OPENNI2)

if (WITH_PROJ_IKPI AND WITH_OPENCV)
  file(GLOB tav_sn_complex_ikpi_SRC "src/ikpi/*.cpp")
endif (WITH_PROJ_IKPI AND WITH_OPENCV)

if (WITH_PROJ_GAM AND WITH_OPENNI2)
  file(GLOB tav_sn_complex_gam_mustakis_danza_SRC "src/gam_mustakis/SNGam_Danza.cpp")
endif (WITH_PROJ_GAM AND WITH_OPENNI2)
  
if (WITH_PROJ_GAM AND WITH_AUDIO AND WITH_NVIDIA_TYPO)
  file(GLOB tav_sn_complex_gam_mustakis_musica_SRC "src/gam_mustakis/SNGam_Musica.cpp")
endif(WITH_PROJ_GAM AND WITH_AUDIO AND WITH_NVIDIA_TYPO)

if (WITH_PROJ_GAM AND WITH_AUDIO)
  file(GLOB tav_sn_complex_gam_mustakis_musica_fondo_SRC "src/gam_mustakis/SNGam_Musica_Fondo.cpp")
endif(WITH_PROJ_GAM AND WITH_AUDIO)

if (WITH_PROJ_GAM AND WITH_OPENCV)
  file(GLOB tav_sn_complex_gam_mustakis_libro_SRC "src/gam_mustakis/SNGam_Libro.cpp")
endif (WITH_PROJ_GAM AND WITH_OPENCV)

if (WITH_PROJ_COKE AND WITH_OPENCV)
  file(GLOB tav_sn_complex_coke_SRC "src/Coke/*.cpp")
endif (WITH_PROJ_COKE AND WITH_OPENCV)

if (WITH_PROJ_CAMCHAL AND WITH_OPENCV)
  file(GLOB tav_sn_complex_camchal_SRC "src/camchal/*.cpp")
endif (WITH_PROJ_CAMCHAL AND WITH_OPENCV)

if (WITH_PROJ_TRUST AND WITH_CUDA)
  file(GLOB tav_sn_complex_trust_SRC "src/Trust/*.cpp")
endif(WITH_PROJ_TRUST AND WITH_CUDA)


if (NOT WITH_OPENCV)
  list(REMOVE_ITEM tav_sn_complex_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/SNKinectPainting2.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNKinectPainting.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNAutumnLeavesVelTex.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SNAutumnLeaves.cpp")
endif(NOT WITH_OPENCV)

set(TAV_SN_COMPLEX_ALL_SRC ${tav_sn_complex_ikpi_SRC} ${tav_sn_complex_absolut_SRC} ${tav_sn_complex_gam_mustakis_danza_SRC} ${tav_sn_complex_gam_mustakis_musica_SRC} ${tav_sn_complex_gam_mustakis_musica_fondo_SRC} ${tav_sn_complex_gam_mustakis_libro_SRC} ${tav_sn_complex_trust_SRC} ${tav_sn_complex_camchal_SRC} ${tav_sn_complex_enel_SRC} ${tav_sn_complex_SRC})

if(BUILD_STATIC)
  add_library(tav_sn_complex STATIC ${TAV_SN_COMPLEX_ALL_SRC}) 
else()
  add_library(tav_sn_complex SHARED ${TAV_SN_COMPLEX_ALL_SRC}) 
endif(BUILD_STATIC)

#cotire(tav_sn_complex)


if(WITH_CUDA)
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
if (WITH_GSTREAMER)
  find_package (GLIB)
  find_package (GStreamer)
endif(WITH_GSTREAMER)

if (WITH_PROJ_IKPI AND WITH_OPENCV)
  find_package(MYSQL REQUIRED)
endif (WITH_PROJ_IKPI AND WITH_OPENCV)

# Cuda
if (WITH_CUDA AND CUDA_FOUND)
  add_definitions(-DHAVE_CUDA)
  include_directories(${CUDA_INCLUDE_DIRS})
  target_link_libraries (tav_sn_complex ${CUDA_LIBRARIES})
endif (WITH_CUDA AND CUDA_FOUND)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_sn_complex ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

# HarfBuzz
if (HARFBUZZ_FOUND)
  include_directories(${HARFBUZZ_INCLUDE_DIRS})
  target_link_libraries (tav_sn_complex ${HARFBUZZ_LIBRARIES})
endif (HARFBUZZ_FOUND)

if (WITH_GSTREAMER AND GSTREAMER_FOUND)
  add_definitions(-DHAVE_GSTREAMER)
  include_directories(${GSTREAMER_INCLUDE_DIRS})
  target_link_libraries (tav_sn_complex ${GSTREAMER_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_GSTREAMER AND GSTREAMER_FOUND)

if (WITH_GSTREAMER AND GLIB_FOUND)
  include_directories(${GLIB_INCLUDE_DIRS})
  target_link_libraries (tav_sn_complex ${GLIB_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_GSTREAMER AND GLIB_FOUND)

# NITE2
if (WITH_NITE2 AND NITE2_FOUND)
  include_directories(${NITE2_INCLUDE_DIRS})
  target_link_libraries(tav_sn_complex ${NITE2_LIBS})
endif (WITH_NITE2 AND NITE2_FOUND)

# OpenNI2
if (WITH_OPENNI2 AND OPENNI2_FOUND)
  include_directories(${OPENNI2_INCLUDE_DIRS})
  target_link_libraries(tav_sn_complex ${OPENNI2_LIBS})
endif (WITH_OPENNI2 AND OPENNI2_FOUND)

# OpenCV
if (WITH_OPENCV AND OpenCV_FOUND)
  add_definitions(-DHAVE_OPENCV)
  include_directories(${OpenCV_INCLUDE_DIRS})
  target_link_libraries (tav_sn_complex ${OpenCV_LIBRARIES})
endif(WITH_OPENCV AND OpenCV_FOUND)


if (WITH_PROJ_IKPI)
  include_directories(${MYSQL_INCLUDE_DIRS})
  message("including ${MYSQL_INCLUDE_DIRS}")
  target_link_libraries (tav_sn_complex ${MYSQL_CLIENT_LIBS})
  target_link_libraries (tav_sn_complex tav_tracking)
endif (WITH_PROJ_IKPI)

target_link_libraries(tav_sn_complex ${LIBS})

# add_dependencies
add_dependencies(tav_sn_complex tav_core)
if (WITH_PROJ_IKPI)
	add_dependencies(tav_sn_complex tav_tracking)
endif (WITH_PROJ_IKPI)

# add the install targets
install(TARGETS tav_sn_complex DESTINATION "lib")

file( GLOB _HeadersCore "src/*.h" )
install(FILES ${_HeadersCore} DESTINATION "include/tav/tav_sn_complex")
