#!/bin/bash

OUTPUT_DIR="./build/wapwap.out"
LIBHARU_SLIB="./build/libharu.a"
LIBHARU_BUILDSCRIPT="./build_libharu.sh"
FLAGS="-g"
# FLAGS="-fsanitize=address"

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
	./src/config/config.cpp
	./src/tasks/tasks.cpp
	./src/tasks/file_entity.cpp
	)

g++ "$FLAGS" "${SOURCES[@]}" "$LIBHARU_SLIB" -lz -o "$OUTPUT_DIR"
# g++ "$FLAGS" "${SOURCES[@]}" -lhpdf -lz -o "$OUTPUT_DIR" <- this one lets us read C code in gdb
if [ ! -e "$OUTPUT_DIR" ]; then
	echo "Failed build"
	exit 1
fi
echo "Build done"
