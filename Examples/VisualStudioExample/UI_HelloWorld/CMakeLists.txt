cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(UI_HelloWorld)

include(AraSdk)
include_directories(${ara_sdk_INCLUDE_DIRS})

set(SOURCES main.cpp)

list(APPEND ALLSOURCES ${SOURCES})

if (ANDROID)
	add_library(${PROJECT_NAME} STATIC ${ALLSOURCES})
else()
	if(WIN32 AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug)
		add_executable(${PROJECT_NAME} ${ALLSOURCES})
		#add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
	else()
		add_executable(${PROJECT_NAME} ${ALLSOURCES})
	endif()
endif()

set(TARGET_LIBS ${ara_sdk_LIBRARIES})

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
	set(LIB_SUFFIX "d")
endif ()

if (COMPILE_SCENEGRAPH_LIB)
	list(APPEND TARGET_LIBS ${CMAKE_BINARY_DIR}/Libraries/GLBase/src/libGLBase.so
		${CMAKE_BINARY_DIR}/Libraries/SceneGraph/src/libSceneGraph${LIB_SUFFIX}.so
		${CMAKE_BINARY_DIR}/Libraries/Utilities/src/libUtilities.so)
else()
	list(APPEND TARGET_LIBS GLBase SceneGraph UI Utilities)
endif()

if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
	list(APPEND TARGET_LIBS resources)
endif()
if(WIN32)
	list(APPEND TARGET_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib)
endif()

target_link_libraries (${PROJECT_NAME} ${TARGET_LIBS})


# uncomment for generating android project
#include(Android/GenerateAndroidProject)
#gen_android_proj(${PROJECT_NAME} "'armeabi-v7a', 'arm64-v8a'" 0 icon ${CMAKE_SOURCE_DIR}/Assets)
