#!/bin/bash
set -e
ROOTDIR=`pwd`

# Check for target parameter
if [ -z "$1" ]; then
    echo "Usage: $0 <target-project>"
    echo ""
    echo "Available targets:"
    echo "  target-blink    - Blink LED example"
    echo "  target-usb-cdc   - USB CDC loopback example"
    exit 1
fi

TARGET_PROJECT=$1

# Validate target project exists
if [ ! -d "$TARGET_PROJECT" ]; then
    echo "Error: Target project '$TARGET_PROJECT' not found"
    echo "Available targets: target-blink, target-usb-cdc"
    exit 1
fi

# Determine ELF filename based on target
case $TARGET_PROJECT in
    target-blink)
        ELF_NAME="ram_blink.elf"
        ;;
    target-usb-cdc)
        ELF_NAME="ram_usb_cdc.elf"
        ;;
    *)
        echo "Error: Unknown target project '$TARGET_PROJECT'"
        echo "Available targets: target-blink, target-usb-cdc"
        exit 1
        ;;
esac

# Build target firmware
echo "Building target firmware: $TARGET_PROJECT..."
cd $TARGET_PROJECT
rm -rf build && mkdir build && cd build
export PICO_SDK_PATH=$ROOTDIR/pico-sdk
cmake ..
make -j$(nproc)
echo "✓ Target firmware built"

# Convert ELF to header
echo "Converting ELF to header..."
python3 ../../host/elf_to_header.py $ELF_NAME ../../host/target_firmware.h
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

