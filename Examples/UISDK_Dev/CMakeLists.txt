cmake_minimum_required(VERSION 3.10)

project(UISDK_dev)

if (NOT ANDROID)
    if (NOT COMPILE_SCENEGRAPH_LIB)
        add_subdirectory(UITest)
        add_dependencies(UITest GLBase SceneGraph Utilities)
    endif()
endif()