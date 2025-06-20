# ARA SDK

Cross-platform application framework for 2D User Interfaces (similar to QT) and 3D graphics based on OpenGL, written in C++20.

![Static Badge](https://img.shields.io/badge/os-macos_windows_linux_android-blue?style=flat)
[![CMake build on ubuntu, windows, macos](https://github.com/EnvnHash/ara_sdk/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/EnvnHash/ara_sdk/actions/workflows/cmake-multi-platform.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/07c680603b224268815644255071746f)](https://app.codacy.com/gh/EnvnHash/ara_sdk/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

![Ara Snapshot](Documentation/images/ara_snapshot.png)

## Building

### Linux, MacOS and Windows

Build using cmake or any IDE that supports it.

``` 
mkdir build
cd build 
cmake ..
make -j
```

All dependencies are checked out and unzipped automatically. 

### Android
[See Building on android](#building-on-android)

## Documentation

[GLBase](Libraries/GLBase/src/README.md) OpenGL Utilities and wrapper classes / Window Management

[SceneGraph](Libraries/SceneGraph/src/README.md) 3D Scenegraph 

[UI](Libraries/UI/src/README.md) UI System

[Utilities](Libraries/Utilities/src/README.md) FileSystem, Network, String and other utilities


## Building on Android

Add these two lines to your ara sdks application root folders CMakeLists.txt

```
include(GenerateAndroidProject)
gen_android_proj(${PROJECT_NAME} 0 icon)
```

The first argument refers to the cmake project name, the second to the type of Android Project (0 = pure native NDK application, 1 = Java NDI application) and the third argument refers to an icon in png format which will be included into the AndroidManifest file

(**Note:** depending on your setup you may need to add an explicit path to the ara sdk modules: e.g.: `include(${ara_sdk_SOURCE_DIR}/Libraries/cmake/Modules/Android/GenerateAndroidProject.cmake)`)

This will generate a folder called ${PROJECT_NAME}_Android in the root folder of your project. Open this folder with Android Studio and build it as a usual NDK project.

As a demo, check the example project in Examples/UITest and uncommnent the lines:

```
include(Android/GenerateAndroidProject)
gen_android_proj(${PROJECT_NAME} 0 icon)
```

This will generate the folder Examples/UITest/UITest_Android. Open and build it as a regular project in android studio.

> [!IMPORTANT]
>- all resources such as images, fonts, etc. are expected to be in a folder called _Assets_ in the root folder of the project
>- inside the _Assets_ folder, a folder called _SplashScreen_ with a bitmap icon, a background image and a splash screen xml layout file is expected

The _GenerateAndroidProject_ macro involves a few automatically executed steps
- check for the NDK version installed on your system (ANDROID_NDK and ANDROID_NDK_HOME environment variables must be set)
- check for a derivative of UIApplication in the root and src/ folder of your project and create a file called _native_lib.cpp_ which will create an instance of this derivative class. This file will contain the _android_main()_ function and also set up the ASensorManage
- create a CMakeLists.txt file inside the ${PROJECT_NAME}_Android folder including the ara sdk dependencies and source files. It will also include any *.cpp sources inside the projects root folder
- create a AndroidManifest file including the icon passed as an argument to gen_android_proj()
- in case of a JNI app, create a java MainActivity class, a JNI interface class and a camera permission helper class
- create a key for signing using _keytool_
