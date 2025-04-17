#!/bin/bash

abis=("armeabi-v7a" "x86" "x86_64" "arm64-v8a")
ffk_abis=("android-arm-neon" "android-x86" "android-x86_64" "android-arm64")

# clone assimp git
DIR="ffmpeg-kit"
if [ -d "$DIR" ]; then
    echo "ffmpeg-kit dir exists..."
else
    echo "ffmpeg-kit dir does not exist, cloning from git..."
    git clone https://github.com/tanersener/ffmpeg-kit
fi

cd $DIR

# for libtwolame
export SNDFILE_CFLAGS=-I$PWD/src/libsndfile/include
export SNDFILE_LIBS=-l$PWD/src/libsndfile/src/.libs/libsndfile.a
export PKG_CONFIG_PATH= # avoid autodetection of wrong x86_64 sytem libraries
./android.sh --full --disable-lib-sdl --disable-lib-srt --disable-lib-libtheora

cd ..
for ((i=0; i<${#abis[@]} ; i++)); do
    mkdir -p $PWD/../../ffmpeg/Android/${abis[$i]}
    cp $PWD/$DIR/prebuilt/${ffk_abis[$i]}/ffmpeg/lib/*.so $PWD/../../ffmpeg/Android/${abis[$i]}/
done

cp -R $PWD/$DIR/prebuilt/${ffk_abis[0]}/ffmpeg/include $PWD/../../ffmpeg/Android/
