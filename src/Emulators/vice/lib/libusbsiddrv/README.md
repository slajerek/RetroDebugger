# libusbsiddrv

USBSID-Pico host driver, unmodified copy of `src/` from
[USBSID-Pico-driver](https://github.com/LouDnl/USBSID-Pico-driver) (commit `7134197`), the same
files VICE ships in `src/lib/libusbsiddrv/`.

Do not edit here. Fix in USBSID-Pico-driver first, then re-copy `USBSID.cpp`, `USBSID.h`,
`USBSIDInterface.cpp`, `USBSIDInterface.h`, `USBSID_Manager.cpp`, `USBSID_Manager.h`,
`USBSIDManagerInterface.cpp`, `USBSIDManagerInterface.h` and `LICENSE`.

Requires libusb-1.0. Only `USBSIDInterface.h` (C ABI, single board) is used by the emulator
glue. The multiboard `USBSID_Manager` files are built as well, as in VICE, but not used yet.
