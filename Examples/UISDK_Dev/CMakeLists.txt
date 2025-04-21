cmake_minimum_required(VERSION 3.10)

project(UISDK_dev)

if (NOT ANDROID)
    if (NOT COMPILE_SCENEGRAPH_LIB)
        if(WIN32)
            add_subdirectory(NativeWindowTest)
            add_dependencies(NativeWindowTest GLBase)
        endif()

        if(WIN32)
            add_subdirectory(MFCApplication)
            add_dependencies(MFCApplication Utilities GLBase SceneGraph)
        endif()

        add_subdirectory(UITest)
        add_dependencies(UITest GLBase SceneGraph Utilities)
    endif()
endif()