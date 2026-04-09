#!/bin/bash

# Define directories
SRC_DIR="./src/internal"
OUT_DIR="./bin"
DYLIB_NAME="rcl_internal.dylib"

# Create output directory
mkdir -p "$OUT_DIR"

echo "------------------------------------------"
echo "[BUILD] Compiling RCL Internal for M-Series..."
echo "------------------------------------------"

# Compile to a Dynamic Library (.dylib)
# -dynamiclib: Create a dylib
# -arch arm64: Target M-series chips
# -o: Output filename
# -lstdc++: Link standard C++ library
# -fPIC: Position Independent Code (Required for dylibs)
clang++ -dynamiclib \
        -arch arm64 \
        -fPIC \
        -std=c++17 \
        "$SRC_DIR/main.cpp" \
        -o "$OUT_DIR/$DYLIB_NAME" \
        -lstdc++

if [ $? -eq 0 ]; then
    echo "[SUCCESS] Dylib created at $OUT_DIR/$DYLIB_NAME"
    
    # Ad-hoc sign the dylib (Required for macOS to load it)
    echo "[SIGN] Ad-hoc signing the library..."
    codesign --force --deep --sign - "$OUT_DIR/$DYLIB_NAME"
    
    # Compile the Loader
    echo "[BUILD] Compiling RCL Loader..."
    clang++ -arch arm64 \
            -std=c++17 \
            "$SRC_DIR/loader.cpp" \
            -o "$OUT_DIR/rcl_loader" \
            -lstdc++
    
    # Ad-hoc sign the loader
    codesign --force --deep --sign - "$OUT_DIR/rcl_loader"
    
    if [ $? -eq 0 ]; then
        echo "[SUCCESS] Signed successfully."
        echo "------------------------------------------"
        echo "To use internal mode:"
        echo "1. Run: ./bin/rcl_loader"
        echo "------------------------------------------"
    else
        echo "[ERROR] Signing failed."
    fi
else
    echo "[ERROR] Compilation failed."
fi
