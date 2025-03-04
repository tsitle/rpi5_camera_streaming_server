#!/usr/bin/env bash

# Starting with libcamera-0.4 there should be no need for building a custom
# version of the libcamera Gstreamer plugin anymore.
# Support for setting the Autofocus Mode is now implemented in the official release
# of the library.
# It can now be set in a GStreamer pipeline like this: 'libcamerasrc af-mode=continuous [...]'
exportCustomBuiltGstreamerPluginPath() {
	#CUSTOM_GSTREAMER_PLUGIN_PATH="~/test-opencv_mpjeg_stream-cpp/build_libcamera/libcamera/build/src/gstreamer"
	CUSTOM_GSTREAMER_PLUGIN_PATH="/usr/local/lib/arm-linux-gnueabihf/gstreamer-1.0/libgstlibcamera.so"
	export GST_PLUGIN_PATH="$CUSTOM_GSTREAMER_PLUGIN_PATH"
}
#exportCustomBuiltGstreamerPluginPath

./comp.sh || exit 1

test -d yamls || mkdir yamls
test -d yamls || {
	echo "Cannot create dir 'yamls'. Aborting" >>/dev/stderr
	exit 1
}

./build/HttpCamServer "$@"
