#!/bin/bash

#
# by TS, Sep 2023
#

sudo apt-get install -y \
		g++ \
		libyaml-dev python3-yaml python3-ply python3-jinja2 \
		libgnutls28-dev \
		libudev-dev \
		libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
		libevent-dev libdrm-dev libjpeg-dev libsdl2-dev \
		ninja-build pkg-config \
		|| exit 1

sudo pip3 install --system meson || exit 1
sudo pip3 install --system --upgrade meson || exit 1

test -d libcamera && rm -fr libcamera
git clone https://git.libcamera.org/libcamera/libcamera.git || exit 1

cd libcamera || exit 1

echo -e "\n\nBUILD\n\n"
meson setup build || exit 1

echo -e "\n\nINSTALL\n\n"
sudo ninja -C build install || exit 1

sudo ldconfig
