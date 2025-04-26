cmake_minimum_required(VERSION 3.31.6)

project("UITest")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ARA_SDK_SOURCE_DIR /home/sven/local/github/ara_sdk)
set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CROSSCOMPILE_FOR_ANDROID ON)

#find_program(CCACHE_FOUND ccache)
#if(CCACHE_FOUND)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
#endif(CCACHE_FOUND)

#control number of parallel builds
if (PARALLEL_COMPILE_JOBS)
  set(CMAKE_JOB_POOL_COMPILE compile_job_pool${CMAKE_CURRENT_SOURCE_DIR})
  string (REGEX REPLACE "[^a-zA-Z0-9]+" "_" CMAKE_JOB_POOL_COMPILE ${CMAKE_JOB_POOL_COMPILE})
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS ${CMAKE_JOB_POOL_COMPILE}=${PARALLEL_COMPILE_JOBS})
endif ()
if (PARALLEL_COMPILE_JOBS)
    message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: Limiting compiler jobs to ${PARALLEL_COMPILE_JOBS}")
endif ()

add_compile_definitions(ARA_ANDROID_PURE_NATIVE_APP)
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    
include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/AraConfigure.cmake)
include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/GLBaseDepInclude.cmake)
include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/UtilitiesDepInclude.cmake)
include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/CreateSymLink.cmake)

#add_subdirectory(Assets)
# copy the sdk to the project as a symbolic link
create_symlink(/home/sven/local/github/ara_sdk/Assets ${CMAKE_SOURCE_DIR}/Assets)
include(CMakeRC)

file(GLOB_RECURSE RESOURCES ${CMAKE_SOURCE_DIR}/Assets/resdata/* )
cmrc_add_resource_library(resources ALIAS ara::rc NAMESPACE ara ${RESOURCES})


file(GLOB_RECURSE ARA_SDK_SOURCES
    ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/*.cpp
    ${ARA_SDK_SOURCE_DIR}/Libraries/SceneGraph/src/*.cpp
    ${ARA_SDK_SOURCE_DIR}/Libraries/UI/src/*.cpp
    ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/*.cpp
    )

# IMPORTANT NOTE: linking other libs via cmake add_library(.. OBJECT) causes the library to contain double defined method variables !!!!, so adding .cpp files directly here
add_library(${PROJECT_NAME} SHARED ${ARA_SDK_SOURCES} native-lib.cpp /mnt/data/Applications/Android/Sdk/ndk/29.0.13113456/sources/android/native_app_glue/android_native_app_glue.c
            /home/sven/local/github/ara_sdk/Libraries/third_party/easywsclient/easywsclient.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_ComboBox.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_Edit.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_FloatingMenu.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_Resources.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_ScrollView.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_ScrollViewTable.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_ScrollView_2.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_ScrollView_3.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_Spinner.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_Table.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/Demo/DemoView_Table_2.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/UITestApp.cpp
            /home/sven/local/github/ara_sdk/Examples/UITest/main.cpp
            )

include(GLBaseDependencies)
include(UtilitiesDependencies)


target_link_libraries(${PROJECT_NAME} android GLESv1_CM GLESv2 GLESv3 EGL resources log)
