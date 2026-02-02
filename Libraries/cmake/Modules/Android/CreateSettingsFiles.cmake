macro (create_settings_files
        #APP_NAME ADMOB_APP_ID
)

    get_property(APP_TYPE TARGET ${PROJECT_NAME} PROPERTY ARASDK_APP_TYPE)
    get_property(PACKAGE_NAME TARGET ${PROJECT_NAME} PROPERTY ARASDK_PACKAGE_NAME)

    # /app/src/main/res/values/strings.xml
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/res/values/strings.xml "<resources>
    <string name=\"app_name\">${PACKAGE_NAME}</string>
</resources>")

#    distributionUrl=https://services.gradle.org/distributions/gradle-7.3.3-bin.zip

# /gradle/wrapper/gradle-wrapper.properties
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/gradle/wrapper/gradle-wrapper.properties "distributionBase=GRADLE_USER_HOME
distributionUrl=https://services.gradle.org/distributions/gradle-8.9-bin.zip
distributionPath=wrapper/dists
zipStorePath=wrapper/dists
zipStoreBase=GRADLE_USER_HOME")

    get_property(ADMOB_APP_ID TARGET ${PROJECT_NAME} PROPERTY ARASDK_ADMOB_APP_ID)

    # settings.gradle
    SET(settings_gradle)
    if (NOT "${ADMOB_APP_ID}" STREQUAL "")
        list(APPEND settings_gradle "pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.PREFER_SETTINGS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = \"${PACKAGE_NAME}\"
")
    endif ()
    list(APPEND settings_gradle "include ':app'" )
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/settings.gradle ${settings_gradle})

    # proguard-rules.pro
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/proguard-rules.pro "")

    # local.properties
    if (WIN32 OR APPLE)
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/local.properties "sdk.dir=$ENV{ANDROID_SDK_ROOT}")
    else()
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/local.properties "sdk.dir=$ENV{ANDROID_SDK_ROOT}
cmake.dir=/usr")
    endif()

    # gradle.properties
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/gradle.properties "org.gradle.jvmargs=-Xmx1536m
org.gradle.caching=true
org.gradle.parallel=true
org.gradle.workers.max=6
android.useAndroidX=true
android.enableJetifier=true")

    # build.gradle
    SET(project_build_gradle)
    list(APPEND project_build_gradle
            "buildscript {
    repositories {
       google()
       mavenCentral()
    }
    dependencies {
        classpath 'com.android.tools.build:gradle:8.1.1'
    }
}

allprojects {
    repositories {")

    if (${ADMOB_APP_ID} STREQUAL "")
        list(APPEND project_build_gradle "\t\tgoogle()")
    endif ()

    list(APPEND project_build_gradle "
        mavenCentral()
        mavenLocal()
    }
}

task clean(type: Delete) {
    delete rootProject.buildDir
} ")

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/build.gradle ${project_build_gradle})
endmacro()