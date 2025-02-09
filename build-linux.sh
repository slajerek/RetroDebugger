#!/bin/bash
# note this does not work due to changes in paths

mkdir -p build
cd build
mkdir -p MTEngineSDL
mkdir -p c64d

cd ../MTEngineSDL
cmake ../../MTEngineSDL/
make -j$(nproc) MTEngineSDL

cd ../c64d
cmake ../../c64d/
make -j$(nproc) RetroDebugger

