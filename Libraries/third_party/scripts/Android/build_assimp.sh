#!/bin/bash

abis=("armeabi-v7a" "x86" "x86_64" "arm64-v8a")

# clone assimp git
DIR="assimp"
if [ -d "$DIR" ]; then
    echo "assimp dir exists..."
else
    echo "assimp dir does not exist, cloning from git..."
    git clone https://github.com/assimp/assimp
fi

#patch assimp/port/AndroidJNI/CMakeLists.txt < androidjni.patch

cd $DIR

export PKG_CONFIG_PATH= # avoid autodetection of wrong x86_64 sytem libraries

for abi in ${abis[@]}; do
    mkdir build && cd build
    mkdir -p $PWD/../../../../Assimp/Android/$abi
    cmake -DASSIMP_ANDROID_JNIIOSYSTEM=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=ON -DANDROID_PLATFORM=android-24 -DANDROID_ABI=$abi -DCMAKE_INSTALL_PREFIX=$PWD/../../../../Assimp/Android -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake ../
    make -j9 && make install
    mv $PWD/../../../../Assimp/Android/lib/* $PWD/../../../../Assimp/Android/$abi
    rm -r $PWD/../../../../Assimp/Android/lib
    make clean
    cd .. && rm -r build
done