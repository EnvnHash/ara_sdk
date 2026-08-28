macro (create_android_cmakelists
        #APP_TYPE ASSETS_FOLDER
)

    # create the CMakeList.txt
    list(APPEND ANDROID_CMAKELIST "cmake_minimum_required(VERSION ${DST_CMAKE_VERSION})

project(${PROJECT_NAME})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
")

    list(APPEND ANDROID_CMAKELIST "
set(ARA_SDK_SOURCE_DIR ${ARA_SDK_SOURCE_DIR})
set(CMAKE_MODULE_PATH \${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CROSSCOMPILE_FOR_ANDROID ON)

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

    get_property(APP_TYPE TARGET ${PROJECT_NAME} PROPERTY ARASDK_APP_TYPE)
    if (${APP_TYPE} EQUAL 0)
        list(APPEND ANDROID_CMAKELIST "add_compile_definitions(ARA_ANDROID_PURE_NATIVE_APP)
")
    endif()

    list(APPEND ANDROID_CMAKELIST "include_directories(
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
")

    LIST(APPEND ANDROID_CMAKELIST "add_subdirectory(\${CMAKE_SOURCE_DIR}/Assets)
")

    file(GLOB deps_sub_dir ${FETCHCONTENT_BASE_DIR}/ara_sdk*-src)
    foreach(dir IN LISTS deps_sub_dir)
        LIST(APPEND ANDROID_CMAKELIST "add_subdirectory(${dir} \${CMAKE_BINARY_DIR}/${dir})
")
    endforeach ()

    LIST(APPEND ANDROID_CMAKELIST "
# IMPORTANT NOTE: linking other libs via cmake add_library(.. OBJECT) causes the library to contain double defined method variables !!!!, so adding .cpp files directly here
add_library(${PROJECT_NAME} SHARED ")

    foreach(item ${ANDROID_CMAKE_SOURCES})
        if (NOT ${item} STREQUAL "main.cpp")
            list(APPEND ANDROID_CMAKELIST "${item}
            ")
        endif()
    endforeach()

    list(APPEND ANDROID_CMAKELIST ")
")

    if (ARA_USE_ARCORE)
        list(APPEND ANDROID_CMAKELIST "
set_target_properties(${PROJECT_NAME} PROPERTIES IMPORTED_LOCATION
    \${ARCORE_LIBPATH}/\${ANDROID_ABI}/libarcore_sdk_c.so
    INTERFACE_INCLUDE_DIRECTORIES \${ARCORE_INCLUDE}
)\n")
    endif()

    list(APPEND ANDROID_CMAKELIST "
target_link_libraries(${PROJECT_NAME} android GLESv1_CM GLESv2 GLESv3 EGL resources log")

    # link all ara::* libraries linked to the parent project
    get_target_property(linked_libs ${PROJECT_NAME} LINK_LIBRARIES)
    if(linked_libs)
        foreach(lib IN LISTS linked_libs)
            if("${lib}" MATCHES "^ara::(.*)")
                list(APPEND ANDROID_CMAKELIST " ${lib}")
            endif ()
        endforeach()
    endif()

    # link all libs from ara_sdk*LIBRARIES variables included into the parent project
    get_cmake_property(vars VARIABLES)
    foreach(var ${vars})
        if("${var}" MATCHES "^ara_sdk(.*)LIBRARIES")
            list(APPEND ANDROID_CMAKELIST " \${${var}}")
        endif()
    endforeach()

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
