cmake_minimum_required(VERSION 3.9)

project(NativeWindowTest VERSION 0.1.0 DESCRIPTION "NativeWindowTest")

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)

include(VisualStudioSettings)
include(GeneralSettings)
include(CMakeRC)

include_directories(
	${CMAKE_SOURCE_DIR}/Libraries/GLBase
	${CMAKE_SOURCE_DIR}/Libraries/Utilities/src
)

file(GLOB_RECURSE SOURCES "*.cpp")

if (WIN32)
	add_executable(${PROJECT_NAME} ${SOURCES})
else()
	add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(TARGET_LIBS GLBase Utilities)
if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
	list(APPEND TARGET_LIBS resources)
endif()

target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS})

set(GLSG_LIB_DIR ${CMAKE_SOURCE_DIR}/Libraries/third_party/)
include(GLBaseDependencies) # set GLSG_LIB_DIR before!!
include(GLBaseCopyDependencies)
include(UtilitiesDependencies)

INSTALL(TARGETS ${PROJECT_NAME}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)