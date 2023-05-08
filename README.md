# Hello and welcome to the Retro Debugger!

This is Commodore 64, Atari XL/XE and NES code and memory debugger 
that works in real time. It is quick prototyping tool where you 
can play with 8-bit machine and its internals.

Note that this is not ready product, this is still beta version.

If you are starting this for the first time and looking for Atari or
Commodore, then you first need to setup your own ROMs of these retro
computers. You can find that in the Settings menu. 

ROMS: 
```
File Size Name
     2048 ATARI5200.rom
     8192 ATARIBASIC.rom
    10240 ATARIOSA.rom
    10240 ATARIOSB.rom
    16384 ATARIXL.rom
     8192 basic
     4096 chargen
    16384 dos1541
    16384 dos1541II
     8192 kernal
```


Retro Debugger embeds Vice v3.1, Atari800 and NestopiaUE 
emulator engines provided by:

Vice: https://sourceforge.net/projects/vice-emu/
Atari800: http://atari800.github.io
NestopiaUE: http://0ldsk00l.ca/nestopia/

Thank you for that awesome part of code!

# Usage

The README usage file for this version is not ready yet.
Please refer to the old C64 65XE NES Debugger documentation in ./docs folder.
https://sourceforge.net/p/c64-debugger/code/ci/master/tree/MTEngine/Assets/README.txt

Note that you can right-click now on some of the views to display context menu.

# How to compile

Code is based on MTEngineSDL.
Engine compiles SDL2 with ImGui and this app as a static binary.
You need to compile the MTEngineSDL first.

MTEngineSDL: https://github.com/slajerek/MTEngineSDL
SDL2: https://github.com/libsdl-org/SDL
ImGui: https://github.com/imgui

## macOS

cd ./platform/MacOS
I normally put files into `~/develop/c64d` folder. 
Project should compile as is in Xcode, remember to reference MTEngineSDL
library.

## Windows

Check VS2019 project in `./platform/Windows`. This should work when put into
`C:\develop\c64d`. Note, the SDL2 library was built static.

## Linux

You need SDL2 and GLEW installed:
`sudo apt install libsdl2-dev`
`sudo apt install libglew-dev`

Put MTEngineSDL and RetroDebugger folders into the same folder and then compile.

```
cd RetroDebugger
mkdir build
cd build
cmake ./../
make
```

Remember to have MTEngineSDL library in `./../../MTEngineSDL` folder.

# Thanks

This product would not have been created without the help of alpha testers:
Euan Gamble, Robert Troughton, Jesper Rune Larsen, Steve West, Lukhash, 
Markus Dano Burgstaller, Brush/Elysium, Alex Goldblat, Cescom, Artix, 
Isildur/Samar, Dkt/Samar, Mojzesh/Samar, Pajero/MadTeam^Samar, 
Euan Gamble, Tebe, Linus Nielsen Boogaloo/Horizon, zero211, 
Mr.Mouse/XeNTaX/Genesis Project, Yugorin/Samar

And C64 65XE NES Debugger testers: Scan/House, Dr.J/Delysid, Mr Wegi/Elysium, 
ElfKaa/Avatar, Ruben Aparicio, 64 bites, Stein Pedersen, 
Mads Nielsen (Slammer/Camelot), Roy C Riggs (furroy)

Also, the famous SDL2 and ImGui team!

*Plus everyone who made a donation, you know who you are and you are awesome!*

# Promo videos

See promo videos here: 
https://youtu.be/Xu6EknKA7GE
https://youtu.be/Lxd296tDdoo
https://youtu.be/_s6s7qnXBx8

# Beer Donation

If you like this tool and you feel that you would like to share with me
some beers, then you can use this link: https://tinyurl.com/C64Debugger-PayPal

Or send me some Bitcoins using this address:
`1G3ZRT7j27QycHnkoo176t9j5a2J49fsXc`

Donations will help me in development, thanks!

# Facebook page

Join Retro Debugger Facebook page here: https://tinyurl.com/RetroDebugger-Facebook

# License

Retro Debugger is (C) Marcin Skoczylas

The license for Retro Debugger is some kind of FSF license as decided by
Richard Stallman when I discussed that with him during CodeEurope conference.
This tool is free to use.

Please refer to C64 65XE NES Debugger license for more details.
https://sourceforge.net/projects/c64-debugger/

C64 65XE NES Debugger (C) 2016 Marcin Skoczylas
Vice (C) 1993 The VICE Team
65XE Debugger (C) 2018 Marcin Skoczylas
Atari800 emulator (C) The Atari800 emulator Team
NestopiaUE emulator (C) The NestopiaUE and Nestopia teams.
GoatTracker 2 is (C) Lasse Öörni and GoatTracker 2 team.
ASAP library is (C) Piotr Fusik et al.

This product uses 1-Writer font: http://home-2002.code-cop.org/c64/font_01.html
UI assets licenses are provided by the ImGui, SDL2 and all referenced licenses.

Roboto-Medium.ttf, by Christian Robetson 
Apache License 2.0 
https://fonts.google.com/specimen/Roboto

Cousine-Regular.ttf, by Steve Matteson 
Digitized data copyright (c) 2010 Google Corporation. 
Licensed under the SIL Open Font License, Version 1.1 
https://fonts.google.com/specimen/Cousine

DroidSans.ttf, by Steve Matteson 
Apache License 2.0 
https://www.fontsquirrel.com/fonts/droid-sans

ProggyClean.ttf, by Tristan Grimmer 
MIT License 
(recommended loading setting: Size = 13.0, GlyphOffset.y = +1) 
http://www.proggyfonts.net/

ProggyTiny.ttf, by Tristan Grimmer 
MIT License 
(recommended loading setting: Size = 10.0, GlyphOffset.y = +1) 
http://www.proggyfonts.net/

Karla-Regular.ttf, by Jonathan Pinhorn 
SIL OPEN FONT LICENSE Version 1.1

imgui-notify by patrickcjk


CIAO!
