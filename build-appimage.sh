#!/bin/bash
# Build AppImage for HaiClaude
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
APPDIR="$SCRIPT_DIR/AppDir"

# Ensure build exists
if [ ! -f "$BUILD_DIR/haiclaude" ]; then
    echo "Building HaiClaude..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build "$BUILD_DIR"
fi

# Download linuxdeploy if not present
LINUXDEPLOY="$SCRIPT_DIR/linuxdeploy-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY" ]; then
    echo "Downloading linuxdeploy..."
    wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
        -O "$LINUXDEPLOY"
    chmod +x "$LINUXDEPLOY"
fi

# Download linuxdeploy-qt plugin
LINUXDEPLOY_QT="$SCRIPT_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOY_QT" ]; then
    echo "Downloading linuxdeploy-qt plugin..."
    wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
        -O "$LINUXDEPLOY_QT"
    chmod +x "$LINUXDEPLOY_QT"
fi

# Clean and create AppDir
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# Copy executable
cp "$BUILD_DIR/haiclaude" "$APPDIR/usr/bin/"

# Copy desktop file
cp "$SCRIPT_DIR/haiclaude.desktop" "$APPDIR/usr/share/applications/"

# Copy icon (use 256x256 standard location)
cp "$SCRIPT_DIR/haiclaude-icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/haiclaude-icon.png"

# Create symlinks in AppDir root for linuxdeploy
ln -sf "$APPDIR/usr/share/applications/haiclaude.desktop" "$APPDIR/haiclaude.desktop"
ln -sf "$APPDIR/usr/share/icons/hicolor/256x256/apps/haiclaude-icon.png" "$APPDIR/haiclaude-icon.png"

# Build AppImage
echo "Creating AppImage..."
export OUTPUT="HaiClaude-x86_64.AppImage"
export NO_STRIP=1  # Disable stripping (avoids .relr.dyn section issues on newer distros)
export QMAKE="/usr/bin/qmake6"  # Use Qt6 qmake
cd "$SCRIPT_DIR"
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --output appimage

echo ""
echo "AppImage created: $SCRIPT_DIR/HaiClaude-x86_64.AppImage"
