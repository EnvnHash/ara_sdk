cmake_minimum_required(VERSION 3.10)

project(UI)

include(GeneralSettings)
include(AraConfigure)

include_directories(
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party
        ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
)

file(GLOB_RECURSE UI_SRC "*.cpp")

add_library(${PROJECT_NAME} OBJECT ${UI_SRC})

include(GLBaseDepInclude)
include(GLBaseDependencies)
