#!/bin/bash

#CUSTOM_GSTREAMER_PLUGIN_PATH="~/test-opencv_mpjeg_stream-cpp/build_libcamera/libcamera/build/src/gstreamer"
CUSTOM_GSTREAMER_PLUGIN_PATH="/usr/local/lib/arm-linux-gnueabihf/gstreamer-1.0/libgstlibcamera.so"
export GST_PLUGIN_PATH="$CUSTOM_GSTREAMER_PLUGIN_PATH"

./comp.sh || exit 1
./build/HttpCamServer $@
