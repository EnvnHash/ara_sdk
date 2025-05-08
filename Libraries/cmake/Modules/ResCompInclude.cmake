include(CreateSymLink)
if(${CMAKE_BUILD_TYPE} MATCHES Debug AND NOT ANDROID)
    create_symlink(${CMAKE_SOURCE_DIR}/Assets/resdata ${CMAKE_CURRENT_BINARY_DIR}/resdata)
endif()
