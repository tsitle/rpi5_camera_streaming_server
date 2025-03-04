#!/usr/bin/env bash

if ! test -d build; then
	mkdir build
	cd build || exit 1
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../src || exit 1
	cd ..
fi
cd build || exit 1
make || exit 1
