cmake_minimum_required(VERSION 3.7.2)

project (tav_core)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE Debug)
set(OpenGL_GL_PREFERENCE "LEGACY")

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


# The version number.
set (OPENGL_VERSION_MAJOR 4)
set (OPENGL_VERSION_MINOR 5)

option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_AUDIO "Build Tav Audio Module" ON)
option (WITH_CUDA "Use Cuda" ON)
option (WITH_NVIDIA_TYPO "Use Nvidia font rendering support" ON)
option (WITH_OPENCV "Use OpenCV" ON)
option (WITH_OPENNI2 "Use OpenNI2" ON)
option (WITH_NITE2 "Use Nite2" ON)

if(WITH_AUDIO)
	add_definitions(-DWITH_AUDIO)
endif(WITH_AUDIO)
	
# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")

find_package (liblo REQUIRED)
find_package (assimp REQUIRED)
find_package (Boost 1.55 REQUIRED regex thread system)
find_package (Freetype REQUIRED)
find_package (FreeImage REQUIRED)

if(WITH_CUDA)
  find_package (CUDA)
  find_package (Flex)
endif(WITH_CUDA)
find_package (GLEW REQUIRED)
find_package (GLFW REQUIRED)
find_package (GLM REQUIRED)
find_package (HarfBuzz REQUIRED)
find_package (OpenGL REQUIRED)
if (WITH_OPENCV)
  find_package (OpenCV)
endif(WITH_OPENCV)
find_package (UCDN)
if (WITH_MUSICXML)
  find_package (MusicXml REQUIRED)
endif(WITH_MUSICXML)


# tell GLFW to not include gl headers
add_definitions(-DGLFW_INCLUDE_NONE)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)


#optional includes
file(GLOB tav_core_SRC_FLEX "")
file(GLOB tav_core_SRC_GLUTILS_NVIDIA_TYPO "")
file(GLOB tav_core_SRC_OPENCV "")
file(GLOB tav_core_SRC_GLUTILS_TYPO_NVIDIA "")
file(GLOB tav_core_SRC_MUSICXML "")

file(GLOB tav_core_SRC "src/*.cpp")
file(GLOB tav_core_SRC_GLUTILS "src/GLUtils/*.cpp")
file(GLOB tav_core_SRC_GLUTILS_ASSIMP "src/GLUtils/Assimp/*.cpp")
file(GLOB tav_core_SRC_GLUTILS_TYPO "src/GLUtils/Typo/*.cpp")
if (WITH_NVIDIA_TYPO)
	file(GLOB tav_core_SRC_GLUTILS_TYPO_NVIDIA "src/GLUtils/Typo/Nvidia/*.cpp")
endif(WITH_NVIDIA_TYPO)
file(GLOB tav_core_SRC_GLUTILS_GLSL "src/GLUtils/GLSL/*.cpp")
file(GLOB tav_core_SRC_GEOPRIMITIVES "src/GeoPrimitives/*.cpp")
file(GLOB tav_core_SRC_MESHES "src/Meshes/*.cpp")
file(GLOB tav_core_SRC_HEADERS "src/headers/*.cpp")
file(GLOB tav_core_SRC_SHADERS "src/Shaders/*.cpp")
file(GLOB tav_core_SRC_SHADERUTILS "src/Shaders/ShaderUtils/*.cpp")
file(GLOB tav_core_SRC_SHADOWMAPS "src/Shaders/ShadowMaps/*.cpp")
file(GLOB tav_core_SRC_SCENENODES "src/SceneNodes/*.cpp")
file(GLOB tav_core_SRC_CAMERASETS "src/CameraSets/*.cpp")
file(GLOB tav_core_SRC_GUI "src/GUI/*.cpp")
file(GLOB tav_core_SRC_COMMUNICATION "src/Communication/*.cpp")
file(GLOB tav_core_SRC_OSC "src/Communication/OSC/*.cpp")
file(GLOB tav_core_SRC_SHADERPROTO "src/ShaderPrototype/*.cpp")
file(GLOB tav_core_SRC_TINY "src/tinyObj/*.cpp")



if (NOT WITH_OPENCV)
  list(REMOVE_ITEM tav_core_SRC_GLUTILS "${CMAKE_CURRENT_SOURCE_DIR}/src/GLUtils/FboDedistPersp.cpp")
  list(REMOVE_ITEM tav_core_SRC_CAMERASETS "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1FboAspectQuad.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1Fbo.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1PerspFboBackVid.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1FboAspect.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1PerspFboBack.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1PerspFboConv.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1PerspFboFft.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs2InRowVert.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs2InRowVertFbo.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs3FloorWall.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs3FloorWallFbo.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs3Tunnel.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs5Tunnel.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs6FloorTunnel.cpp")
endif (NOT WITH_OPENCV)

if (NOT WITH_AUDIO)
  list(REMOVE_ITEM tav_core_SRC_CAMERASETS "${CMAKE_CURRENT_SOURCE_DIR}/src/CameraSets/Cs1PerspFboFft.cpp")
endif (NOT WITH_AUDIO)

