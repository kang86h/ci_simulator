#!/usr/bin/env bash
set -euo pipefail

# Run from repo root
cd "$(dirname "$0")/.."

VCPKG_TRIPLET="${VCPKG_TRIPLET:-x64-osx}"
VCPKG_ROOT="${VCPKG_ROOT:-$PWD/vcpkg}"
BUILD_DIR="${BUILD_DIR:-build-mac}"
export VCPKG_DEFAULT_TRIPLET="$VCPKG_TRIPLET"
export VCPKG_ROOT
export CMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# Ensure vcpkg is present
if [ ! -x "$VCPKG_ROOT/vcpkg" ]; then
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
  "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
fi

# Dependencies
"$VCPKG_ROOT/vcpkg" install portaudio --triplet "$VCPKG_TRIPLET"

# Configure (clean if cache exists)
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
  rm -rf "$BUILD_DIR"
fi

cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
  -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET" \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build "$BUILD_DIR" --parallel

# Locate executable
EXE="$BUILD_DIR/ci_rt_sim"
if [ ! -x "$EXE" ] && [ -x "$BUILD_DIR/Release/ci_rt_sim" ]; then
  EXE="$BUILD_DIR/Release/ci_rt_sim"
fi

if [ ! -x "$EXE" ]; then
  echo "[ERROR] Executable not found." >&2
  exit 1
fi

# Run with defaults
"$EXE" --electrodes 22 --fmin 300 --fmax 7200 --lambda 0 --env-sr 250 --env-lpf 160 --sample-rate 48000 --block-frames 256

