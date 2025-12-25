#!/bin/bash
set -e

# Build target firmware
echo "Building target firmware..."
cd target-sdk
rm -rf build && mkdir build && cd build
export PICO_SDK_PATH=/home/caleb/rpi-loader/pico-sdk
cmake .. > /dev/null
make -j$(nproc) > /dev/null
echo "✓ Target firmware built"

# Convert ELF to header
echo "Converting ELF to header..."
python3 ../../host/elf_to_header.py ram_blink.elf ../../host/target_firmware.h
echo "✓ Header generated"

# Build host loader
echo "Building host loader..."
cd ../../host/build
make -j$(nproc) > /dev/null
echo "✓ Host loader built"

# Copy to host Pico if mounted
if [ -d /media/caleb/RPI-RP2 ]; then
    echo "Copying to host Pico..."
    cp swd_loader.uf2 /media/caleb/RPI-RP2/
    echo "✓ Copied! Host Pico will reboot and load firmware."
else
    echo "⚠ Host Pico not mounted. Put it in bootloader mode and run:"
    echo "  cp host/build/swd_loader.uf2 /media/caleb/RPI-RP2/"
fi

echo ""
echo "Done! Connect to host Pico's serial port to see output."

