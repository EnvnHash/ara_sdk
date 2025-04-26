macro (create_app_build_gradle APP_NAME)

    # app/build.gradle
    set(app_build_gradle)

    list(APPEND app_build_gradle "plugins {
    id 'com.android.application'
}\n\n")

    if(ARA_USE_ARCORE)
        list(APPEND app_build_gradle "/*
The arcore aar library contains the native shared libraries.  These are
extracted before building to a temporary directory.
 */
def arcore_libpath = \"\${buildDir}/arcore-native\"\n
// Create a configuration to mark which aars to extract .so files from
configurations { natives }\n\n")
    endif()

    list(APPEND app_build_gradle "android {
    compileSdkVersion ${ANDROID_SDK_VERSION}
    ndkVersion '${NDK_VERS}'
    namespace 'eu.zeitkunst.${PACKAGE_NAME}'

    defaultConfig {
        applicationId \"eu.zeitkunst.${PACKAGE_NAME}\"
        minSdkVersion 24
        targetSdkVersion ${ANDROID_SDK_VERSION}
        versionCode 1
        versionName \"1.0\"
        externalNativeBuild {
            cmake {
                cppFlags \"-std=c++20 -frtti \"
                arguments \"-DANDROID_STL=c++_shared\", \"-DCMAKE_VERBOSE_MAKEFILE=1\"")

    if(ARA_USE_ARCORE)
        list(APPEND app_build_gradle ",
                        \"-DARCORE_LIBPATH=\${arcore_libpath}/jni\",
                        \"-DARCORE_INCLUDE=\${project.rootDir}/../../libraries/include\",
                        \"-DPARALLEL_COMPILE_JOBS=8\"")
    endif()

    list(APPEND app_build_gradle "
            }
        }
        ndk {
            abiFilters ${DEST_PLATFORMS}
        }
    }

    signingConfigs {
        release {
            // You need to specify either an absolute path or include the
            // keystore file in the same directory as the build.gradle file.
            storeFile file(\"${ANDROID_STUDIO_PROJ}/${APP_NAME}_key.jks\")
            storePassword \"${SIGN_KEY_PASS}\"
            keyAlias \"${APP_NAME}\"
            keyPassword \"${SIGN_KEY_PASS}\"
        }
    }

    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android-optimize.txt'), 'proguard-rules.pro'
            signingConfig signingConfigs.release
        }
    }
    externalNativeBuild {
        cmake {
            path \"src/main/cpp/CMakeLists.txt\"
            version \"${DST_CMAKE_VERSION}\"
        }
    }
}

dependencies {
    implementation fileTree(dir: 'libs', include: ['*.jar'])
    implementation 'androidx.appcompat:appcompat:1.3.0'
    implementation 'androidx.core:core-splashscreen:1.0.0'
    implementation 'androidx.constraintlayout:constraintlayout:1.1.3'")

    if(ARA_USE_ARCORE)
        list(APPEND app_build_gradle "
    // ARCore (Google Play Services for AR) library.
    implementation 'com.google.ar:core:1.32.0'
    implementation 'com.google.android.material:material:1.1.0'
    natives 'com.google.ar:core:1.32.0'
")
    endif()

    list(APPEND app_build_gradle "}")

    if (ARA_USE_ARCORE)
        list(APPEND app_build_gradle "
// Extracts the shared libraries from aars in the natives configuration.
// This is done so that NDK builds can access these libraries.
task extractNativeLibraries() {
    // Always extract, this insures the native libs are updated if the version changes.
    outputs.upToDateWhen { false }
    doFirst {
        configurations.natives.files.each { f ->
            copy {
                from zipTree(f)
                into arcore_libpath
                include \"jni/**/*\"
            }
        }
    }
}

tasks.whenTaskAdded {
    task-> if (task.name.contains(\"external\") && !task.name.contains(\"Clean\")) {
        task.dependsOn(extractNativeLibraries)
    }
}")
    endif()

    FILE(WRITE ${ANDROID_STUDIO_PROJ}/app/build.gradle ${app_build_gradle})

endmacro()
