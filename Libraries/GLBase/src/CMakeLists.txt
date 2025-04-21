cmake_minimum_required(VERSION 3.10)
project(GLBase VERSION 0.1 LANGUAGES CXX)

include(GeneralSettings)
include(AraConfigure)

if (NOT COMPILE_SCENEGRAPH_LIB)
    add_compile_definitions(WGLWINDOW)
endif ()


include_directories(
        ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
)

file(GLOB_RECURSE GLB_SRC *.cpp)

add_library(${PROJECT_NAME} OBJECT ${GLB_SRC} ${GLB_HEADERS})

include(GLBaseDepInclude)
include(GLBaseDependencies)
