cmake_minimum_required (VERSION 2.6)
project (tav)

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
SET(GXX_COVERAGE_COMPILE_FLAGS "-Wno-deprecated -Wno-deprecated-declarations -Wno-subobject-linkage")
add_definitions(${GXX_COVERAGE_COMPILE_FLAGS})

# cotire doesn't user ffmpeg precompiled header if this flag isnt set 
add_definitions(-D__DEPRECATED)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Copy files from source directory to destination directory, substituting any
# variables.  Create destination directory if it does not exist.
macro(CopyDir srcDir destDir)
  message(STATUS "Configuring directory ${destDir}")
  make_directory(${destDir})
  file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
  foreach(templateFile ${templateFiles})
    set(srcTemplatePath ${srcDir}/${templateFile})
    if(NOT IS_DIRECTORY ${srcTemplatePath})
      message(STATUS "Configuring file ${templateFile}")
      configure_file(
	${srcTemplatePath}
        ${destDir}/${templateFile}
	@ONLY)
    endif(NOT IS_DIRECTORY ${srcTemplatePath})
  endforeach(templateFile)
endmacro(CopyDir)

option (BUILD_STATIC "Build Static Library" OFF)
option (WITH_AUDIO "Build Tav Audio Module" ON)
option (WITH_CUDA "Use Cuda" ON)
option (WITH_DLIB "Use libdlib" ON)
option (WITH_FLEX "Use Nvidia Flex Physical Simulation" OFF)
option (WITH_FREENECT2 "Use libfreenect2" ON)
option (WITH_GSTREAMER "Build with GStreamer" OFF)
option (WITH_MUSICXML "Use MusicXml" ON)
option (WITH_NITE2 "Use Nite2" ON)
option (WITH_NVIDIA_TYPO "Use Nvidia font rendering support" ON)
option (WITH_NVVIDEO "Build with NVIDIA Video SDK Decoder" OFF)
option (WITH_OPENBLAS "Use libopenblas" ON)
option (WITH_OPENCV "Use OpenCV" ON)
option (WITH_OPENNI2 "Use OpenNI2" ON)
option (WITH_TRACKING "Build Tav Tracking Module" ON)
option (WITH_VIDEO "Build Tav Video Module" ON)
option (WITH_SN_AUDIO "Build basic audio SceneNodes" OFF)
option (WITH_SN_TESTING "Build testing SceneNodes" ON)
option (WITH_SN_TRACKING "Build basic tracking SceneNodes" OFF)
option (WITH_SN_VIDEO "Build basic video SceneNodes" OFF)
option (WITH_ACTRANGE_TRACKING "Use the ActiveRange Video Tracking Module" ON)

if (WITH_ACTRANGE_TRACKING)
	add_definitions(-DUSE_ACTRANGE_TRACKING)
endif(WITH_ACTRANGE_TRACKING)

if(BUILD_STATIC)
#SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#SET(BUILD_SHARED_LIBS OFF)
#SET(CMAKE_EXE_LINKER_FLAGS "-static")
endif(BUILD_STATIC)

# add the binary tree to the search path for include files
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src")
if(WITH_AUDIO)
  add_definitions(-DWITH_AUDIO)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_audio/src")
endif(WITH_AUDIO)
if(WITH_TRACKING)
  add_definitions(-DWITH_TRACKING)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_tracking/src")
endif(WITH_TRACKING)
if(WITH_VIDEO)
  add_definitions(-DWITH_VIDEO)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_video/src")
endif(WITH_VIDEO)
if(WITH_SN_AUDIO)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_audio/src")
endif(WITH_SN_AUDIO)
if(WITH_SN_VIDEO)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_video/src")
endif(WITH_SN_VIDEO)
if(WITH_SN_TESTING)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_testing/src")
endif(WITH_SN_TESTING)
if(WITH_SN_TRACKING)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_tracking/src")
endif(WITH_SN_TRACKING)

