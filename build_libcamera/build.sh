#!/bin/bash

#
# Building libcamera appears to be unnecessary on Rasp Pi 5 with Debian 12.5 Bookworm
#
# by TS, Aug 2023, Apr 2024
#

exit 0

sudo apt-get install -y \
		g++ \
		libyaml-dev python3-yaml python3-ply python3-jinja2 \
		libgnutls28-dev \
		libudev-dev \
		libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
		libevent-dev libdrm-dev libjpeg-dev libsdl2-dev \
		ninja-build pkg-config python3-pip meson \
		|| exit 1

test -d libcamera && rm -fr libcamera
git clone https://git.libcamera.org/libcamera/libcamera.git || exit 1

cd libcamera || exit 1

echo -e "\n\nBUILD\n\n"
meson setup build || exit 1

echo -e "\n\nINSTALL\n\n"
sudo ninja -C build install || exit 1

sudo ldconfig
