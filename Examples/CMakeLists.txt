cmake_minimum_required(VERSION 3.10)

project(Applications)

add_subdirectory(JsonEditor)
add_subdirectory(UIElementsOverview)
add_subdirectory(UIHelloWorld)

if(ANDROID)
    add_subdirectory(ARCore_Test)
    add_dependencies(ARCore_Test Utilities GLBase SceneGraph UI)
endif()