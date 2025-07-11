#!/usr/bin/env bash
set -euo pipefail

# Default settings; override via environment or args as you like
BUILD_DIR=build
CPU=${APPLE_CPU:-apple-m1}
PGO=${USE_PGO:-OFF}
CONFIG=${CONFIGURATION:-Release}   # or Debug
CLEAN_BUILD="false"

# Allow passing overrides on the command line:
while [[ $# -gt 0 ]]; do
  case $1 in
    --cpu)
      if [[ -z ${2:-} ]]; then
        echo "Error: --cpu requires an argument" >&2
        exit 1
      fi
      CPU=$2
      shift 2
      ;;
    --pgo)    PGO=ON; shift    ;;
    --debug)  CONFIG=Debug; shift ;;
    --release)CONFIG=Release; shift ;;
    --clean)  CLEAN_BUILD=true; shift ;;
    *)        echo "Unknown option: $1"; exit 1 ;;
  esac
done

echo "Configuring for CPU=$CPU, PGO=$PGO, Config=$CONFIG"
if [[ "$CLEAN_BUILD" == "true" ]]; then
  echo "Cleaning build directory..."
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR"
trap 'popd' EXIT
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
  $( [[ "$OSTYPE" == darwin* ]] && echo "-DAPPLE_CPU=$CPU" ) \
  -DUSE_PGO="$PGO" \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  -DENABLE_WERROR=OFF

-cmake --build . -- -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
+cmake --build . -- -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

echo "Build complete in ./$BUILD_DIR/"
