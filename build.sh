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

# Detect C++23 flag for Apple Clang
CXX_FLAGS=""
if [[ "$OSTYPE" == darwin* ]]; then
  if clang++ --version 2>/dev/null | grep -q "Apple clang"; then
    echo "Detected Apple Clang - using -std=c++2b for C++23 support"
    CXX_FLAGS="-DCMAKE_CXX_FLAGS=-std=c++2b"
  else
    echo "Using standard C++23 flag"
    CXX_FLAGS="-DCMAKE_CXX_FLAGS=-std=c++23"
  fi
else
  echo "Using standard C++23 flag"
  CXX_FLAGS="-DCMAKE_CXX_FLAGS=-std=c++23"
fi

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
  -DENABLE_WERROR=OFF \
  $CXX_FLAGS

cmake --build . -- -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

echo "Build complete in ./$BUILD_DIR/"
