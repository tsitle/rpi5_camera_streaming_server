#!/bin/bash

test -d build
if [ $? -ne 0 ]; then
	mkdir build
	cd build
	cmake .. || exit 1
	cd ..
fi
cd build
make || exit 1
