#!/bin/bash
# Script to fix "damaged" message on macOS

APP_NAME="RCL Executor.app"

if [ ! -d "/Applications/$APP_NAME" ]; then
    echo "Error: $APP_NAME not found in /Applications."
    echo "Please move the app to /Applications first."
    exit 1
fi

echo "Fixing '$APP_NAME'..."
sudo xattr -rd com.apple.quarantine "/Applications/$APP_NAME"
sudo xattr -cr "/Applications/$APP_NAME"
sudo codesign --force --deep --sign - "/Applications/$APP_NAME"

echo "Done! You can now open the app."
