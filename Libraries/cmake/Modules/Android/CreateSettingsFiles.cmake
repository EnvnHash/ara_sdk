macro (create_settings_files)

    # /app/src/main/res/values/strings.xml
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/src/main/res/values/strings.xml "<resources>
    <string name=\"app_name\">${APP_NAME}</string>
</resources>")

#    distributionUrl=https://services.gradle.org/distributions/gradle-7.3.3-bin.zip

# /gradle/wrapper/gradle-wrapper.properties
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/gradle/wrapper/gradle-wrapper.properties "distributionBase=GRADLE_USER_HOME
distributionUrl=https://services.gradle.org/distributions/gradle-8.9-bin.zip
distributionPath=wrapper/dists
zipStorePath=wrapper/dists
zipStoreBase=GRADLE_USER_HOME")

    # settings.gradle
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/settings.gradle "include ':app'")

    # proguard-rules.pro
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/proguard-rules.pro "")

    # local.properties
    if (WIN32)
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/local.properties "sdk.dir=$ENV{ANDROID_SDK_ROOT}")
    else()
        FILE(WRITE ${ANDROID_STUDIO_PROJ}/local.properties "sdk.dir=$ENV{ANDROID_SDK_ROOT}
cmake.dir=/usr")
    endif()

    # gradle.properties
    FILE(WRITE ${ANDROID_STUDIO_PROJ}/gradle.properties "android.enableJetifier=true\nandroid.useAndroidX=true\norg.gradle.jvmargs=-Xmx1536m
org.gradle.caching=true
org.gradle.parallel=true
org.gradle.workers.max=6
android.useAndroidX=true
android.enableJetifier=true")

    # build.gradle
    FILE(WRITE  ${ANDROID_STUDIO_PROJ}/build.gradle
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
    repositories {
        google()
        mavenCentral()
        mavenLocal()
    }
}

task clean(type: Delete) {
    delete rootProject.buildDir
} ") # write it

endmacro()