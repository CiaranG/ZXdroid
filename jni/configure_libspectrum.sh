#!/bin/sh

cd libspectrum-0.5.0.1/

export ANDROID_ROOT=/opt/android-ndk
export PATH=$PATH:$ANDROID_ROOT/build/prebuilt/linux-x86/arm-eabi-4.2.1/bin/

./configure --host=arm-eabi \
	CC=arm-eabi-gcc \
	CPPFLAGS="-I$ANDROID_ROOT/build/platforms/android-4/arch-arm/usr/include/" \
	CFLAGS="-nostdlib" \
	LDFLAGS="-Wl,-rpath-link=$ANDROID_ROOT/build/platforms/android-4/arch-arm/usr/lib/ -L$ANDROID_ROOT/build/platforms/android-4/arch-arm/usr/lib/" \
	LIBS="-lc "

make libspectrum.h

