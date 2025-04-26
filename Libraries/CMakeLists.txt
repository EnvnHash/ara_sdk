cmake_minimum_required(VERSION 3.10)

project(Libraries)

set(REQUIRED_THIRD_PARTY_DIRS )

if (NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "GLEW")
endif ()

if (ARA_USE_ASSIMP)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "assimp")
endif()

if (ARA_USE_CURL)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "curl")
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "openssl")
endif()

if (ARA_USE_FREEIMAGE)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "FreeImage")
endif()

if (ARA_USE_OPENAL)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "OpenAL")
endif()

if (ARA_USE_PORTAUDIO AND NOT UNIX)
    LIST(APPEND REQUIRED_THIRD_PARTY_DIRS "portaudio")
endif()

# Main script logic
find_program(UNZIP_EXECUTABLE NAMES unzip)

if(NOT EXISTS "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout")
    file(MAKE_DIRECTORY ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout)
endif()

foreach(DIR IN LISTS REQUIRED_THIRD_PARTY_DIRS)
    if (NOT EXISTS "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout/${DIR}.tar.gz")
        message(STATUS "\t downloading ${DIR}")

        file(DOWNLOAD
                ftp://ara_ftp:JcjUIUYcr74x@zeitkunst.eu/${DIR}.tar.gz
                "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout/${DIR}.tar.gz"
                SHOW_PROGRESS
        )

        message(STATUS ${error})

        if(NOT EXISTS "${ARA_SDK_SOURCE_DIR}/Libraries/third_party/${DIR}")
            message(STATUS "extracting ${DIR}.tar.gz")
            execute_process(
                    COMMAND tar -xvf ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/__checkout/${DIR}.tar.gz
                    WORKING_DIRECTORY ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/
                    RESULT_VARIABLE result
            )
        endif()
    endif ()

endforeach()

add_subdirectory(GLBase)
if(ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    add_dependencies(GLBase resources)
endif()

add_subdirectory(SceneGraph)
add_subdirectory(UI)
add_subdirectory(Utilities)
