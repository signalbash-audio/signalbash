#!/bin/bash
set -e

cmake -B build-macos \
    -DCMAKE_BUILD_TYPE=Release \
    -DMACOS_RELEASE=ON \
    -D"CMAKE_OSX_ARCHITECTURES=arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11

cmake --build build-macos --config Release --target Signalbash_VST3  -j8
cmake --build build-macos --config Release --target Signalbash_AU -j8
cmake --build build-macos --config Release --target Signalbash_CLAP -j8

