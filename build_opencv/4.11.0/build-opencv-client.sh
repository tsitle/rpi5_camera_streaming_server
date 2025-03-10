#!/usr/bin/env bash

#
# by TS, Aug 2023
#

COMPILE_WITH_GUI=true
COMPILE_WITH_FFMPEG=true
COMPILE_WITH_GSTREAMER=true
COMPILE_WITH_JAVA=false
NEED_LIBCAMERA=false  # only Raspberry Pi
COMPILE_STATIC_LIBS=false  # mainly for Java

. .build-opencv.sh
