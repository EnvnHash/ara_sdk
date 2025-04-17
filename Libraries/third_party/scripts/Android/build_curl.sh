#!/bin/bash

abis=("armeabi-v7a" "x86" "x86_64" "arm64-v8a")
hosts=("armv7a-linux-androideabi" "i686-linux-android" "x86_64-linux-android" "aarch64-linux-android")
andr_hosts=("android-arm" "android-x86" "android-x86_64" "android-arm64")

export TOOLCHAIN=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64
export MIN_SDK_VERSION=24
#PATH=$TOOLCHAIN/bin:$PATH


# clone assimp git
DIR="curl"
if [ -d "$DIR" ]; then
    echo "curl dir exists..."
else
    echo "curl dir does not exist, cloning .."
    #git clone https://github.com/robertying/openssl-curl-android

    git clone https://github.com/curl/curl.git
    cd curl
    git checkout bfbde883af33397943df68a3ae01847a634d33bf
    cd ..
    git clone https://github.com/openssl/openssl.git
    cd openssl
    git checkout fb047ebc87b18bdc4cf9ddee9ee1f5ed93e56aff # openssl (OpenSSL_1_1_1l)
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    export CORES=$((`sysctl -n hw.logicalcpu`+1))
else
    export CORES=$((`nproc`+1))
fi

mkdir -p build/openssl
cd openssl

# build openssl
for ((i=0; i<${#hosts[@]} ; i++)); do
    echo "----------- build openssl ${abis[$i]}"
    echo $PWD
    # arm64
    export TARGET_HOST=${hosts[$i]}
    export ANDROID_ARCH=${abis[$i]}
    export AR=$TOOLCHAIN/bin/llvm-ar
    export CC=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang
    export AS=$CC
    export CXX=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang++
    export LD=$TOOLCHAIN/bin/ld
    export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
    export STRIP=$TOOLCHAIN/bin/llvm-strip

    ./Configure ${andr_hosts[$i]} shared -fPIC -D__ANDROID_API__=$MIN_SDK_VERSION --prefix=$PWD/build/$ANDROID_ARCH

    make -j$CORES
    make install_sw
    make clean
    mkdir -p ../build/openssl/$ANDROID_ARCH
    cp -R $PWD/build/$ANDROID_ARCH ../build/openssl/

    mkdir -p $PWD/../../../openssl/Android/$ANDROID_ARCH
    cp -R $PWD/build/$ANDROID_ARCH/include $PWD/../../../openssl/Android/$ANDROID_ARCH/
done

cd ..

# build curl
ARGUMENTS=" "
mkdir -p build/curl
cd curl

autoreconf -fi

for ((i=0; i<${#hosts[@]} ; i++)); do
    echo "----------- build curl ${abis[$i]}"
    echo $PWD
    # arm64
    export TARGET_HOST=${hosts[$i]}
    export AR=$TOOLCHAIN/bin/llvm-ar
    export CC=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang
    export AS=$CC
    export CXX=$TOOLCHAIN/bin/$TARGET_HOST$MIN_SDK_VERSION-clang++
    export LD=$TOOLCHAIN/bin/ld
    export RANLIB=$TOOLCHAIN/bin/llvm-ranlib
    export STRIP=$TOOLCHAIN/bin/llvm-strip
    export PKG_CONFIG_PATH=$PWD/../build/openssl/${abis[$i]}/lib/pkgconfig:$PKG_CONFIG_PATH

    ./configure --host=${hosts[$i]} --target=${abis[$i]} --with-ssl=$PWD/../openssl/build/${abis[$i]} --prefix=$PWD/../build/curl/${abis[$i]}

    make -j$CORES
    make install

    mkdir -p $PWD/../../../curl/Android/${abis[$i]}
    cp -R $PWD/../build/curl/${abis[$i]}/include $PWD/../../../curl/Android/

    make clean
done

cd ..
cd build/openssl

# android studio can't deal with versioned shared libraries ... so remove the versioning here
for ((i=0; i<${#abis[@]} ; i++)); do
    cd ${abis[$i]}/lib

    mv libssl.so.1.1 libssl.so
    mv libcrypto.so.1.1 libcrypto.so
    patchelf --replace-needed libcrypto.so.1.1 libcrypto.so libssl.so
    patchelf --set-soname libssl.so libssl.so
    patchelf --set-soname libcrypto.so libcrypto.so
    cp -R libcrypto.so $PWD/../../../../../../openssl/Android/$ANDROID_ARCH/.
    cp -R libssl.so $PWD/../../../../../../openssl/Android/$ANDROID_ARCH/.

    cd ../..
done

cd ../curl

# android studio can't deal with versioned shared libraries ... so remove the versioning here
for ((i=0; i<${#abis[@]} ; i++)); do
    cd ${abis[$i]}/lib

    patchelf --replace-needed libcrypto.so.1.1 libcrypto.so libcurl.so
    patchelf --replace-needed libssl.so.1.1 libssl.so libcurl.so

    cp -R libcurl.so $PWD/../../../../../../curl/Android/$ANDROID_ARCH/.

    cd ../..
done
