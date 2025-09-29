include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/AraSdkMacros.cmake)

if(${CMAKE_BUILD_TYPE} MATCHES Debug AND NOT ANDROID)
    if (${ARA_IS_SUBPROJECT} AND (${CMAKE_CURRENT_BINARY_DIR} STREQUAL "${FETCHCONTENT_BASE_DIR}/ara_sdk-build"))
        create_symlink(${CMAKE_SOURCE_DIR}/Assets/resdata ${CMAKE_BINARY_DIR}/resdata)
    else ()
        create_symlink(${CMAKE_SOURCE_DIR}/Assets/resdata ${CMAKE_CURRENT_BINARY_DIR}/resdata)
    endif ()
endif()
