#!/bin/bash

OUTPUT_DIR="./build/wapwap.out"
LIBHARU_SLIB="./build/libharu.a"
LIBHARU_BUILDSCRIPT="./build_libharu.sh"

# fix directory
cd "$(dirname ${BASH_SOURCE[0]})"
echo "Directory fixed to : $PWD"


# if libharu.a doesn't exist. built it
if [ ! -e "$LIBHARU_SLIB" ]; then
	echo "libharu static lib not found. Building using $LIBHARU_BUILDSCRIPT"
	bash "$LIBHARU_BUILDSCRIPT"

	if [ ! -e "$LIBHARU_SLIB" ]; then
		echo "Failed to build libharu static library"
		exit 1
	fi
	echo "libharu static lib build"
fi
echo "libharu static library found"


SOURCES=(
	./src/main.cpp
	./src/config/config.c
	./src/tasks/tasks.c
	)

g++ "${SOURCES[@]}" "$LIBHARU_SLIB" -lz -o "$OUTPUT_DIR"
if [ ! -e "$OUTPUT_DIR" ]; then
	echo "Failed build"
	exit 1
fi
echo "Build done"
