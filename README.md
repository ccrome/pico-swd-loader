# SWD RAM Loader - Hello World

A complete example of loading and executing firmware on a Raspberry Pi Pico via SWD (Serial Wire Debug) protocol. The target Pico runs firmware entirely from RAM without any flash.

## Project Structure

```
pico-swd-loader/
├── host/              # SWD loader (runs on host Pico)
│   ├── main.cpp       # Entry point
│   ├── swd_load.cpp   # SWD protocol implementation
│   ├── swd_load.hpp   # SWD loader header
│   ├── swd_debug.cpp  # Debug helper functions
│   ├── swd.pio        # PIO bit-banging for SWD
│   ├── pio_helpers.cpp # PIO helper functions
│   ├── elf_to_header.py   # Converts ELF to C header
│   ├── monitor.py     # Monitoring script
│   └── CMakeLists.txt # Build configuration
├── target-blink/      # Target firmware - LED blink example
│   ├── blink_sdk_minimal.c  # Blink firmware using Pico SDK
│   └── CMakeLists.txt # Build configuration
├── target-usb-cdc/    # Target firmware - USB CDC loopback
│   ├── usb_cdc_loopback.c  # USB CDC echo/loopback example
│   └── CMakeLists.txt # Build configuration
├── pico-sdk/          # Raspberry Pi Pico SDK (submodule)
├── reload.sh          # Build script for target projects
└── README.md          # This file
```

## Hardware Setup

### Connections (Host Pico → Target Pico)

- **GPIO2 (SWDIO)** → Target SWDIO (Pin 25 / GPIO19)
- **GPIO3 (SWCLK)** → Target SWCLK (Pin 24 / GPIO18)
- **GND** → Target GND
- **VBUS** (optional) → Target VBUS (to power target from host)

### Requirements

- 2× Raspberry Pi Pico boards
- 3× jumper wires (4 if powering target from host)
- USB cable for host Pico

## Building

### Prerequisites

```bash
# Install ARM toolchain
cd ~
sudo apt install -y gcc-arm-none-eabi cmake git python3 python3-pip python3-pyelftools

# Clone Pico SDK
git clone https://github.com/ccrome/pico-swd-loader.git
cd pico-swd-loader
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
```

### Quick Build (Recommended)

Use the `reload.sh` script to build everything automatically:

```bash
# Build target-blink (LED blink example)
./reload.sh target-blink

# Build target-usb-cdc (USB CDC loopback example)
./reload.sh target-usb-cdc
```

The script will:
1. Build the target firmware
2. Convert the ELF to a C header
3. Build the host loader
4. Optionally copy to host Pico if mounted

### Manual Build

#### Build Target Firmware

```bash
# For target-blink
cd ~/pico-swd-loader/target-blink
mkdir -p build && cd build
export PICO_SDK_PATH=~/pico-swd-loader/pico-sdk
cmake ..
make -j$(nproc)
# Creates: ram_blink.elf

# For target-usb-cdc
cd ~/pico-swd-loader/target-usb-cdc
mkdir -p build && cd build
export PICO_SDK_PATH=~/pico-swd-loader/pico-sdk
cmake ..
make -j$(nproc)
# Creates: ram_usb_cdc.elf
```

#### Build Host Loader

```bash
# Convert target firmware to C header
cd ~/pico-swd-loader/host
python3 elf_to_header.py ../target-blink/build/ram_blink.elf target_firmware.h
# Or for USB CDC:
# python3 elf_to_header.py ../target-usb-cdc/build/ram_usb_cdc.elf target_firmware.h

# Build host
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

This creates `swd_loader.uf2` - the host loader program.

## Running

1. **Flash Host Pico:**
   - Hold BOOTSEL on host Pico, plug into USB
   - Copy `host/build/swd_loader.uf2` to the mounted drive
   - Host Pico will reboot and run the loader

2. **Prepare Target Pico:**
   - Target should have **empty flash** or be in a clean state
   - Connect wires: SWCLK (GPIO3), SWDIO (GPIO2), GND
   - Power target (via VBUS from host or external power)

3. **Load Firmware:**
   - Host automatically loads firmware to target on boot
   - Behavior depends on target project:
     - **target-blink**: Target LED (GPIO25) will start **blinking**!
     - **target-usb-cdc**: Target will appear as USB CDC device; connect to it and type characters to see them echoed back
   - Check host serial output: `screen /dev/ttyACM0 115200`

## How It Works

### SWD Protocol

The host bit-bangs the SWD (Serial Wire Debug) protocol using PIO to communicate with the target's debug interface. This allows:
- Reading/writing target memory
- Halting/starting the CPU
- Setting registers

### RAM-Only Execution Flow

1. **Host connects via SWD** - Initializes SWD protocol, resets target
2. **Host halts target CPU** - Puts target in debug halt state
3. **Host disables XIP** - If using XIP as RAM, disables execute-in-place
4. **Host loads firmware** - Writes firmware sections to target RAM (0x20000000) via SWD
5. **Host verifies firmware** - Reads back loaded data to verify integrity
6. **Host sets PC and SP** - Sets program counter to entry point (0x20000001 with thumb bit) and stack pointer (0x20042000)
7. **Host resumes CPU** - Releases target from halt, firmware starts executing
8. **Firmware runs** - Uses Pico SDK to initialize GPIO and blink LED

### Why This Approach?

- **Direct RAM execution** - Firmware runs entirely from RAM, no flash required
- **SDK support** - Target firmware can use Pico SDK functions (gpio_init, sleep_ms, etc.)
- **Simple development** - Write normal Pico SDK code, no custom vector tables needed
- **SWD control** - Host has full control over target execution via debug interface

## Available Target Projects

### target-blink

Simple LED blink example using the Pico SDK. The target LED will blink continuously after loading.

**Files:**
- `blink_sdk_minimal.c` - Main firmware source

**Output:**
- `ram_blink.elf` - Compiled firmware

### target-usb-cdc

USB CDC (Communication Device Class) loopback example. The target appears as a USB serial device and echoes all received characters back.

**Files:**
- `usb_cdc_loopback.c` - Main firmware source

**Output:**
- `ram_usb_cdc.elf` - Compiled firmware

**Usage:**
After loading, connect to the target Pico's USB port. It will appear as a serial device (e.g., `/dev/ttyACM0` on Linux). Any characters you type will be echoed back.

## Target Firmware Development

### target-blink Example

The target firmware (`blink_sdk_minimal.c`) demonstrates a simple LED blink using the Pico SDK:

```c
#include "hardware/gpio.h"
#include "pico/stdlib.h"

