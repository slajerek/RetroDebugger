#!/bin/sh
set -e
xcodebuild clean -project c64d.xcodeproj -scheme "Retro Debugger" -configuration Debug
xcodebuild -project c64d.xcodeproj -scheme "Retro Debugger" -configuration Debug clean build | xcpretty -r json-compilation-database --output compile_commands.json
