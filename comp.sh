#!/bin/bash

test -d build
if [ $? -ne 0 ]; then
	mkdir build
	cd build
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../src || exit 1
	cd ..
fi
cd build
make || exit 1
