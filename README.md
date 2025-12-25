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
├── target-sdk/        # Target firmware (loaded via SWD)
│   ├── blink_sdk_minimal.c  # Blink firmware using Pico SDK
│   └── CMakeLists.txt # Build configuration
├── pico-sdk/          # Raspberry Pi Pico SDK (submodule)
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

### Build Target Firmware

```bash
cd ~/pico-swd-loader/target-sdk
mkdir -p build && cd build
export PICO_SDK_PATH=~/pico-swd-loader/pico-sdk
cmake ..
make -j$(nproc)
```

This creates `ram_blink.elf` - the firmware to be loaded.

### Build Host Loader

```bash
# Convert target firmware to C header
cd ~/pico-swd-loader/host
python3 elf_to_header.py ../target-sdk/build/ram_blink.elf target_firmware.h

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
   - Target LED (GPIO25) will start **blinking**!
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

## Target Firmware Development

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

## Modifying Target Firmware

To create your own RAM firmware:

1. Start with `blink_sdk_minimal.c` as a template
2. Use Pico SDK functions normally (gpio_init, stdio, etc.)
3. Ensure your code is configured for RAM-only execution (CMakeLists.txt uses `pico_set_binary_type(ram_blink no_flash)`)
4. Rebuild target: `cd target-sdk/build && make`
5. Regenerate header: `python3 host/elf_to_header.py target-sdk/build/ram_blink.elf host/target_firmware.h`
6. Rebuild and reflash host: `cd host/build && make`

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