if (WITH_CUDA AND FLEX_FOUND)
  file(GLOB tav_core_SRC_FLEX "src/flex/*.cpp")
endif (WITH_CUDA AND FLEX_FOUND)

if (WITH_MUSICXML)
  file(GLOB tav_core_SRC_MUSICXML "src/MusicXml/*.cpp")
endif (WITH_MUSICXML)

if (WITH_OPENCV AND OpenCV_FOUND)
  file(GLOB tav_core_SRC_OPENCV "src/OpenCvUtils/*.cpp")
endif (WITH_OPENCV AND OpenCV_FOUND)

if (NOT WITH_OPENCV)
  remove_definitions(-DHAVE_OPENCV)
endif (NOT WITH_OPENCV)

if (WITH_NVIDIA_TYPO)
  file(GLOB tav_core_SRC_GLUTILS_NVIDIA_TYPO "src/GLUtils/Typo/Nvidia/*.cpp")
endif (WITH_NVIDIA_TYPO)


set(TAV_CORE_SRCS ${tav_core_SRC} ${tav_core_SRC_FLEX} ${tav_core_SRC_GLUTILS} ${tav_core_SRC_GLUTILS_ASSIMP} ${tav_core_SRC_GLUTILS_TYPO}
	${tav_core_SRC_GLUTILS_TYPO_NVIDIA} ${tav_core_SRC_GLUTILS_GLSL} ${tav_core_SRC_GEOPRIMITIVES} ${tav_core_SRC_MESHES} ${tav_core_SRC_HEADERS}
	${tav_core_SRC_SHADERS} ${tav_core_SRC_SHADERUTILS} ${tav_core_SRC_SHADOWMAPS} ${tav_core_SRC_CAMERASETS} ${tav_core_SRC_GUI} ${tav_core_SRC_COMMUNICATION} 
	${tav_core_SRC_OSC} ${tav_core_SRC_MUSICXML} ${tav_core_SRC_OPENCV} ${tav_core_SRC_SHADERPROTO} ${tav_core_SRC_SCENENODES} ${tav_core_SRC_TINY})

if(BUILD_STATIC)
add_library(tav_core STATIC ${TAV_CORE_SRCS})
else(NOT BUILD_STATIC)
add_library(tav_core SHARED ${TAV_CORE_SRCS})
endif(BUILD_STATIC)

cotire(tav_core)



# assimp
if (ASSIMP_FOUND)
  include_directories(${ASSIMP_INCLUDE_DIRS})
  target_link_libraries (tav_core ${ASSIMP_LIBRARIES})
endif (ASSIMP_FOUND)

# Boost
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries (tav_core ${Boost_LIBRARIES})
endif (Boost_FOUND)

# Cuda
if (WITH_CUDA AND CUDA_FOUND)
  add_definitions(-DHAVE_CUDA)
  include_directories(${CUDA_INCLUDE_DIRS})
  target_link_libraries (tav_core ${CUDA_LIBRARIES})
endif (WITH_CUDA AND CUDA_FOUND)

if (WITH_CUDA AND FLEX_FOUND)
  add_definitions(-DHAVE_FLEX)
  include_directories(${FLEX_INCLUDE_DIRS})
  target_link_libraries(tav_core ${FLEX_LIBRARIES})
endif (WITH_CUDA AND FLEX_FOUND)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav_core ${FREETYPE_LIBRARIES})
endif (FREETYPE_FOUND)

# Freeimage
if (FREEIMAGE_FOUND)
  include_directories(${FREEIMAGE_INCLUDE_DIRS})
  target_link_libraries (tav_core ${FREEIMAGE_LIBRARIES} ${LIB_LINK_OPT})
endif (FREEIMAGE_FOUND)

# GLEW
if (GLEW_FOUND)
  add_definitions(-DHAVE_GLEW)
  include_directories(${GLEW_INCLUDE_DIRS})
  target_link_libraries (tav_core ${GLEW_LIBRARIES})
endif (GLEW_FOUND)

# GLFW
if (GLFW_FOUND)
  include_directories(${GLFW_INCLUDE_DIRS})
  target_link_libraries (tav_core ${GLFW_LIBRARIES})
endif (GLFW_FOUND)

# glm
if (GLM_FOUND)
  add_definitions(-DHAVE_GLM)
  include_directories(${GLM_INCLUDE_DIRS})
endif (GLM_FOUND)

# HarfBuzz
if (HARFBUZZ_FOUND)
  include_directories(${HARFBUZZ_INCLUDE_DIRS})
  target_link_libraries (tav_core ${HARFBUZZ_LIBRARIES})
endif (HARFBUZZ_FOUND)

