#!/bin/bash

#
# by TS, Aug 2023
#
# https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html
#

[[ -z "$COMPILE_WITH_GUI" ]] && {
	echo "Missing COMPILE_WITH_GUI. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "$COMPILE_WITH_FFMPEG" ]] && {
	echo "Missing COMPILE_WITH_FFMPEG. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "$COMPILE_WITH_GSTREAMER" ]] && {
	echo "Missing COMPILE_WITH_GSTREAMER. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "$NEED_LIBCAMERA" ]] && {
	echo "Missing NEED_LIBCAMERA. Aborting." >>/dev/stderr
	exit 1
}

OPENCV_RELEASE="opencv-4.8.0"

#

sudo apt-get update || exit 1
sudo apt-get install -y \
		g++ \
		cmake \
		make \
		git \
		pkg-config \
		libgnutls28-dev \
		libv4l-dev \
		|| exit 1

if [ ! -d "${OPENCV_RELEASE}" ]; then
	tar xf "${OPENCV_RELEASE}.tar.gz" || exit 1
fi

test -d build && rm -fr build
mkdir build && cd build

TMP_OPT_HIGHGUI="OFF"
if [ "$COMPILE_WITH_GUI" = "true" ]; then
	TMP_OPT_HIGHGUI="ON"
	sudo apt install -y libgtk2.0-dev || exit 1
fi

TMP_OPT_FFMPEG="OFF"
if [ "$COMPILE_WITH_FFMPEG" = "true" ]; then
	TMP_OPT_FFMPEG="ON"
	sudo apt-get install -y \
			libavcodec-dev \
			libavformat-dev \
			libavutil-dev \
			libswscale-dev \
			libavresample-dev \
			libavresample4 \
			libjpeg-dev \
			libopenjp2-7-dev \
			libpng-dev \
			|| exit 1
fi

TMP_OPT_GSTREAMER="OFF"
if [ "$COMPILE_WITH_GSTREAMER" = "true" ]; then
	TMP_OPT_GSTREAMER="ON"
	sudo apt-get install -y \
			libgstreamer1.0-dev \
			libgstreamer-plugins-base1.0-dev \
			libgstreamer-plugins-bad1.0-dev \
			gstreamer1.0-plugins-base \
			gstreamer1.0-plugins-base-apps \
			gstreamer1.0-plugins-good \
			gstreamer1.0-plugins-bad \
			gstreamer1.0-plugins-ugly \
			gstreamer1.0-libav \
			gstreamer1.0-tools \
			gstreamer1.0-alsa \
			gstreamer1.0-gl \
			gstreamer1.0-pulseaudio \
			|| exit 1
	if [ "$COMPILE_WITH_GUI" = "true" ]; then
		sudo apt install -y \
				gstreamer1.0-x \
				gstreamer1.0-gtk3 \
				gstreamer1.0-qt5 \
				|| exit 1
	fi
fi

if [ "$NEED_LIBCAMERA" = "true" ]; then
	sudo apt-get install -y libcamera-dev || exit 1
fi

cmake \
		-D WITH_LIBV4L=ON \
		-D WITH_V4L=ON \
		-D WITH_FFMPEG=$TMP_OPT_FFMPEG \
		-D WITH_GSTREAMER=$TMP_OPT_GSTREAMER \
		-D WITH_MSMF=OFF \
		-D WITH_DSHOW=OFF \
		-D WITH_AVFOUNDATION=OFF \
		-D WITH_1394=OFF \
		-D WITH_ANDROID_MEDIANDK=OFF \
		-D VIDEOIO_ENABLE_PLUGINS=OFF \
		-DBUILD_TESTS=OFF \
		-DBUILD_PERF_TESTS=OFF \
		-DBUILD_opencv_calib3d=OFF \
		-DBUILD_opencv_dnn=OFF \
		-DBUILD_opencv_highgui=$TMP_OPT_HIGHGUI \
		-DBUILD_opencv_java=OFF \
		-DBUILD_opencv_js=OFF \
		-DBUILD_opencv_ml=OFF \
		-DBUILD_opencv_objc=OFF \
		-DBUILD_opencv_python=OFF \
		"../$OPENCV_RELEASE" || exit 1

#

make -j4 || exit 1
sudo make install
