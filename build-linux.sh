#!/bin/bash
CURRENT_DIR=`pwd`

# MTEngineSDL
cd $CURRENT_DIR/../
if [ ! -d "MTEngineSDL" ]; then
	echo -e "\e[94mCloning \e[31mMTEngineSDL \e[94mlibrary repository\e[0m"
	git clone https://github.com/slajerek/MTEngineSDL.git
	echo -e ""
fi

echo -e "\e[94mCompiling \e[31mMTEngineSDL \e[94mlibrary\e[0m"
mkdir -p $CURRENT_DIR/../MTEngineSDL/build
cd $CURRENT_DIR/../MTEngineSDL/build
cmake ../
make -j$(nproc) MTEngineSDL

# uSockets
cd $CURRENT_DIR/../
if [ ! -d "uSockets" ]; then
	echo -e "\n\e[94mCloning \e[31muSockets \e[94mlibrary repository\e[0m"
	git clone https://github.com/uNetworking/uSockets.git
fi

echo -e "\n\e[94mCompiling \e[31muSockets \e[94mlibrary\e[0m"
cd uSockets
make -j$(nproc)
mkdir -p $CURRENT_DIR/../MTEngineSDL/platform/Linux/libs/
cp -f uSockets.a $CURRENT_DIR/../MTEngineSDL/platform/Linux/libs/

# RetroDebugger
echo -e "\n\e[94mCompiling \e[31mRetroDebugger\e[0m"
mkdir -p $CURRENT_DIR/build
cd $CURRENT_DIR/build
cmake ../
make -j$(nproc) retrodebugger

if [ -f "$CURRENT_DIR/build/retrodebugger" ]; then
	echo -e "\n\e[1;92mRetroDebugger compiled successfully. Binary is in ./build folder.\e[0m"
else
	echo -e "\n\e[1;91mFailed to build RetroDebugger.\e[0m"
fi 
