#!/usr/bin/env python3
"""Writes the checksum of an Atari .car file's image into its header (RD-030).

    python3 -I tests/source-debug/scripts/car_checksum.py main.car

A .car is a 16-byte header ("CART", type, checksum, 0) and the raw image; the
checksum is the sum of the image bytes, 32 bits big-endian (Atari800's
CARTRIDGE_Checksum). ld65 writes the header with a zero checksum, which no
linker expression can compute.
"""
import struct
import sys


def main():
    path = sys.argv[1]
    with open(path, "rb") as f:
        data = bytearray(f.read())
    if len(data) < 16 or data[:4] != b"CART":
        sys.exit("%s: not a .car file" % path)
    data[8:12] = struct.pack(">I", sum(data[16:]) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(data)


if __name__ == "__main__":
    main()
