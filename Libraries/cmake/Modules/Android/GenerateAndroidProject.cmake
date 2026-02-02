# to be used in the CMakeList.txt of an Application
# e.g. in Applications/UITest/CMakeLists.txt, add the line "gen_android_proj(${PROJECT_NAME})"
# this will create a folder "Applications/UITest/Android" which will contain a android studio project
# with a symbolic link to the SDK
#
# The Application folder must have the following structure
#   AppFolder
#       OtherSourceFolder ..
#       OtherSourceFolder ..
#       main.cpp        -> must have exactly this name, otherwise it won't be filtered by the macro
#       AppClass.cpp    -> a derivative of UIApplication
#       AppClass.h
#
# NOTE: Make sure the Environmental variables ANDROID_NDK_HOME is set (linux -> /etc/environment)
# NOTE: On Windows the "Developer Mode" must be activated in order to permit the creating of symbolic links

macro(is_excluded_dir dir found)
    set(EXCLUDE_DIRS cmake-build-debug cmake-build-release cmake-build-relwithdebinfo .idea)
    set(${found} FALSE)
    FOREACH(excl_dir ${EXCLUDE_DIRS})
        string(FIND "${dir}" "${excl_dir}" index)
        if (index GREATER 0)
            set(${found} TRUE)
        endif ()
    ENDFOREACH()
endmacro()

macro(replace_dot_with_char input replChar output)
    set(result "")
    string(LENGTH "${input}" length)
    math(EXPR length_sub1 "${length} - 1")
    foreach(i RANGE ${length_sub1})
        string(SUBSTRING "${input}" ${i} 1 char)
        if(char STREQUAL ".")
            set(char ${replChar})
        endif()
        set(result "${result}${char}")
    endforeach()
    set(${output} "${result}")
endmacro()

set(ANDROID_SDK_VERSION 35)

include(Android/CreateAndroidCmakeLists)
include(Android/CreateAndroidManifest)
include(Android/CreateAppBuildGradle)
include(Android/CreateAppJavaSources)
include(Android/CreateAppCppSources)
include(Android/CreateBillingClient)
include(Android/CreateProjectStructure)
include(Android/CreatePureNativeAppSources)
include(Android/CreateSettingsFiles)
include(Android/CreateAppKey)

macro (extract_class_name deri)
    if (NOT deri STREQUAL "")
        string(REGEX MATCH "class.[(A-z)|(a-z)|(0-9)]*" cn ${line})
        string(LENGTH ${cn} var_length)
        string(SUBSTRING ${cn} 6 ${var_length} UIAPP_DERIVATE_CLASS)
        if (UIAPP_DERIVATE_CLASS STREQUAL "")
            message("Error!! No derivate of UIApplication found in the present header files")
        elseif (NOT CURRENT_NAMESPACE STREQUAL "")
            string(CONCAT UIAPP_DERIVATE_CLASS "::" "${UIAPP_DERIVATE_CLASS}")
            string(CONCAT UIAPP_DERIVATE_CLASS ${CURRENT_NAMESPACE} "${UIAPP_DERIVATE_CLASS}")
        endif()

        # get the relative header name
        get_filename_component(UIAPP_DERIVATE_CLASS_FILE_NAME_WE ${headerFile} NAME_WE)
    endif()
endmacro()