int main(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(50);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(50);
    }
    return 0;
}
```

The firmware uses standard Pico SDK functions:
- `gpio_init()` - Initialize GPIO pin
- `gpio_set_dir()` - Set GPIO direction
- `gpio_put()` - Set GPIO output level
- `sleep_ms()` - Sleep for milliseconds

The SDK handles all the low-level initialization (clocks, vector tables, etc.) automatically.

### target-usb-cdc Example

The USB CDC loopback firmware (`usb_cdc_loopback.c`) demonstrates USB serial communication:

```c
#include "pico/stdlib.h"
#include <stdio.h>

int main(void) {
    stdio_init_all();  // Initialize USB CDC stdio
    sleep_ms(1000);    // Wait for USB enumeration
    
    printf("\n=== USB CDC Loopback ===\n");
    
    while (true) {
        int c = getchar_timeout_us(0);  // Non-blocking read
        if (c != PICO_ERROR_TIMEOUT) {
            putchar(c);  // Echo character back
            if (c == '\n' || c == '\r') {
                fflush(stdout);
            }
        }
        sleep_us(100);
    }
    return 0;
}
```

This firmware:
- Initializes USB CDC stdio
- Reads characters from USB input
- Echoes them back to USB output
- Works as a simple serial loopback device

## Modifying Target Firmware

To create your own RAM firmware:

1. **Copy an existing target project** as a template:
   ```bash
   cp -r target-blink target-myproject
   # Or: cp -r target-usb-cdc target-myproject
   ```

2. **Modify the source code** in your new target directory

3. **Update CMakeLists.txt**:
   - Change `project()` name to match your project
   - Change `add_executable()` name
   - Update `pico_set_binary_type()` to use your executable name
   - Configure stdio (USB/UART) as needed

4. **Add to reload.sh** (optional):
   - Add your target to the case statement
   - Map it to the correct ELF filename

5. **Build using reload.sh**:
   ```bash
   ./reload.sh target-myproject
   ```

   Or build manually:
   ```bash
   cd target-myproject
   mkdir -p build && cd build
   export PICO_SDK_PATH=../../pico-sdk
   cmake ..
   make -j$(nproc)
   python3 ../../host/elf_to_header.py <your-elf-name>.elf ../../host/target_firmware.h
   cd ../../host/build
   make
   ```

## Debugging

### Enable Full Verification

Debug output is always enabled via `mp_printf()` calls throughout the code. To enable full verification of loaded firmware, ensure `FULL_VERIFY` is defined in `host/swd_load.cpp`:

```cpp
// Enable full verification of loaded firmware
#define FULL_VERIFY
```

This verifies every word of loaded firmware by reading it back after writing. Without this, only the first word is verified.

### Additional Debug Tools

The `host/swd_debug.cpp` file contains helper functions for debugging:
- `test_gpio_pins()` - Test SWD GPIO pins
- `print_connection_checklist()` - Print connection requirements
- `test_target_power()` - Check if target is powered

### Common Issues

**"Read ID failed"**: Check SWD connections (SWCLK/SWDIO swapped?)

**"PC is 0x200000cc" (hardfault)**: VTOR not set, or SDK startup issue

**"PC is 0xfffffffe" (exception return)**: Exception occurred during startup

**LED not blinking**: 
- Target flash not erased?
- Wrong GPIO pin?
- Check serial output for errors

## Credits

Based on the PicoVision project's SWD loader:
https://github.com/pimoroni/picovision

## License

MIT License - See individual file headers
