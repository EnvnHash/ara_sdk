cmake_minimum_required(VERSION 3.10)
project(GLBase VERSION 0.1 LANGUAGES CXX)

#include(VisualStudioSettings)
include(GeneralSettings)
include(AraConfigure)

if (NOT COMPILE_GLSCENEGRAPH_LIB)
    add_compile_definitions(WGLWINDOW)
endif ()

# options
if (WIN32)
    #add_compile_definitions(NOMINMAX)
    #if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    #	string(APPEND CMAKE_CXX_FLAGS " /wd4146")
    #endif ()
    #option (PUGIXML_HEADER_ONLY "Use the header only version of PugiXML (longer compilation time)" ON)
else ()
    option(PUGIXML_HEADER_ONLY "Use the header only version of PugiXML (longer compilation time)" OFF)
endif (WIN32)

include_directories(
        ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
)

file(GLOB_RECURSE GLB_SRC *.cpp)

#header files this is just to have them m_show up in the visual studio project
#file(GLOB_RECURSE GLB_HEADERS *.h)

add_library(${PROJECT_NAME} OBJECT ${GLB_SRC} ${GLB_HEADERS})

if (ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/cmrc/include)
endif ()

include(GLBaseDepInclude)
include(GLBaseDependencies)