file(GLOB tav_SRC "src/*.cpp")
file(GLOB tav_nodes_SRC "src/SceneNodes/*.cpp")
file(GLOB tav_sequencer_SRC "src/Sequencer/*.cpp")
file(GLOB tav_setupman_SRC "src/SetupManagement/*.cpp")

set(Boost_USE_STATIC_LIBS OFF) 

include_directories(${tav_BINARY_DIR})

# generate the SceneFactory Header File
# init variables
set(SN_FACT_SN_PROJ_ABSOLUT_SRC_FILES "")
set(SN_FACT_SN_PROJ_CAMCHAL_SRC_FILES "")
set(SN_FACT_SN_PROJ_ENEL_SRC_FILES "")
set(SN_FACT_SN_PROJ_IKPI_SRC_FILES "")
set(SN_FACT_SN_PROJ_GAM_SRC_FILES "")
set(SN_FACT_SN_PROJ_TRUST_SRC_FILES "")
set(SN_FACT_SN_AUDIO_SRC_FILES "")
set(SN_FACT_SN_COMP_SRC_FILES "")
set(SN_FACT_SN_TEST_SRC_FILES "")
set(SN_FACT_SN_TRACK_SRC_FILES "")
set(SN_FACT_SN_VIDEO_SRC_FILES "")

