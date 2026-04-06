cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(UIHelloWorld)

set(SOURCES main.cpp)

if(WIN32 AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug)
	add_executable(${PROJECT_NAME} ${SOURCES})
else()
	add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(TARGET_LIBS ${ara_sdk_LIBRARIES} GLBase SceneGraph UI Utilities)

if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug)
	list(APPEND TARGET_LIBS resources)
endif()

if(WIN32)
	list(APPEND TARGET_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib)
endif()

target_link_libraries (${PROJECT_NAME} ${TARGET_LIBS})

FOREACH(binary ${ara_sdk_BINARIES})
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${binary} ${CMAKE_CURRENT_BINARY_DIR})
ENDFOREACH()

include(ResCompInclude)
