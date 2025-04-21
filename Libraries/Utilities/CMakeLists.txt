cmake_minimum_required(VERSION 3.10)

add_subdirectory(src)

if (NOT ANDROID)
    add_subdirectory(UnitTests)
    add_dependencies(Utilities_UnitTests Utilities)
endif ()
