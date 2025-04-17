#!/bin/bash

abis=("armeabi-v7a" "x86" "x86_64" "arm64-v8a")
hosts=("armv7a-linux-androideabi" "i686-linux-android" "x86_64-linux-android" "aarch64-linux-android")

if [[ "$OSTYPE" == "darwin"* ]]; then
    export CORES=$((`sysctl -n hw.logicalcpu`+1))
else
    export CORES=$((`nproc`+1))
fi

export PKG_CONFIG_PATH= # avoid autodetection of wrong x86_64 sytem libraries
export TOOLCHAIN=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64
export MIN_SDK_VERSION=24

# clone assimp git
DIR="FreeImage"
if [ -d "$DIR" ]; then
    echo "FreeImage dir exists..."
else
    echo "FreeImage dir does not exist, cloning from git..."
    wget http://downloads.sourceforge.net/freeimage/FreeImage3180.zip
    unzip FreeImage3180.zip
    wget https://sourceforge.net/projects/libpng/files/libpng16/1.6.35/lpng1635.zip
    unzip lpng1635.zip
fi

cd $DIR

for ((i=0; i<${#hosts[@]} ; i++)); do
    # arm64
    export TARGET_HOST=${hosts[$i]}
    export ANDROID_ARCH=${abis[$i]}
    export AR=$TOOLCHAIN/bin/llvm-ar
    export CC=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang
    export AS=$CC
    export CC=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang
    export CXX=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang++
    export LD=$TOOLCHAIN/bin/ld
    export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
    export STRIP=$TOOLCHAIN/bin/llvm-strip
    export CFLAGS=-D__ANSI__
    export CXXFLAGS="-O3 -fPIC"

    if [ "${abis[$i]}" == "armeabi-v7a" ]; then
        export CFLAGS="-O3 -D__ANSI__ -mfloat-abi=softfp -mfpu=neon -march=armv7 -O3 -fPIC -D__ARM_NEON -DPNG_ARM_NEON -DPNG_ALIGNED_MEMORY_SUPPORTED "
        export CXXFLAGS="-O3 -mfloat-abi=softfp -mfpu=neon -march=armv7 -O3 -fPIC -D__ARM_NEON -DPNG_ARM_NEON -DPNG_ALIGNED_MEMORY_SUPPORTED "
    fi

    make clean
    make -f Makefile.gnu -j $CORES

    mkdir -p $PWD/../../../FreeImage/Android/${abis[$i]}
    mkdir -p $PWD/../../../FreeImage/Android/include
    cp $PWD/Dist/libfreeimage-3.18.0.so $PWD/../../../FreeImage/Android/${abis[$i]}/libfreeimage.so
    patchelf --set-soname libfreeimage.so $PWD/../../../FreeImage/Android/${abis[$i]}/libfreeimage.so
    cp $PWD/Dist/FreeImage.h $PWD/../../../FreeImage/Android/include/.

    # neon optimized filter functions are not built with freeimage repo ... do it here
    cd ../lpng1635
    rm -r build
    mkdir -p build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_PLATFORM=android-24 -DPNG_STATIC=OFF -DPNG_SHARED=ON -DPNG_TESTS=OFF -DANDROID_ABI=${abis[$i]} -DHAVE_LD_VERSION_SCRIPT=OFF ..
    make -j$CORES
    cp libpng16.so $PWD/../../../../FreeImage/Android/${abis[$i]}/.
    cd ../../FreeImage
done


