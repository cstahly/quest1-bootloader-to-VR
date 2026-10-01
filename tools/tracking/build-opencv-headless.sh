#!/bin/sh
# Build tracking-only OpenCV against an existing pmbootstrap ARM sysroot.
# Source checkout: upstream OpenCV tag 4.13.0. No device changes or package installs.
set -eu
[ "$#" -eq 3 ] || { echo 'usage: build-opencv-headless.sh OPENCV_SOURCE BUILD_DIR INSTALL_PREFIX' >&2; exit 2; }
: "${QUEST_VIO_SYSROOT:?Set to the prepared aarch64 musl buildroot}"
toolchain=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)/aarch64-musl-toolchain.cmake
cmake -S "$1" -B "$2" -G Ninja \
 -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Release \
 -DCMAKE_INSTALL_PREFIX="$3" -DBUILD_LIST=core,imgproc,features2d,flann \
 -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF \
 -DBUILD_opencv_apps=OFF -DBUILD_JAVA=OFF -DBUILD_opencv_python3=OFF \
 -DWITH_OPENGL=OFF -DWITH_OPENCL=OFF -DWITH_LAPACK=OFF -DWITH_EIGEN=OFF \
 -DWITH_IPP=OFF -DWITH_TBB=OFF -DWITH_ITT=OFF -DWITH_GTK=OFF -DWITH_QT=OFF \
 -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_V4L=OFF
cmake --build "$2" -j2
cmake --install "$2"
