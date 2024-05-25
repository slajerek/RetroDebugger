#!/bin/sh
./../../../MTEngineSDL/other/tools/DeployMaker/Release/DeployMaker zlib reset_basic.snap
./../../../MTEngineSDL/other/tools/DeployMaker/Release/DeployMaker embed reset_basic.snap.zlib
rm -f ./../../src/Embedded/reset_basic_snap_zlib.h
cp reset_basic_snap_zlib.h ./../../src/Embedded
