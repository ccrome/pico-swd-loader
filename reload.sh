#!/bin/bash
set -e
ROOTDIR=`pwd`

# Build target firmware
echo "Building target firmware..."
cd target-sdk
rm -rf build && mkdir build && cd build
export PICO_SDK_PATH=$ROOTDIR/pico-sdk
cmake ..
make -j$(nproc)
echo "✓ Target firmware built"

# Convert ELF to header
echo "Converting ELF to header..."
python3 ../../host/elf_to_header.py ram_blink.elf ../../host/target_firmware.h
echo "✓ Header generated"

# Build host loader
echo "Building host loader... `pwd`"
mkdir -p $ROOTDIR/host/build
cd $ROOTDIR/host/build
cmake ..
make -j$(nproc) > /dev/null
echo "✓ Host loader built"

# Copy to host Pico if mounted
if [ -d /media/$USER/RPI-RP2 ]; then
    echo "Copying to host Pico..."
    cp swd_loader.uf2 /media/$USER/RPI-RP2/
    echo "✓ Copied! Host Pico will reboot and load firmware."
else
    echo "⚠ Host Pico not mounted. Put it in bootloader mode and run:"
    echo "  cp host/build/swd_loader.uf2 /media/caleb/RPI-RP2/"
fi

echo ""
echo "Done! Connect to host Pico's serial port to see output."

