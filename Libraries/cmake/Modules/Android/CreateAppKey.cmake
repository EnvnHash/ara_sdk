macro(create_app_key)
    if(SIGN_KEY_PASS)
        get_property(PACKAGE_NAME TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_NAME)
        set(DNAME_STR CN=Unknown,OU=Unknown,O=Unknown,C=Unknown)

        if (UNIX)
            find_program(KEYTOOL keytool)
            if (KEYTOOL)
                if (NOT EXISTS "${ANDROID_STUDIO_PROJ}/${PACKAGE_NAME}_key.jks")
                    execute_process(
                            COMMAND ${KEYTOOL}
                            -genkey -v
                            -keystore "${ANDROID_STUDIO_PROJ}/${PACKAGE_NAME}_key.jks"
                            -keyalg RSA
                            -keysize 2048
                            -validity 10000
                            -alias ${PACKAGE_NAME}
                            -dname ${DNAME_STR}
                            -storepass ${SIGN_KEY_PASS}
                            -keypass ${SIGN_KEY_PASS}
                    )
                endif()
            else()
                message(WARNING "keytool does not exist on this system. can't generate a key for signing")
            endif()

        elseif (WIN32)
            set(KEYTOOL_PATH "")
            if (EXISTS "$ENV{ProgramFiles}/Android/Android Studio/jbr/bin/keytool.exe")
                set(KEYTOOL_PATH "$ENV{ProgramFiles}/Android/Android Studio/jbr/bin/keytool.exe")
            elseif (EXISTS "$ENV{ProgramFiles}/Android/Android Studio/jre/bin/keytool.exe")
                set(KEYTOOL_PATH "$ENV{ProgramFiles}/Android/Android Studio/jre/bin/keytool.exe")
            endif()

            if (KEYTOOL_PATH)
                execute_process(
                        COMMAND "${KEYTOOL_PATH}"
                        -genkey -v
                        -keystore "${ANDROID_STUDIO_PROJ}/${PACKAGE_NAME}_key.jks"
                        -keyalg RSA
                        -keysize 2048
                        -validity 10000
                        -alias ${PACKAGE_NAME}
                        -dname ${DNAME_STR}
                        -storepass ${SIGN_KEY_PASS}
                        -keypass ${SIGN_KEY_PASS}
                )
            else()
                message(WARNING "keytool.exe not found")
            endif()
        endif()
    endif()
endmacro()