file(GLOB SN_FACT_CORE_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_core/src/SceneNodes/*.h")

if(WITH_AUDIO AND WITH_SN_AUDIO)
  file(GLOB_RECURSE SN_FACT_SN_AUDIO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_audio/src/*.h")
endif(WITH_AUDIO AND WITH_SN_AUDIO)

if(WITH_TRACKING AND WITH_SN_TRACKING)
  file(GLOB_RECURSE SN_FACT_SN_TRACK_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_tracking/src/*.h")
endif(WITH_TRACKING AND WITH_SN_TRACKING)

if(WITH_VIDEO AND WITH_SN_VIDEO)
  file(GLOB_RECURSE SN_FACT_SN_VIDEO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_video/src/*.h")
endif(WITH_VIDEO AND WITH_SN_VIDEO)

if (NOT WITH_AUDIO)
  list(REMOVE_ITEM SN_FACT_SN_VIDEO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_video/src/SNAudioRingV.h" "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_video/src/SNVideoTileAudio.h" "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_video/src/SNVideoTileAudio3d.h")
endif (NOT WITH_AUDIO)


# TAV_SN_TESTING
if(WITH_SN_TESTING)
	file(GLOB_RECURSE SN_FACT_SN_TEST_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_testing/src/*.h")
	if (NOT WITH_OPENNI2)
		file(GLOB_RECURSE SN_FACT_SN_TEST_KINECT_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_testing/src/kinect/*.h")
  		list(REMOVE_ITEM SN_FACT_SN_TEST_SRC_FILES ${SN_FACT_SN_TEST_KINECT_SRC_FILES})
	endif (NOT WITH_OPENNI2)
	if (NOT WITH_FLEX)
		file(GLOB_RECURSE SN_FACT_SN_TEST_FLEX_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_testing/src/flex/*.h")
	  	list(REMOVE_ITEM SN_FACT_SN_TEST_SRC_FILES ${SN_FACT_SN_TEST_FLEX_SRC_FILES})
	endif(NOT WITH_FLEX)
	if (NOT WITH_VIDEO)
		file(GLOB_RECURSE SN_FACT_SN_TEST_VIDEO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tav_sn_testing/src/video/*.h")
	  	list(REMOVE_ITEM SN_FACT_SN_TEST_SRC_FILES ${SN_FACT_SN_TEST_VIDEO_SRC_FILES})
	endif(NOT WITH_VIDEO)
endif(WITH_SN_TESTING)



set(SN_FACT_SRC_FILES ${SN_FACT_CORE_SRC_FILES} ${SN_FACT_SN_AUDIO_SRC_FILES} ${SN_FACT_SN_COMP_SRC_FILES} ${SN_FACT_SN_TEST_SRC_FILES} 
	${SN_FACT_SN_TRACK_SRC_FILES} ${SN_FACT_SN_VIDEO_SRC_FILES} ${SN_FACT_SN_PROJ_ABSOLUT_SRC_FILES} 
	${SN_FACT_SN_PROJ_CAMCHAL_SRC_FILES} ${SN_FACT_SN_PROJ_ENEL_SRC_FILES} ${SN_FACT_SN_PROJ_IKPI_SRC_FILES} ${SN_FACT_SN_PROJ_GAM_SRC_FILES}
	${SN_FACT_SN_PROJ_TRUST_SRC_FILES})
set(SNFACT_H_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SceneNodeFact.h)

add_custom_command(
  OUTPUT  ${SNFACT_H_OUTPUT}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/genSceneNodeFactH.sh ${SNFACT_H_OUTPUT} ${SN_FACT_SRC_FILES}
  DEPENDS ${SN_FACT_SRC_FILES}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# now create an executable using the generated files

set(TAV_ALL_SRC ${tav_SRC} ${tav_sequencer_SRC} ${tav_setupman_SRC} ${tav_nodes_SRC} ${SNFACT_H_OUTPUT})
add_executable(tav ${TAV_ALL_SRC})

if(BUILD_STATIC)
  set(LIB_LINK_OPT, "-static")
else()
  set(LIB_LINK_OPT, "")
endif(BUILD_STATIC)

#cotire(tav)


find_package (assimp REQUIRED)
if(WITH_CUDA)
  find_package (CUDA)
  find_package (Flex)
endif(WITH_CUDA)
find_package (Boost 1.55 REQUIRED COMPONENTS filesystem timer thread regex system)
if(WITH_DLIB)
  find_package (dlib REQUIRED)
endif(WITH_DLIB)
find_package (FreeImage REQUIRED)
find_package (Freetype REQUIRED)
find_package (FFmpeg REQUIRED)
if (WITH_GSTREAMER)
  find_package (GStreamer)
endif (WITH_GSTREAMER)
if(WITH_FREENECT2)
  find_package (Freenect2 REQUIRED)
endif(WITH_FREENECT2)
find_package (GLFW REQUIRED)
find_package (GLEW REQUIRED)
find_package (GLM REQUIRED)
find_package (HarfBuzz REQUIRED)
find_package (LibXML++ REQUIRED)
find_package (liblo REQUIRED)
if(WITH_OPENBLAS)
  find_package (OpenBLAS REQUIRED)
endif(WITH_OPENBLAS)
if(WITH_OPENCV)
  find_package (OpenCV REQUIRED)
endif(WITH_OPENCV)
if(WITH_OPENNI2)
  find_package (OpenNI2 REQUIRED)
endif(WITH_OPENNI2)
find_package (OpenGL REQUIRED)
if(WITH_MUSICXML)
  find_package (MusicXml REQUIRED)
endif(WITH_MUSICXML)
if(WITH_NITE2)
  find_package (NITE2 REQUIRED)
endif(WITH_NITE2)
find_package (UCDN)

# assimp
if (ASSIMP_FOUND)
  include_directories(${ASSIMP_INCLUDE_DIRS})
  target_link_libraries (tav ${ASSIMP_LIBRARIES} ${LIB_LINK_OPT})
endif (ASSIMP_FOUND)

# Blas
if (WITH_OPENBLAS AND OpenBLAS_FOUND)
  add_definitions(-DHAVE_OPENBLAS)
  include_directories(${BLAS_INCLUDE_DIRS})
  target_link_libraries (tav ${OpenBLAS_LIB} ${LIB_LINK_OPT})
endif (WITH_OPENBLAS AND OpenBLAS_FOUND)

# Boost
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries (tav ${Boost_LIBRARIES} ${LIB_LINK_OPT})
endif (Boost_FOUND)

# NVIDIA Video Codec SDK
if (WITH_NVVIDEO)
	add_definitions(-DINIT_CUDA_GL=1)
	add_definitions(-DHAVE_NVVIDEO)
endif (WITH_NVVIDEO)
	
# Cuda
if (WITH_CUDA AND CUDA_FOUND)
  add_definitions(-DHAVE_CUDA)
  include_directories(${CUDA_INCLUDE_DIRS})
  target_link_libraries (tav ${CUDA_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_CUDA AND CUDA_FOUND)

# Dlib
if (WITH_DLIB AND DLIB_FOUND)
  add_definitions(-DHAVE_DLIB)
  include_directories(${DLIB_INCLUDE_DIRS})
  target_link_libraries (tav ${DLIB_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_DLIB AND DLIB_FOUND)

# Freeimage
if (FREEIMAGE_FOUND)
  include_directories(${FREEIMAGE_INCLUDE_DIRS})
  target_link_libraries (tav ${FREEIMAGE_LIBRARIES} ${LIB_LINK_OPT})
endif (FREEIMAGE_FOUND)

# Flex
if (WITH_CUDA AND FLEX_FOUND)
  include_directories(${FLEX_INCLUDE_DIRS})
  target_link_libraries (tav ${FLEX_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_CUDA AND FLEX_FOUND)

if (FREETYPE_FOUND)
  include_directories(${FREETYPE_INCLUDE_DIRS})
  target_link_libraries (tav ${FREETYPE_LIBRARIES} ${LIB_LINK_OPT})
endif (FREETYPE_FOUND)

# Freenect2
if (WITH_FREENECT2 AND FREENECT2_FOUND)
  add_definitions(-DHAVE_FREENECT2)
  include_directories(${FREENECT2_INCLUDE_DIRS})
  target_link_libraries(tav ${FREENECT2_LIBRARY} ${LIB_LINK_OPT})
endif (WITH_FREENECT2 AND FREENECT2_FOUND)

# FFMPEG
if (FFMPEG_FOUND)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  # if we are linking statically we need the static libraries of ffmpeg
  #if(BUILD_STATIC)
  #  STRING(REGEX REPLACE ".so" ".a" FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES})
  #endif(BUILD_STATIC)
  target_link_libraries(tav ${FFMPEG_LIBRARIES} ${LIB_LINK_OPT})
endif (FFMPEG_FOUND)

# GLEW
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIRS})
  target_link_libraries (tav ${GLEW_LIBRARIES} ${LIB_LINK_OPT})
endif (GLEW_FOUND)

# GLFW
if (GLFW_FOUND)
  # tell GLFW to not include gl headers
  add_definitions(-DGLFW_INCLUDE_NONE)
  include_directories(${GLFW_INCLUDE_DIRS})
  target_link_libraries (tav ${GLFW_LIBRARIES} ${LIB_LINK_OPT})
endif (GLFW_FOUND)

# glm
if (GLM_FOUND)
  include_directories(${GLM_INCLUDE_DIRS})
endif (GLM_FOUND)

# GSTREAMER
if (WITH_GSTREAMER AND GSTREAMER_FOUND)
  add_definitions(-DHAVE_GSTREAMER)
  include_directories(${GSTREAMER_INCLUDE_DIRS})
  target_link_libraries (tav ${GSTREAMER_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_GSTREAMER AND GSTREAMER_FOUND)

# HarfBuzz
if (HARFBUZZ_FOUND)
  include_directories(${HARFBUZZ_INCLUDE_DIRS})
  target_link_libraries (tav ${HARFBUZZ_LIBRARIES} ${LIB_LINK_OPT})
endif (HARFBUZZ_FOUND)

if (LIBLO_FOUND)
  include_directories(${LIBLO_INCLUDE_DIRS})
  target_link_libraries (tav ${LIBLO_LIBRARIES} ${LIB_LINK_OPT})
endif (LIBLO_FOUND)

# LibXML
if (LibXML++_FOUND)
  include_directories(${LibXML++_INCLUDE_DIRS})
  target_link_libraries (tav ${LibXML++_LIBRARIES} ${LIB_LINK_OPT})
endif (LibXML++_FOUND)

if (WITH_OPENBLAS AND OpenBLAS_FOUND)
  add_definitions(-DHAVE_OPENBLAS)
  include_directories(${OpenBLAS_INCLUDE_DIRS})
  target_link_libraries (tav ${OpenBLAS_LIB} ${LIB_LINK_OPT})
endif (WITH_OPENBLAS AND OpenBLAS_FOUND)

# OpenGL
if (OpenGL_FOUND)
  include_directories(${OPENGL_INCLUDE_DIRS})
  target_link_libraries (tav ${OPENGL_LIBRARIES} ${LIB_LINK_OPT})
endif (OpenGL_FOUND)

# OpenGL
if (WITH_OPENCV AND OpenCV_FOUND)
  add_definitions(-DHAVE_OPENCV)
  include_directories(${OpenCV_INCLUDE_DIRS})
  target_link_libraries (tav ${OpenCV_LIBRARIES} ${LIB_LINK_OPT})
endif(WITH_OPENCV AND OpenCV_FOUND)

#MusicXml
if (WITH_MUSICXML AND MusicXml_FOUND)
  add_definitions(-DHAVE_MUSICXML)
  include_directories(${MUSICXML_INCLUDE_DIRS})
  target_link_libraries (tav ${MUSICXML_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_MUSICXML AND MusicXml_FOUND)

# NITE2
if (WITH_NITE2 AND NITE2_FOUND)
  add_definitions(-DHAVE_NITE2)
  include_directories(${NITE2_INCLUDE_DIRS})
  target_link_libraries(tav ${NITE2_LIBRARIES} ${LIB_LINK_OPT})
  file(COPY ${NITE2_INI_DIR}/NiTE.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif (WITH_NITE2 AND NITE2_FOUND)

# OpenNI2
if (WITH_OPENNI2 AND OPENNI2_FOUND)
  add_definitions(-DHAVE_OPENNI2)
  include_directories(${OPENNI2_INCLUDE_DIRS})
  target_link_libraries(tav ${OPENNI2_LIBRARIES} ${LIB_LINK_OPT})
endif (WITH_OPENNI2 AND OPENNI2_FOUND)

# UCDN
if (UCDN_FOUND)
  include_directories(${UCDN_INCLUDE_DIRS})
  target_link_libraries(tav ${UCDN_LIBRARY} ${LIB_LINK_OPT})
endif (UCDN_FOUND)

add_dependencies(tav tav_core)


if(BUILD_STATIC)
  set(LIB_SUFFIX ".a")
else()
  set(LIB_SUFFIX ".so")
endif(BUILD_STATIC)

target_link_libraries(tav ${tav_BINARY_DIR}/../tav_core/libtav_core${LIB_SUFFIX})
if(WITH_AUDIO)
  add_dependencies(tav tav_audio)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_audio/libtav_audio${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_AUDIO)
if(WITH_VIDEO)
  add_dependencies(tav tav_video)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_video/libtav_video${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_VIDEO)
if(WITH_TRACKING)
  add_dependencies(tav tav_tracking)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_tracking/libtav_tracking${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_TRACKING)
if(WITH_SN_AUDIO)
  add_dependencies(tav tav_sn_audio)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_sn_audio/libtav_sn_audio${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_SN_AUDIO)
if(WITH_SN_TESTING)
  add_dependencies(tav tav_sn_testing)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_sn_testing/libtav_sn_testing${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_SN_TESTING)
if(WITH_SN_TRACKING)
  add_dependencies(tav tav_sn_tracking)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_sn_tracking/libtav_sn_tracking${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_SN_TRACKING)
if(WITH_SN_VIDEO)
  add_dependencies(tav tav_sn_video)
  target_link_libraries(tav ${tav_BINARY_DIR}/../tav_sn_video/libtav_sn_video${LIB_SUFFIX} ${LIB_LINK_OPT})
endif(WITH_SN_VIDEO)
