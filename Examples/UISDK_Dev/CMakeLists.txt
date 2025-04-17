cmake_minimum_required(VERSION 3.10)

project(UISDK_dev)

if (NOT COMPILE_GLSCENEGRAPH_LIB)
    add_subdirectory(UI_NDI_Test)
    add_dependencies(UI_NDI_Test Utilities GLBase GLSceneGraph VideoInput)
endif()

if (NOT ANDROID)
    if (NOT COMPILE_GLSCENEGRAPH_LIB)
        add_subdirectory(Stage3DTest)
        add_dependencies(Stage3DTest Utilities GLBase GLSceneGraph)

        add_subdirectory(Stage3DStereoTest)
        add_dependencies(Stage3DStereoTest Utilities GLBase GLSceneGraph)

        if (NOT BUILD_UNVUE_OSX_BUNDLE)
            if(WIN32)
                add_subdirectory(NativeWindowTest)
                add_dependencies(NativeWindowTest GLBase)
            endif()

            #add_subdirectory(WarpingToolTest)
            #add_dependencies(WarpingToolTest Utilities GLBase GLSceneGraph)

            #add_subdirectory(MaskingToolTest)
            #if(WIN32)
            #    if ("${CMAKE_BUILD_TYPE}" STREQUAL RELEASE OR "${CMAKE_BUILD_TYPE}" STREQUAL RELWITHDEBUGINFO)
            #        add_dependencies(MaskingToolTest Utilities GLBase GLSceneGraph ResComp)
            #    else()
            #        add_dependencies(MaskingToolTest Utilities GLBase GLSceneGraph)
            #    endif()
            #else ()
            #    add_dependencies(MaskingToolTest Utilities GLBase GLSceneGraph)
            #endif()

            if(WIN32)
                add_subdirectory(MFCApplication)
                add_dependencies(MFCApplication Utilities GLBase GLSceneGraph)
            endif()
        endif()

        add_subdirectory(UITest)
        add_dependencies(UITest GLBase GLSceneGraph Utilities)
    endif()

    add_subdirectory(UISDK_lib_test)
endif()