#!/usr/bin/env python3
"""
Read the hardware variant ("sub_model") of a Mi Bedside Lamp 2 from a full
flash backup of the original firmware (made with: esptool read_flash 0x0 0x400000 backup.bin).

The stock firmware stores it in its "minvs" key/value area at 0x3f8000,
under the key "app_data.sub_model":

    0 (or missing) -> hardware_variant: default
    1              -> hardware_variant: hongli

Usage: python3 read_sub_model.py backup.bin
"""
import struct
import sys

MINVS_ADDR = 0x3F8000
MINVS_SIZE = 0x4000
KEY = b"app_data.sub_model"
VARIANTS = {0: "default", 1: "hongli"}


def find_sub_model(area):
    found = None
    i = area.find(KEY)
    while i >= 16:
        h = area[i - 16:i]
        if h[0:2] == b"\x55\xaa" and h[12] == len(KEY):
            vlen = struct.unpack("<H", h[8:10])[0]
            value = area[i + len(KEY):i + len(KEY) + vlen]
            current = h[2:4] == b"\xff\xff"
            if value.isdigit() and (current or found is None or not found[1]):
                found = (int(value), current)
        i = area.find(KEY, i + 1)
    return found


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    data = open(sys.argv[1], "rb").read()
    if len(data) < MINVS_ADDR + MINVS_SIZE:
        sys.exit(f"File is {len(data)} bytes; expected a full 4 MB flash backup.")
    found = find_sub_model(data[MINVS_ADDR:MINVS_ADDR + MINVS_SIZE])
    if found is None:
        print("No app_data.sub_model record found -> stock firmware uses sub_model 0")
        print("Use:  hardware_variant: default")
        return
    value, current = found
    print(f"app_data.sub_model = {value}" + ("" if current else " (from a superseded record)"))
    print(f"Use:  hardware_variant: {VARIANTS.get(value, f'unknown ({value})')}")


if __name__ == "__main__":
    main()
