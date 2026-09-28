# USBSID-Pico-driver
This repo contains the driver code for USBSID-Pico in C, Java and JAvascript.

For more information about [USBSID-Pico](https://github.com/LouDnl/USBSID-Pico) visit the main [repo](https://github.com/LouDnl/USBSID-Pico).  

# C/C++ API
Usage information is available in [USBSID.h](src/USBSID.h).  
For native C applications refer to [USBSIDInterface.h](src/USBSIDInterface.h) which is a wrapper around the C++ functions.  
For using multiple boards at once refer to [USBSID_Manager.h](src/USBSID_Manager.h), which opens every attached board and addresses all their SIDs as one continuous range of SID numbers. Each board's SIDs follow that board's own configuration (its SID numbering, flipped/mixed socket presets included). Remapping them is up to the player/emulator settings or command line.  
For native C applications refer to [USBSIDManagerInterface.h](src/USBSIDManagerInterface.h) which is a wrapper around the multiboard C++ functions.  

# Java API
See the interface file for available functions [IUSBSID.java](java/usbsid-usb-driver-library-java/src/main/java/usbsid/IUSBSID.java).  

# Javascript
A Javascript driver example around [USBSID-Player](https://github.com/LouDnl/USBSID-Player) can be found in the main repo [examples dir](https://github.com/LouDnl/USBSID-Pico/examples/config-tool-web/uspico).  
Two older examples with jsSID are in the [Javascript](https://github.com/LouDnl/USBSID-Pico-driver/javascript).  
```shell
# to run them clone the repository and run the respective `web.py` file in the example directory.
git clone https://github.com/LouDnl/USBSID-Pico-driver

# change directory
cd USBSID-Pico-driver/javascript/webusb-jsSID
# or
cd USBSID-Pico-driver/javascript/webusb-jsSID-worker_driver

# run the example
python3 web.py

# open a webbrowser and go to http://localhost:8000
# click connect and select your board
# click the two dots .. to load the sample SID file
# auto play commences
```

# Usage
This driver code (or an adapted/ rewritten variant of it) is used in:
- [Vice](https://github.com/VICE-Team/svn-mirror/tree/main/vice/src/lib/libusbsiddrv)
- [libsidplayfp](https://github.com/libsidplayfp/libsidplayfp/tree/master/src/builders/usbsid-builder)
- [JSidplay2](https://sourceforge.net/p/jsidplay2/git/ci/master/tree/lib/usbsid/)
- [Denise](https://github.com/LouDnl/denise_usbsid/tree/master/deps/USBSID-Pico)
- [Acid64 Pro](https://acid64.com/download)
- [Phosphor](https://github.com/sandlbn/Phosphor)
  * Uses an in Rust rewritten variant [USBSID-Pico-Rust-driver](https://github.com/sandlbn/USBSID-Pico-Rust-driver)
- [Deepsid](https://github.com/Chordian/deepsid/tree/master/js/handlers)
- [USBSID-Player](https://github.com/LouDnl/USBSID-Player)
- [SidBerry (fork)](https://github.com/LouDnl/SidBerry)
- [gt2ultra (fork)](https://github.com/LouDnl/GTUltra-USBSID/tree/master/src/driver)
- [RetroDebugger (fork)](https://github.com/LouDnl/RetroDebugger/tree/master/platform/Linux/src.Linux/usbsid)
- [emudore embedded (fork of emudore)](https://github.com/LouDnl/emudore-embedded)
- goattracker2 (fork)
- sidfactory2 (fork)
