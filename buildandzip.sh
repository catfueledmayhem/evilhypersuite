#!/bin/bash
set -e

# =========================
# Paths
# =========================
BUILD_DIR="build"
WIN_DIR="$BUILD_DIR/win64"
LINUX_DIR="$BUILD_DIR/linux"

ALONGSIDE_DIR="alongside"
RESOURCE_SUBDIR="resources"

# =========================
# Resource definitions
# =========================

# Everything that goes into resources/
COMMON_RESOURCES=(
    "$ALONGSIDE_DIR/$RESOURCE_SUBDIR"
)

# Files placed next to the executable
WIN_ALONGSIDE=(
    "$ALONGSIDE_DIR/WinDivert.dll"
    "$ALONGSIDE_DIR/WinDivert64.sys"
)

LINUX_ALONGSIDE=(
    # Example:
    # "$ALONGSIDE_DIR/libsomething.so"
)

# Exclusions (relative paths after copy)
WINDOWS_EXCLUDE=(
    "resources/app.manifest"
    "resources/app.rc"
    "resources/app.res"
)

LINUX_EXCLUDE=(
    "resources/app.manifest"
    "resources/app.rc"
    "resources/app.res"
)

# =========================
# Timing helpers
# =========================
timer_start=$(date +%s)
print_elapsed() {
    local start=$1
    local msg=$2
    echo "$msg (took $(( $(date +%s) - start ))s)"
}

# =========================
# Build
# =========================
echo "============================"
echo "Building Linux..."
start=$(date +%s)
make
print_elapsed $start "Linux build finished"

echo "Building Windows..."
start=$(date +%s)
make windows
print_elapsed $start "Windows build finished"

# =========================
# Prepare directories
# =========================
echo "============================"
echo "Preparing build directories..."
mkdir -p "$WIN_DIR" "$LINUX_DIR"

# =========================
# Copy common resources
# =========================
echo "Copying resources..."
for r in "${COMMON_RESOURCES[@]}"; do
    cp -r "$r" "$WIN_DIR/"
    cp -r "$r" "$LINUX_DIR/"
done

# =========================
# Copy alongside files
# =========================
echo "Copying alongside files..."

for f in "${WIN_ALONGSIDE[@]}"; do
    cp "$f" "$WIN_DIR/"
done

for f in "${LINUX_ALONGSIDE[@]}"; do
    cp "$f" "$LINUX_DIR/"
done

# =========================
# Apply exclusions
# =========================
echo "Applying Linux exclusions..."
for f in "${LINUX_EXCLUDE[@]}"; do
    rm -rf "$LINUX_DIR/$f" 2>/dev/null || true
done

echo "Applying Windows exclusions..."
for f in "${WINDOWS_EXCLUDE[@]}"; do
    rm -rf "$WIN_DIR/$f" 2>/dev/null || true
done

# =========================
# Zip
# =========================
echo "============================"
echo "Creating zip archives..."
start=$(date +%s)
cd "$BUILD_DIR"
zip -qr utility-win64.zip win64
zip -qr utility-linux-x86_64.zip linux
cd ..
print_elapsed $start "Zipping finished"

# =========================
# Done
# =========================
echo "============================"
echo "All done in $(( $(date +%s) - timer_start ))s"
