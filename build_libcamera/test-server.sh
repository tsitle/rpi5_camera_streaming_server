#!/bin/bash

#CUSTOM_GSTREAMER_PLUGIN_PATH="~/test-opencv_mpjeg_stream-cpp/build_libcamera/libcamera/build/src/gstreamer"
#CUSTOM_GSTREAMER_PLUGIN_PATH="/usr/local/lib/arm-linux-gnueabihf/gstreamer-1.0/libgstlibcamera.so"
#CUSTOM_GSTREAMER_PLUGIN_PATH="/usr/local/lib/aarch64-linux-gnu/gstreamer-1.0/libgstlibcamera.so"
#CUSTOM_GSTREAMER_PLUGIN_PATH="/usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstlibcamera.so"
#export GST_PLUGIN_PATH="$CUSTOM_GSTREAMER_PLUGIN_PATH"

#TMP_FORMAT="NV12"
TMP_FORMAT="RGBx"
gst-launch-1.0 libcamerasrc ! \
		video/x-raw,colorimetry=bt709,format=$TMP_FORMAT,width=1280,height=720,framerate=15/1 ! \
		jpegenc ! multipartmux ! \
		queue leaky=1 max-size-buffers=0 ! \
		tcpserversink host=0.0.0.0 port=8888


#gst-launch-1.0 -v libcamerasrc ! \
#		video/x-raw, format=RGBx, width=1920, height=1080, framerate=30/1 ! \
#		timeoverlay ! videoconvert ! video/x-raw,format=I420 ! x264enc ! \
#		rtph264pay config-interval=-1 ! \
#		udpsink host=192.168.2.1 port=5600 sync=false async=true
