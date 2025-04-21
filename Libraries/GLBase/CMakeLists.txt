cmake_minimum_required(VERSION 3.10)

project(Libraries)

add_subdirectory(src)

#unit test
if (NOT ANDROID)
    add_subdirectory(UnitTests)
    add_dependencies(GLBase_UnitTests GLBase Utilities)
endif ()
