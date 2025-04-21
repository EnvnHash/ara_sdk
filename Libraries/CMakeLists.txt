cmake_minimum_required(VERSION 3.10)

project(Libraries)

set(REQUIRED_THIRD_PARTY_DIRS
     "libyuv"
)
if (NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "GLEW")
endif ()

if (ARA_USE_ASSIMP AND NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "assimp")
endif()

if (ARA_USE_CURL)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "curl")
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "openssl")
endif()

if (ARA_USE_FREEIMAGE AND NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "FreeImage")
endif()

if (ARA_USE_NDI)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "NDI")
endif()

if (ARA_USE_OPENAL)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "OpenAL")
endif()

if (ARA_USE_FFMPEG AND NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "ffmpeg")
endif()

if (ARA_USE_REALSENSE)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "intel_realsense")
endif()

if (ARA_USE_OBS)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "obs")
endif()

if (ARA_USE_OPENCV)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "opencv")
endif()

if (ARA_USE_PORTAUDIO AND NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "portaudio")
endif()

# Main script logic
find_program(UNZIP_EXECUTABLE NAMES unzip)

foreach(DIR IN LISTS REQUIRED_THIRD_PARTY_DIRS)
    if(NOT EXISTS "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout")
        file(MAKE_DIRECTORY ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout)
    endif()

    if(NOT EXISTS "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout/${DIR}.tar.gz")
        message(STATUS "\t downloading ${DIR}")

        file(DOWNLOAD
                ftp://ara_ftp:JcjUIUYcr74x@zeitkunst.eu/${DIR}.tar.gz
                "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout/${DIR}.tar.gz"
                SHOW_PROGRESS
        )

        message(STATUS ${error})
    endif ()

    if(NOT EXISTS "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/${DIR}")
        execute_process(
            COMMAND tar -xvf ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout/${DIR}.tar.gz
            WORKING_DIRECTORY ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/
            RESULT_VARIABLE result
        )
    endif()
endforeach()

add_subdirectory(GLBase)
if(ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    add_dependencies(GLBase resources)
endif()

add_subdirectory(SceneGraph)
add_subdirectory(UI)
add_subdirectory(Utilities)
