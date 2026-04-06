cmake_minimum_required(VERSION 3.10)

project(Applications)

add_subdirectory(JsonEditor)
add_subdirectory(UITest)
add_subdirectory(UIElementsOverview)
add_subdirectory(UIHelloWorld)
add_subdirectory(UIPropertyExample)

if(ANDROID)
    add_subdirectory(ARCore_Test)
    add_dependencies(ARCore_Test Utilities GLBase SceneGraph UI)
endif()