# OpenGL
if (OpenGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries (tav_core ${OPENGL_LIBRARIES})
endif (OpenGL_FOUND)

if (LIBLO_FOUND)
  include_directories(${LIBLO_INCLUDE_DIRS})
  target_link_libraries (tav_core ${LIBLO_LIBRARIES} ${LIB_LINK_OPT})
endif (LIBLO_FOUND)

if (WITH_OPENCV AND OpenCV_FOUND)
  add_definitions(-DHAVE_OPENCV)
  include_directories(${OpenCV_INCLUDE_DIRS})
  target_link_libraries (tav_core ${OpenCV_LIBRARIES})
endif (WITH_OPENCV AND OpenCV_FOUND)

#MusicXml
if (WITH_MUSICXML AND MUSICXML_FOUND)
  add_definitions(-DHAVE_MUSICXML)  
  include_directories(${MUSICXML_INCLUDE_DIRS})
  target_link_libraries (tav_core ${MUSICXML_LIBRARIES})
endif (WITH_MUSICXML AND MUSICXML_FOUND)

# UCDN
if (UCDN_FOUND)
  include_directories(${UCDN_INCLUDE_DIRS})
  target_link_libraries(tav_core ${UCDN_LIBRARY})
endif (UCDN_FOUND)


# add the install targets
install (TARGETS tav_core DESTINATION "lib")

file( GLOB _HeadersCore "src/*.h" )
install(FILES ${_HeadersCore} DESTINATION "include/tav/tav_core")

file( GLOB _HeadersCoreCameraSets "src/CameraSets/*.h" )
install(FILES ${_HeadersCoreCameraSets} DESTINATION "include/tav/tav_core/CameraSets")

file( GLOB _HeadersCoreComm "src/Communication/*.h" )
install(FILES ${_HeadersCoreComm} DESTINATION "include/tav/tav_core/Communication")

file( GLOB _HeadersCoreOsc "src/Communication/OSC/*.h" )
install(FILES ${_HeadersCoreOsc} DESTINATION "include/tav/tav_core/Communication/OSC")

if (WITH_CUDA AND FLEX_FOUND)
  file( GLOB _HeadersCoreFlex "src/flex/*.h" )
  install(FILES ${_HeadersCoreFlex} DESTINATION "include/tav/tav_core/flex")
endif(WITH_CUDA AND FLEX_FOUND)

file( GLOB _HeadersCoreGLUtils "src/GLUtils/*.h" )
install(FILES ${_HeadersCoreGLUtils} DESTINATION "include/tav/tav_core/GLUtils")

file( GLOB _HeadersCoreGLSL "src/GLUtils/GLSL/*.h" )
install(FILES ${_HeadersCoreGLSL} DESTINATION "include/tav/tav_core/GLUtils/GLSL")

file( GLOB _HeadersCoreGLSLAssimp "src/GLUtils/GLSL/Assimp/*.h" )
install(FILES ${_HeadersCoreGLSLAssimp} DESTINATION "include/tav/tav_core/GLUtils/GLSL/Assimp")

file( GLOB _HeadersCoreGLSLTypo "src/GLUtils/GLSL/Typo/*.h" )
install(FILES ${_HeadersCoreGLSLTypo} DESTINATION "include/tav/tav_core/GLUtils/GLSL/Typo")

if (WITH_NVIDIA_TYPO)
	file( GLOB _HeadersCoreGLSLTypoNvidia "src/GLUtils/GLSL/Typo/Nvidia*.h" )
	install(FILES ${_HeadersCoreGLSLTypoNvidia} DESTINATION "include/tav/tav_core/GLUtils/GLSL/Typo/Nvidia")
endif(WITH_NVIDIA_TYPO)

file( GLOB _HeadersCoreGeoPrimitives "src/GeoPrimitives/*.h" )
install(FILES ${_HeadersCoreGeoPrimitives} DESTINATION "include/tav/tav_core/GeoPrimitives")

file( GLOB _HeadersCoreGUI "src/GUI/*.h" )
install(FILES ${_HeadersCoreGUI} DESTINATION "include/tav/tav_core/GUI")

file( GLOB _HeadersCoreHeaders "src/headers/*.h" )
install(FILES ${_HeadersCoreHeaders} DESTINATION "include/tav/tav_core/headers")

file( GLOB _HeadersCoreMeshes "src/Meshes/*.h" )
install(FILES ${_HeadersCoreMeshes} DESTINATION "include/tav/tav_core/Meshes")

file( GLOB _HeadersCoreMeshes "src/Meshes/*.h" )
install(FILES ${_HeadersCoreMeshes} DESTINATION "include/tav/tav_core/Meshes")

file( GLOB _HeadersCoreShaders "src/Shaders/*.h" )
install(FILES ${_HeadersCoreShaders} DESTINATION "include/tav/tav_core/Shaders")

file( GLOB _HeadersCoreShaderUtils "src/Shaders/ShaderUtils/*.h" )
install(FILES ${_HeadersCoreShaderUtils} DESTINATION "include/tav/tav_core/Shaders/ShaderUtils")

file( GLOB _HeadersCoreShadowMaps "src/Shaders/ShadowMaps/*.h" )
install(FILES ${_HeadersCoreShadowMaps} DESTINATION "include/tav/tav_core/Shaders/ShadowMaps")

file( GLOB _HeadersCoreTiny "src/tinyObj/*.h" )
install(FILES ${_HeadersCoreTiny} DESTINATION "include/tav/tav_core/tinyObj")
