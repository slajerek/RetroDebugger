#!/bin/bash
# note this does not work due to changes in paths

mkdir -p build
cd build
mkdir -p sdl2
mkdir -p MTEngineSDL
mkdir -p c64d

cd sdl2
cmake ../../MTEngineSDL/other/lib/SDL2-2.0.10-static/
make -j$(nproc) SDL2-static

cd ../MTEngineSDL
cmake ../../MTEngineSDL/
make -j$(nproc) MTEngineSDL

cd ../c64d
cmake ../../c64d/
make -j$(nproc) RetroDebugger

