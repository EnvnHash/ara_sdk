cmake_minimum_required(VERSION 3.10)
#cmake_policy(SET CMP0146 NEW)

project(ARASdk)

set (ARA_IS_SUBPROJECT OFF)
if (NOT ARA_SDK_SOURCE_DIR)
    set(ARA_SDK_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
    if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR})
        set (ARA_IS_SUBPROJECT ON)
        message(STATUS "ARA SDK is sub-project")
    endif()
endif()
if (ANDROID)
    set(ARA_IS_SUBPROJECT ON)
endif()
message(STATUS "ARA_SDK_SOURCE_DIR: ${ARA_SDK_SOURCE_DIR}")

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)

include(AraSdk)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)# only include specific components into Cpack package generation
set(GEN_ITEMFACTORY 0)

# in release mode use Assets compilation as static library
if (NOT ARA_IS_SUBPROJECT AND ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    add_subdirectory(Assets)
endif()

if (NOT ${ARA_IS_SUBPROJECT})
    add_subdirectory(Examples)
endif ()

add_subdirectory(Libraries)
add_ara_doc_link_to_readme()

# pass ara_sdk dependencies variables to parent project in case it exists
get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    #set(ara_sdk_COMPILE_DEFINITIONS "${ara_sdk_COMPILE_DEFINITIONS}" PARENT_SCOPE)
    set(ara_sdk_INCLUDE_DIRS "${ara_sdk_INCLUDE_DIRS}" PARENT_SCOPE)
    set(ara_sdk_LIBRARIES "${ara_sdk_LIBRARIES}" PARENT_SCOPE)
    set(ara_sdk_BINARIES "${ara_sdk_BINARIES}" PARENT_SCOPE)
endif()