#!/bin/bash

#CUSTOM_GSTREAMER_PLUGIN_PATH="~/test-opencv_mpjeg_stream-cpp/build_libcamera/libcamera/build/src/gstreamer"
CUSTOM_GSTREAMER_PLUGIN_PATH="/usr/local/lib/arm-linux-gnueabihf/gstreamer-1.0/libgstlibcamera.so"
export GST_PLUGIN_PATH="$CUSTOM_GSTREAMER_PLUGIN_PATH"

gst-launch-1.0 libcamerasrc ! \
		video/x-raw,colorimetry=bt709,format=NV12,width=1280,height=720,framerate=15/1 ! \
		jpegenc ! multipartmux ! \
		queue leaky=1 max-size-buffers=0 ! \
		tcpserversink host=0.0.0.0 port=8091
