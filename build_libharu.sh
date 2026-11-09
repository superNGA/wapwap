#!/bin/bash

# Builds libharu into a static library so our program can link against it statically at compile time

LIBHARU="./lib/libharu/src"
INC_HARU="./lib/libharu/include"
INC_HARUCONFIG="./build/include"
GCC_BUILD_DIR="./build/libharu_obj"
LIB_OUTPUT="./build/libharu.a"

# fix directory
cd "$(dirname ${BASH_SOURCE[0]})"
echo "Directory fixed to : $PWD"

# libharu must exist
if [ ! -d "$LIBHARU" ]; then
	echo "$LIBHARU directory not found."
	exit 1
fi
echo "$LIBHARU found"

# Assert temp directtory
mkdir -p "$GCC_BUILD_DIR"

# this macro is required for libharu to build.
mkdir -p "$INC_HARUCONFIG"
echo "#define LIBHPDF_HAVE_ZLIB" > "$INC_HARUCONFIG/hpdf_config.h"

# Build each libharu file
for file in $LIBHARU/*.c; do
	# Make sure the file exists on the filesystem (check by -e)
	if [ ! -e "$file" ]; then
		continue
	fi

	FILENAME=$(basename "$file" .c) # Extract filename. ex: ./libharu/src/file.c -> file
	gcc -c -O3 -I"$INC_HARU" -I"$INC_HARUCONFIG" "$file" -o "$GCC_BUILD_DIR/$FILENAME.o"
	echo "Compiled $file"
done

# Combine all libharu .obj files to a archive
ar rcs "$LIB_OUTPUT" "$GCC_BUILD_DIR"/*.o
if [ ! -e "$LIB_OUTPUT" ]; then
	echo "Failed to build libharu .obj files into static library at : $LIB_OUTPUT"
	exit 1
fi

echo "Static library build at $LIB_OUTPUT"
