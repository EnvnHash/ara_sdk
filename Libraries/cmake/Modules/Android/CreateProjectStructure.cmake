include(Android/CreateMainActivityLayout)

macro(create_proj_structure APP_TYPE)

    # create the project folder
    set(ANDROID_STUDIO_PROJ ${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGE_NAME}_Android)

    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ})
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/cpp)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/res)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/res/layout)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/res/values-v${ANDROID_SDK_VERSION})
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/libs)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/gradle)
    file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/gradle/wrapper)

    if (${APP_TYPE} EQUAL 1)
        file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu)
        file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst)
        file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/java/eu/zeitkunst/${PROJECT_NAME})
    endif()

    FILE(GLOB sub_dir ABSOLUTE .. ${CMAKE_CURRENT_SOURCE_DIR}/*)

    SET(sub_dir_list "")
    FOREACH(dir ${sub_dir})
        is_excluded_dir(${dir} is_exl)
        IF(IS_DIRECTORY ${dir}
                AND NOT ${dir} STREQUAL ${ANDROID_STUDIO_PROJ}
                AND NOT ${dir} STREQUAL ..
                AND NOT is_exl)
            LIST(APPEND sub_dir_list ${dir})
        ENDIF()
    ENDFOREACH()

    foreach(SRC_DIR ${sub_dir_list})
        # avoid creating symlinks from created symlinks
        string(FIND ${SRC_DIR} ${ANDROID_STUDIO_PROJ} IS_PROJ_SYMLINK)

        if (${IS_PROJ_SYMLINK} EQUAL -1)
            # get the project relative path
            file(RELATIVE_PATH SRC_REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${SRC_DIR})

            # if the project relative path is valid, link it as a directory
            if(NOT EXISTS ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_REL_PATH})
                create_symlink(${SRC_DIR} ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_REL_PATH})
            endif()
        endif()
    endforeach()

    file(GLOB PROJ_HEADERS *.h)
    foreach(SRC ${PROJ_HEADERS})
        get_filename_component(SRC_NAME ${SRC} NAME)
        if(NOT EXISTS ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_NAME})
            if (NOT WIN32)
                create_symlink(${SRC} ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_NAME})
            else()
                file(COPY ${SRC} ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_NAME})
            endif()
        endif()
    endforeach()

    # check the project folder for all .cpp files, except main.cpp and create a symlink
    file(GLOB PROJ_SOURCES *.cpp)
    foreach(SRC ${PROJ_SOURCES})
        get_filename_component(SRC_NAME ${SRC} NAME)
        if (NOT ${SRC_NAME} STREQUAL "main.cpp")
            if (NOT WIN32)
                create_symlink(${SRC} ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_NAME})
            else()
                file(COPY ${SRC} ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/${SRC_NAME})
            endif()
        endif()
    endforeach()

    # copy the sdk to the project as a symbolic link
#   if(NOT EXISTS ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/sdk)
#        create_symlink(${CMAKE_SOURCE_DIR} ${ANDROID_STUDIO_PROJ}/app/src/main/cpp/sdk)
#    endif()
    # copy resources

    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/mipmap-hdpi DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/mipmap-mdpi DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/mipmap-xhdpi DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/mipmap-xxhdpi DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/mipmap-xxxhdpi DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    #file(COPY ${ARA_SDK_SOURCE_DIR}/Assets/android/layout DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/values DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)
    file(COPY ${CMAKE_SOURCE_DIR}/Assets/android/drawable DESTINATION ${ANDROID_STUDIO_PROJ}/app/src/main/res/)

    if (${APP_TYPE} EQUAL 1)
        file(MAKE_DIRECTORY ${ANDROID_STUDIO_PROJ}/app/src/main/res/layout)
        create_main_activity_layout()
    endif()

endmacro()
