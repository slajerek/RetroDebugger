# libusb (vendored)

Subset of libusb 1.0.30, used by the USBSID-Pico SID engine on macOS and
Windows. Linux builds link the system libusb-1.0 through pkg-config instead.

- Source: https://github.com/libusb/libusb/releases/download/v1.0.30/libusb-1.0.30.tar.bz2
- SHA-256: fea36f34f9156400209595e300840767ab1a385ede1dc7ee893015aea9c6dbaf
- License: LGPL-2.1-or-later, see `COPYING`

Files are unmodified copies from the release tarball:

- `libusb/`: core sources and headers
- `libusb/os/`: darwin, posix and windows (WinUSB, UsbDk) backends
- `Xcode/config.h`: build config for the Xcode project
- `msvc/config.h`: build config for the Visual Studio project

The directory lives outside `src/` on purpose: the Xcode target searches
`src/**` recursively, and `msvc/config.h` must never be visible to a
non-MSVC compile.

To update, copy the same files from a newer release tarball and update the
version and checksum above.
