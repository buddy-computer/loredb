#!/usr/bin/env bash
set -euo pipefail

# Default settings; override via environment or args as you like
BUILD_DIR=build
CPU=${APPLE_CPU:-apple-m1}
PGO=${USE_PGO:-OFF}
CONFIG=${CONFIGURATION:-Release}   # or Debug

# Allow passing overrides on the command line:
while [[ $# -gt 0 ]]; do
  case $1 in
    --cpu)    CPU=$2; shift 2 ;;
    --pgo)    PGO=ON; shift    ;;
    --debug)  CONFIG=Debug; shift ;;
    --release)CONFIG=Release; shift ;;
    *)        echo "Unknown option: $1"; exit 1 ;;
  esac
done

echo "Configuring for CPU=$CPU, PGO=$PGO, Config=$CONFIG"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR"

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DAPPLE_CPU="$CPU" \
  -DUSE_PGO="$PGO" \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  -DENABLE_WERROR=OFF

cmake --build . -- -j$(sysctl -n hw.logicalcpu)

popd
echo "Build complete in ./$BUILD_DIR/"
