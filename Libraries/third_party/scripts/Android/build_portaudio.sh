#!/bin/bash

abis=("armeabi-v7a" "x86" "x86_64" "arm64-v8a")

# clone assimp git
DIR="portaudio_opensles"
if [ -d "$DIR" ]; then
    echo "portaudio dir exists..."
else
    echo "portaudio dir does not exist, cloning from git..."
    git clone https://github.com/Gundersanne/portaudio_opensles
fi

export ANDROID_NDK=$ANDROID_NDK_ROOT


if [[ "$OSTYPE" == "darwin"* ]]; then
    export CORES=$((`sysctl -n hw.logicalcpu`+1))
else
    export CORES=$((`nproc`+1))
fi


# create build dir
BUILD_DIR="portaudio_opensles/build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

cd $BUILD_DIR

for abi in ${abis[@]}; do
    mkdir -p $PWD/../../../../portaudio/Android/$abi
    cmake .. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_API_LEVEL=9 -DANDROID_PLATFORM=android-24 -DANDROID_ABI=$abi -DBUILD_SHARED_LIBS=OFF -DALSA=OFF -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=ON -DJACK=OFF
    make -j$CORES
    cp libportaudio.so $PWD/../../../../portaudio/Android/$abi/
    make clean
done

cd ..
cp -r include $PWD/../../../portaudio/Android/.