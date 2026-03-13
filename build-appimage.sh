#!/bin/bash
# Build AppImage for HaiClaude
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
APPDIR="$SCRIPT_DIR/AppDir"
QT6_PLUGINS="/usr/lib/qt6/plugins"

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
mkdir -p "$APPDIR/usr/plugins/platformthemes"

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

# Bundle additional platform theme plugins for system theming
echo "Adding platform theme plugins..."
mkdir -p "$APPDIR/usr/plugins/platformthemes"
if [ -f "$QT6_PLUGINS/platformthemes/libqgtk3.so" ]; then
    cp "$QT6_PLUGINS/platformthemes/libqgtk3.so" "$APPDIR/usr/plugins/platformthemes/"
fi

# Rename binary so wrapper can call it
mv "$APPDIR/usr/bin/haiclaude" "$APPDIR/usr/bin/haiclaude.bin"

# Create wrapper script for theme support
cat > "$APPDIR/usr/bin/haiclaude" << 'WRAPPER'
#!/bin/bash
# Wrapper with theme support
APPDIR="$(dirname "$(dirname "$(readlink -f "$0")")")"
export QT_PLUGIN_PATH="$APPDIR/usr/plugins:$QT_PLUGIN_PATH"

# Auto-detect platform theme if not set
if [ -z "$QT_QPA_PLATFORMTHEME" ]; then
    if [ -n "$XDG_CURRENT_DESKTOP" ]; then
        case "$XDG_CURRENT_DESKTOP" in
            *GNOME*|*gnome*|*Ubuntu*|*Pop*)
                export QT_QPA_PLATFORMTHEME=gtk3
                ;;
            *KDE*|*kde*|*Plasma*)
                export QT_QPA_PLATFORMTHEME=kde
                ;;
            *)
                # Try gtk3 as fallback for most desktops
                export QT_QPA_PLATFORMTHEME=gtk3
                ;;
        esac
    else
        export QT_QPA_PLATFORMTHEME=gtk3
    fi
fi

exec "$APPDIR/usr/bin/haiclaude.bin" "$@"
WRAPPER
chmod +x "$APPDIR/usr/bin/haiclaude"

# Rebuild AppImage with theme support
echo "Rebuilding AppImage with theme support..."
rm -f "$SCRIPT_DIR/HaiClaude-x86_64.AppImage"
"$LINUXDEPLOY" --appdir "$APPDIR" --output appimage

echo ""
echo "AppImage created: $SCRIPT_DIR/HaiClaude-x86_64.AppImage"
