################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/SNAudioRingV.cpp \
../src/SNFFmpegVideo.cpp \
../src/SNMuseoFrei.cpp \
../src/SNVideoTexture.cpp \
../src/SNVideoTexture2.cpp \
../src/SNVideoTile0.cpp \
../src/SNVideoTileAudio.cpp \
../src/SNVideoTileAudio3d.cpp 

OBJS += \
./src/SNAudioRingV.o \
./src/SNFFmpegVideo.o \
./src/SNMuseoFrei.o \
./src/SNVideoTexture.o \
./src/SNVideoTexture2.o \
./src/SNVideoTile0.o \
./src/SNVideoTileAudio.o \
./src/SNVideoTileAudio3d.o 

CPP_DEPS += \
./src/SNAudioRingV.d \
./src/SNFFmpegVideo.d \
./src/SNMuseoFrei.d \
./src/SNVideoTexture.d \
./src/SNVideoTexture2.d \
./src/SNVideoTile0.d \
./src/SNVideoTileAudio.d \
./src/SNVideoTileAudio3d.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I"/home/sven/local/src/eclipse_tav/tav_video/src" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