# APP_TYPE 0 = Pure native app without JAVA, APPTYPE = 1 Java MainActivity and JNI
macro (gen_android_proj
        #APP_NAME APP_PACKAGE_URL DEST_PLATF APP_TYPE APP_ICON_NAME ASSETS_FOLDER ARASDK_APP_ORIENTATION ADMOB_APP_ID ADMOB_UNIT_ID ADMOB_AD_TYPE BILLING_PRODUCT_ID
)
    #get_property(APP_ORIENTATION TARGET ${PROJECT_NAME} PROPERTY ARASDK_APP_ORIENTATION)
    #set(oneValueArgs APP_ORIENTATION)

    #cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    message(STATUS "Generating Android Project")
    if (NOT DEFINED ENV{ANDROID_NDK_HOME})
        message("GenerateAndroidProject.cmake Error!! Environmental variable ANDROID_NDK_HOME not set!! Aborting Android Studio project generation")
    else()
        # CONFIGURATION VARIABLES
        get_property(DEST_PLATFORMS TARGET ${PROJECT_NAME} PROPERTY ARASDK_DEST_PLATF)
        #set(DEST_PLATFORMS ${DEST_PLATF})

        # try to use the android studios cmake version
        if (EXISTS "$ENV{ANDROID_NDK_HOME}/../../cmake")
            set(HIGHEST_CMAKE_VERSION "0" "0" "0")
            # Iterate over all directories in the "cmake" folder
            FILE(GLOB children RELATIVE $ENV{ANDROID_NDK_HOME}/../../cmake $ENV{ANDROID_NDK_HOME}/../../cmake/*)
            foreach(DIR ${children})
                string(REGEX REPLACE "\\." ";" DIR_VERSION_COMPONENTS "${DIR}")
                # Compare the version numbers
                foreach(INDEX RANGE 0 2)
                    list(GET DIR_VERSION_COMPONENTS ${INDEX} VERS_COMP)
                    list(GET HIGHEST_CMAKE_VERSION ${INDEX} REF_VERS_COMP)
                    if (${VERS_COMP} GREATER REF_VERS_COMP)
                        set(HIGHEST_CMAKE_VERSION ${DIR_VERSION_COMPONENTS})
                        break()
                endif()
                endforeach()
            endforeach()
            set(DST_CMAKE_VERSION ${HIGHEST_CMAKE_VERSION}[0].${HIGHEST_CMAKE_VERSION}[1].${HIGHEST_CMAKE_VERSION}[2])
            set(DST_CMAKE_VERSION "")
            foreach(INDEX RANGE 0 2)
                list(GET HIGHEST_CMAKE_VERSION ${INDEX} VERS_COMP)
                string(APPEND DST_CMAKE_VERSION ${VERS_COMP})
                if (${INDEX} LESS 2)
                    string(APPEND DST_CMAKE_VERSION ".")
                endif ()
            endforeach()
            message(STATUS "Using Android Studio's cmake version ${DST_CMAKE_VERSION} for android project generation")
        else ()
            set(DST_CMAKE_VERSION ${CMAKE_VERSION})
            message(STATUS "Using OSes cmake version ${DST_CMAKE_VERSION} for android project generation")
        endif ()

        #get_property(APP_NAME TARGET ${PROJECT_NAME} PROPERTY ARASDK_APP_NAME)
        #get_property(APP_PACKAGE_URL TARGET ${PROJECT_NAME} PROPERTY ARASDK_APP_PACKAGE_URL)
        #set(SIGN_KEY_PASS dshUIYy287)

        # read ndk version string from the NDKs source.properties
        file(STRINGS $ENV{ANDROID_NDK_HOME}/source.properties NDK_VERS_STR)
        set(NDK_VERS_STR_LIST ${NDK_VERS_STR})
        separate_arguments(NDK_VERS_STR_LIST)

        if ("Pkg.Revision" IN_LIST NDK_VERS_STR_LIST)
            list (FIND NDK_VERS_STR_LIST "Pkg.Revision"  _index)
            if (${_index} GREATER -1)
                MATH(EXPR _index "${_index}+2")
                list (GET NDK_VERS_STR_LIST ${_index} NDK_VERS)
                # message(STATUS "NDK_VERS ${NDK_VERS}")
            endif()
        else()
            message(ERROR "Could not read NDK version from ENV{ANDROID_NDK_HOME}/source.properties")
        endif()

        # check for a derivative of UIApplication in the current source folder
        FILE(GLOB PROJ_HEADERS *.h src/*.h)
        foreach(headerFile ${PROJ_HEADERS})
            file(READ ${headerFile} headerText)

            # Convert the multiline string into a list, splitting by newlines
            string(REPLACE "\n" ";" line_list ${headerText})
            set(CURRENT_NAMESPACE "")
            foreach(line IN LISTS line_list)
                if (NOT line STREQUAL "")
                    string(REGEX MATCH "namespace.[(A-z)|(a-z)|(0-9)]*" nmsp ${line})
                    if (NOT nmsp STREQUAL "")
                        string(LENGTH ${nmsp} var_length)
                        string(SUBSTRING ${nmsp} 10 ${var_length} CURRENT_NAMESPACE)
                    endif ()

                    string(REGEX MATCH "::UIApplication.[(A-z)|(a-z)|(0-9)]*" deri ${line})
                    extract_class_name(deri)

                    string(REGEX MATCH "public UIApplication*" deri ${line})
                    extract_class_name(deri)
                endif()
            endforeach()
        endforeach()

        if(NOT UIAPP_DERIVATE_CLASS_FILE_NAME_WE)
            set(UIAPP_DERIVATE_CLASS_FILE_NAME_WE "UIApplication")
        endif()

        if(NOT UIAPP_DERIVATE_CLASS)
            set(UIAPP_DERIVATE_CLASS "ara::UIApplication")
        endif ()

        get_property(BILLING_PRODUCT_ID TARGET ${PROJECT_NAME} PROPERTY ARASDK_BILLING_PRODUCT_ID)
        if (NOT BILLING_PRODUCT_ID STREQUAL "")
            set(use_billing TRUE)
        else ()
            set(use_billing FALSE)
        endif ()

        create_proj_structure(
        #        ${APP_TYPE}
        ) # creates symlinks to all sdk subdirs, the main.cpp and the UIApp.cpp
        create_settings_files(
                #${APP_NAME} ${ADMOB_APP_ID}
        )
        create_android_manifest(
                #${APP_TYPE} ${APP_ICON_NAME} ${ARASDK_APP_ORIENTATION} ${ADMOB_APP_ID}
        )
        create_app_build_gradle(
                #${APP_NAME} ${APP_TYPE} ${ADMOB_APP_ID} ${use_billing}
        )
        create_android_cmakelists(
                #${APP_TYPE} ${ASSETS_FOLDER}
        )
        create_app_key(
                #${APP_NAME}
        )

        if (${use_billing})
            create_billing_client(
                    #${APP_TYPE} "${BILLING_PRODUCT_ID}"
            )
        endif ()

        if (${APP_TYPE} EQUAL 0)
            create_pure_native_app_source()
        elseif(${APP_TYPE} EQUAL 1)
            create_app_java_sources(
                    #${APP_TYPE} ${ADMOB_UNIT_ID} ${ADMOB_AD_TYPE} ${use_billing}
            )
            if("${ADMOB_UNIT_ID}" STREQUAL "")
                set(use_ad_mob FALSE)
            else()
                set(use_ad_mob TRUE)
            endif()
            create_app_cpp_sources(
                    #${use_ad_mob} ${use_billing}
            )
        endif()
    endif()
endmacro()