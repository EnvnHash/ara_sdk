cmake_minimum_required(VERSION 3.10)

project(Applications)

if(NOT ANDROID)
   if (NOT COMPILE_GLSCENEGRAPH_LIB)
        add_subdirectory(FFmpeg)
        add_subdirectory(OpenCV)
        if (ARA_USE_REALSENSE)
            add_subdirectory(Realsense2)
        endif()
   endif ()
   add_subdirectory(UISDK_Dev)

    if (NOT APPLE AND ARA_USE_NDI)
        add_subdirectory(NDI)
    endif()

    if (NOT BUILD_UNVUE_OSX_BUNDLE)
        #add_subdirectory(GreenScreen_Proj)

        if (WIN32 AND ARA_USE_OBS)
      #      add_subdirectory(LibObsTest)
      #      add_dependencies(LibObsTest GLBase GLSceneGraph VideoInput)
        endif()
   endif()

else ()
    add_subdirectory(ARCore_Test)
    add_dependencies(ARCore_Test Utilities GLBase GLSceneGraph VideoInput)
endif()