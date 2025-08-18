include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/AraSdkMacros.cmake)

# PugiXML
if(NOT WIN32 AND NOT ANDROID)
    if(NOT PUGIXML_FOUND)
        find_package (PugiXML REQUIRED)
    endif()
endif()

# options
if (WIN32)
    add_compile_definitions(NOMINMAX)
    #string(APPEND CMAKE_CXX_FLAGS " /wd4146")
    option (PUGIXML_HEADER_ONLY "Use the header only version of PugiXML (longer compilation time)" ON)
    append_unique(ara_sdk_INCLUDE_DIRS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml)
elseif (ANDROID)
    set (PUGIXML_HEADER_ONLY  ON)
else()
    option (PUGIXML_HEADER_ONLY "Use the header only version of PugiXML (longer compilation time)" OFF)
endif()

if (NOT PUGIXML_HEADER_ONLY AND PUGIXML_FOUND)
    append_unique(ara_sdk_INCLUDE_DIRS ${PUGIXML_INCLUDE_DIR})
endif (NOT PUGIXML_HEADER_ONLY AND PUGIXML_FOUND)

if (PUGIXML_HEADER_ONLY)
    append_unique(ara_sdk_INCLUDE_DIRS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml)
    add_definitions(-DPUGIXML_HEADER_ONLY)
else()
    append_unique(ara_sdk_LIBRARIES ${PUGIXML_LIBRARY})
endif()
