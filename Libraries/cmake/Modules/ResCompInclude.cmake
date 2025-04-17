include(CreateSymLink)
if(${CMAKE_BUILD_TYPE} MATCHES Debug AND NOT ANDROID)
    create_symlink(${CMAKE_SOURCE_DIR}/Assets/resdata ${CMAKE_CURRENT_BINARY_DIR}/resdata)
else()
    #if (WIN32)
    #    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    #            COMMAND cmd /c  $<TARGET_FILE:ResComp> ${CMAKE_SOURCE_DIR}/Assets/resdata res_comp
    #            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    #            COMMENT "Compiling resources..."
    #            DEPENDS ${PACKAGE_DEPENDENCIES} $<TARGET_FILE:ResComp>
    #            )
    #e#seif (UNIX)
    #    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    #            COMMAND $<TARGET_FILE:ResComp> ${CMAKE_SOURCE_DIR}/Assets/resdata res_comp
    #            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    #            COMMENT "Compiling resource file..."
    #            DEPENDS ${PACKAGE_DEPENDENCIES} $<TARGET_FILE:ResComp>
    #            )
    #endif()
endif()
