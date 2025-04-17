#!/bin/bash

abis=("armeabi-v7a" "x86" "x86_64" "arm64-v8a")

# clone assimp git
DIR="openal-soft"
if [ -d "$DIR" ]; then
    echo "openal dir exists..."
else
    echo "openal dir does not exist, cloning from git..."
    git clone git://repo.or.cz/openal-soft.git
fi

export ANDROID_NDK=$ANDROID_NDK_ROOT


if [[ "$OSTYPE" == "darwin"* ]]; then
    export CORES=$((`sysctl -n hw.logicalcpu`+1))
else
    export CORES=$((`nproc`+1))
fi


# create build dir
BUILD_DIR="openal-soft/build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

cd $BUILD_DIR


for abi in ${abis[@]}; do
    case $abi in
        "x86" )
            NEON="OFF"
            ABI="i686-linux-android"
            ;;
        "armeabi-v7a" )
            NEON="ON"
            ABI="arm-linux-androideabi"
            ;;
        "x86_64" )
            NEON="OFF"
            ABI="x86_64-linux-android"
            ;;
        "arm64-v8a" )
            NEON="ON"
            ABI="aarch64-linux-android"
            ;;
    esac

    mkdir -p $PWD/../../../../OpenAL/Android/$abi
    cmake .. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_API_LEVEL=9 -DALSOFT_CPUEXT_NEON=$NEON -DANDROID_PLATFORM=android-24 -DANDROID_ABI=$abi -DOPENSL_LIBRARY=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$ABI/24/libOpenSLES.so -DBUILD_SHARED_LIBS=ON -DALSOFT_EXAMPLES=OFF -DALSOFT_UTILS=OFF
    make -j$CORES
    #cp libcommon.a $PWD/../../../../OpenAL/Android/$abi/
    #cp libex-common.a $PWD/../../../../OpenAL/Android/$abi/
    cp libopenal.so $PWD/../../../../OpenAL/Android/$abi/
    make clean
done
cd ..
cp -r include $PWD/../../../OpenAL/Android/.
