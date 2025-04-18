cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
CMAKE_POLICY(SET CMP0071 NEW)
set(OpenGL_GL_PREFERENCE "LEGACY")
set(CMAKE_CXX_STANDARD 17)

include(GeneralSettings)
include(VisualStudioSettings)

project(UITest)

include_directories(
	${CMAKE_SOURCE_DIR}/Libraries/Utilities/src
	)

set(SOURCES main.cpp UITestApp.cpp)

file(GLOB DEMO_SOURCES "Demo/*.h" "Demo/*.cpp")

list(APPEND ALLSOURCES ${SOURCES} ${DEMO_SOURCES})

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

include(GLBaseDependencies)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
	set(LIB_SUFFIX "d")
endif ()

if (COMPILE_GLSCENEGRAPH_LIB)
	set(TARGET_LIBS ${CMAKE_BINARY_DIR}/Libraries/GLBase/src/libGLBase.so
		${CMAKE_BINARY_DIR}/Libraries/GLSceneGraph/src/libGLSceneGraph${LIB_SUFFIX}.so
		${CMAKE_BINARY_DIR}/Libraries/Utilities/src/libUtilities.so)
else()
	set(TARGET_LIBS GLBase GLSceneGraph Utilities)
endif()

if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
	list(APPEND TARGET_LIBS resources)
endif()
if(WIN32)
	list(APPEND TARGET_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib Dwmapi.lib)
endif()

target_link_libraries (${PROJECT_NAME} ${TARGET_LIBS})

include(GLBaseDepInclude)
include(GLBaseCopyDependencies)
include(Gtk3Include)
include(ResCompInclude)
include(PugiXMLInclude)
include(UtilitiesDepInclude)
include(UtilitiesDependencies)

#gen_android_proj(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)