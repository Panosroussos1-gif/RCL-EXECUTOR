#!/bin/bash

# .RCL EXECUTOR - MASTER SETUP SCRIPT
# This script automates everything to make it "One-Click" like MacSploit.

echo "------------------------------------------"
echo "🚀 INITIALIZING .RCL INTERNAL ENGINE"
echo "------------------------------------------"

PROJECT_DIR="/Users/User/Documents/trae_projects/robloc studio"
cd "$PROJECT_DIR"

# 1. Clean up
echo "[1/4] Clearing old temporary files..."
rm -rf /tmp/rcl_temp

# 2. Build Binaries
echo "[2/4] Building Internal Engine for M5..."
mkdir -p bin
clang++ -dynamiclib -arch arm64 -fPIC -std=c++17 src/internal/main.cpp -o bin/rcl_internal.dylib -lstdc++
clang++ -arch arm64 -std=c++17 src/internal/loader.cpp -o bin/rcl_loader -lstdc++

# 3. Sign and Permissions
echo "[3/4] Signing binaries and fixing permissions..."
codesign --force --deep --sign - bin/rcl_internal.dylib
codesign --force --deep --sign - bin/rcl_loader
chmod +x bin/rcl_loader
chmod +x src/internal/insert_dylib

# 4. Launch
echo "[4/4] Launching Executor..."
echo "------------------------------------------"
echo "✅ SETUP COMPLETE! HAPPY SCRIPTING."
echo "------------------------------------------"

npm start
