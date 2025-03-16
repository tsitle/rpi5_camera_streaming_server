#!/usr/bin/env bash

#
# by TS, Aug 2023
#

CFG_COMPILE_WITH_GUI=false
CFG_COMPILE_WITH_FFMPEG=true
CFG_COMPILE_WITH_GSTREAMER=true
CFG_COMPILE_WITH_PYTHON3=false
CFG_COMPILE_WITH_JAVA=false
CFG_NEED_LIBCAMERA=true  # only Raspberry Pi
CFG_COMPILE_STATIC_LIBS=false  # mainly for Java
CFG_INSTALL_TO_SYSTEM=true

. .build-opencv.sh
