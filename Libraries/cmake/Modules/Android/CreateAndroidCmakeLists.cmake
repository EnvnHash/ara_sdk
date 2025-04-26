macro (create_android_cmakelists APP_TYPE ASSETS_FOLDER)

    # create the CMakeList.txt
    list(APPEND ANDROID_CMAKELIST "cmake_minimum_required(VERSION ${CMAKE_VERSION})

project(\"${PACKAGE_NAME}\")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
")

    list(APPEND ANDROID_CMAKELIST "
set(ARA_SDK_SOURCE_DIR ${ARA_SDK_SOURCE_DIR})
set(CMAKE_MODULE_PATH \${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CROSSCOMPILE_FOR_ANDROID ON)

#find_program(CCACHE_FOUND ccache)
#if(CCACHE_FOUND)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
#endif(CCACHE_FOUND)

#control number of parallel builds
if (PARALLEL_COMPILE_JOBS)
  set(CMAKE_JOB_POOL_COMPILE compile_job_pool\${CMAKE_CURRENT_SOURCE_DIR})
  string (REGEX REPLACE \"[^a-zA-Z0-9]+\" \"_\" CMAKE_JOB_POOL_COMPILE \${CMAKE_JOB_POOL_COMPILE})
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS \${CMAKE_JOB_POOL_COMPILE}=\${PARALLEL_COMPILE_JOBS})
endif ()
if (PARALLEL_COMPILE_JOBS)
    message(STATUS \"\${CMAKE_CURRENT_SOURCE_DIR}: Limiting compiler jobs to \${PARALLEL_COMPILE_JOBS}\")
endif ()

")
    if (${APP_TYPE} EQUAL 0)
        list(APPEND ANDROID_CMAKELIST "add_compile_definitions(ARA_ANDROID_PURE_NATIVE_APP)
")
    endif()

    list(APPEND ANDROID_CMAKELIST "include_directories(
    \${CMAKE_SOURCE_DIR}
    \${CMAKE_SOURCE_DIR}/src
    \${ANDROID_NDK}/sources/android/native_app_glue
)
")

    if (${APP_TYPE} EQUAL 0)
        list(APPEND ANDROID_CMAKELIST "
# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS \"\${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate\")
    ")
    endif()

    if (${APP_TYPE} EQUAL 0)
        file(TO_CMAKE_PATH "$ENV{ANDROID_NDK_HOME}" NORM_NDK_HOME_PATH)
        LIST(APPEND ANDROID_CMAKE_SOURCES "native-lib.cpp ${NORM_NDK_HOME_PATH}/sources/android/native_app_glue/android_native_app_glue.c")
    elseif(${APP_TYPE} EQUAL 1)
        LIST(APPEND ANDROID_CMAKE_SOURCES "jni_interface.cpp ")
    endif()

    LIST(APPEND ANDROID_CMAKE_SOURCES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/easywsclient/easywsclient.cpp)

# add all sources from the additional source directories found
    FILE(GLOB_RECURSE sub_dir ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
    FOREACH(src_file ${sub_dir})
        string(FIND ${src_file} ${ANDROID_STUDIO_PROJ} IS_PROJ_SYMLINK)
        if (${IS_PROJ_SYMLINK} EQUAL -1)
            is_excluded_dir(${src_file} is_exl)
            if (NOT ${is_exl})
                LIST(APPEND ANDROID_CMAKE_SOURCES ${src_file})
            endif ()
        endif()
    ENDFOREACH()

    file(READ "${CMAKE_SOURCE_DIR}/CMakeLists.txt" FILE_CONTENT)
    string(REPLACE "\n" ";" line_list ${FILE_CONTENT})
    foreach(line IN LISTS line_list)
        if (NOT line STREQUAL "")
            string(FIND "${line}" "set(ARA_USE_" index)
            if (${index} GREATER -1)
                LIST(APPEND ANDROID_CMAKELIST ${line}\n)
            endif ()
        endif ()
    endforeach ()
    
    LIST(APPEND ANDROID_CMAKELIST "
include(\${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/AraConfigure.cmake)
include(\${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/GLBaseDepInclude.cmake)
include(\${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/UtilitiesDepInclude.cmake)
include(\${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/CreateSymLink.cmake)

#add_subdirectory(Assets)
# copy the sdk to the project as a symbolic link
create_symlink(${ASSETS_FOLDER} \${CMAKE_SOURCE_DIR}/Assets)
include(CMakeRC)

file(GLOB_RECURSE RESOURCES \${CMAKE_SOURCE_DIR}/Assets/resdata/* )
cmrc_add_resource_library(resources ALIAS ara::rc NAMESPACE ara \${RESOURCES})


file(GLOB_RECURSE ARA_SDK_SOURCES
    \${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/*.cpp
    \${ARA_SDK_SOURCE_DIR}/Libraries/SceneGraph/src/*.cpp
    \${ARA_SDK_SOURCE_DIR}/Libraries/UI/src/*.cpp
    \${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/*.cpp
    )

# IMPORTANT NOTE: linking other libs via cmake add_library(.. OBJECT) causes the library to contain double defined method variables !!!!, so adding .cpp files directly here
add_library(\${PROJECT_NAME} SHARED \${ARA_SDK_SOURCES} ")

    foreach(item ${ANDROID_CMAKE_SOURCES})
        if (NOT ${item} STREQUAL "main.cpp")
            list(APPEND ANDROID_CMAKELIST "${item}
            ")
        endif()
    endforeach()

    list(APPEND ANDROID_CMAKELIST ")

include(GLBaseDependencies)
include(UtilitiesDependencies)

")

    if (ARA_USE_ARCORE)
        list(APPEND ANDROID_CMAKELIST "
set_target_properties(\${PROJECT_NAME} PROPERTIES IMPORTED_LOCATION
    \${ARCORE_LIBPATH}/\${ANDROID_ABI}/libarcore_sdk_c.so
    INTERFACE_INCLUDE_DIRECTORIES \${ARCORE_INCLUDE}
)\n")
    endif()

    list(APPEND ANDROID_CMAKELIST "
target_link_libraries(\${PROJECT_NAME} android GLESv1_CM GLESv2 GLESv3 EGL resources log")

    if (ARA_USE_MEDIACODEC)
        list(APPEND ANDROID_CMAKELIST "mediandk OpenMAXAL ")
    endif()
    if (ARA_USE_ARCORE)
        if (${APP_TYPE} EQUAL 0)
            list(APPEND ANDROID_CMAKELIST "\${ARCORE_LIBPATH}/\${ANDROID_ABI}/libarcore_sdk_c.so")
        elseif (${APP_TYPE} EQUAL 1)
            list(APPEND ANDROID_CMAKELIST "\${ARCORE_LIBPATH}/\${ANDROID_ABI}/libarcore_sdk_c.so \${ARCORE_LIBPATH}/\${ANDROID_ABI}/libarcore_sdk_jni.so")
        endif()
    endif()

    list(APPEND ANDROID_CMAKELIST ")
")

    FILE (WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/CMakeLists.txt ${ANDROID_CMAKELIST})
endmacro()
