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
    include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml)
elseif (ANDROID)
    set (PUGIXML_HEADER_ONLY  ON)
else()
    option (PUGIXML_HEADER_ONLY "Use the header only version of PugiXML (longer compilation time)" OFF)
endif()

if (NOT PUGIXML_HEADER_ONLY AND PUGIXML_FOUND)
    include_directories(${PUGIXML_INCLUDE_DIR})
endif (NOT PUGIXML_HEADER_ONLY AND PUGIXML_FOUND)

if (PUGIXML_HEADER_ONLY)
    include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml)

    add_definitions(-DPUGIXML_HEADER_ONLY)
else()
    target_link_libraries(${PROJECT_NAME} ${PUGIXML_LIBRARY})
endif()
