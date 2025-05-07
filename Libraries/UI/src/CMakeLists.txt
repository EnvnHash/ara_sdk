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

add_library(${PROJECT_NAME} OBJECT ${UI_SRC} UIElements/UINodeBase/UINodeGeom.cpp UIElements/UINodeBase/UINodeGeom.h UIElements/UINodeBase/UINodeStyle.cpp UIElements/UINodeBase/UINodeStyle.h UIElements/UINodeBase/UINode.cpp UIElements/UINodeBase/UINode.h UIElements/UINodeBase/UINodeHID.cpp UIElements/UINodeBase/UINodeHID.h)

include(GLBaseDepInclude)
include(GLBaseDependencies)
