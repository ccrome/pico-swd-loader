# SWD RAM Loader - Hello World

A complete example of loading and executing firmware on a Raspberry Pi Pico via SWD (Serial Wire Debug) protocol. The target Pico runs firmware entirely from RAM without any flash.

## Project Structure

```
loader-host/
├── host/              # SWD loader (runs on host Pico)
│   ├── main.cpp       # Entry point
│   ├── swd_load.cpp   # SWD protocol implementation
│   ├── swd.pio        # PIO bit-banging for SWD
│   └── elf_to_header.py   # Converts ELF to C header
├── target-sdk/        # Target firmware (loaded via SWD)
│   ├── minimal_led.c  # Bare-metal blink firmware
│   ├── ram_only.ld    # Linker script for RAM execution
│   └── CMakeLists.txt # Build configuration
└── README.md          # This file
```

## Hardware Setup

### Connections (Host Pico → Target Pico)

- **GPIO2 (SWCLK)** → Target SWCLK (Pin 24 / GPIO18)
- **GPIO3 (SWDIO)** → Target SWDIO (Pin 25 / GPIO19)  
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
   - Target should have **empty flash** (see ERASE_TARGET_FLASH.md)
   - Connect wires: SWCLK, SWDIO, GND
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

1. **Host loads firmware** to target RAM (0x20000000) via SWD
2. **Set up watchdog boot** - Write magic values to watchdog scratch registers
3. **Point PC/SP to ROM** - Let ROM bootloader run first
4. **Start CPU** - Release from halt
5. **ROM checks watchdog** - Sees boot magic, initializes hardware
6. **ROM jumps to RAM** - Executes firmware at 0x20000000
7. **Firmware runs** - Sets VTOR, initializes GPIO, blinks LED

### Why This Approach?

- **ROM initialization** - Clocks, PLLs, peripherals set up properly
- **VTOR setup** - Firmware sets vector table immediately on entry
- **Bare-metal** - No SDK startup overhead that causes exceptions
- **Watchdog boot** - Standard RP2040 boot mechanism

## Target Firmware Development

The target firmware (`minimal_led.c`) demonstrates:

### Custom Vector Table
```c
__attribute__((section(".vectors"))) 
const void* vector_table[48] = {
    (void*)0x20042000,      // Initial SP
    _reset_handler,         // Reset handler
    // ... exception handlers
};
```

### Reset Handler
```c
__attribute__((naked, noreturn))
void _reset_handler(void) {
    // Set VTOR to our vector table in RAM
    (*(volatile uint32_t*)0xE000ED08) = 0x20000000;
    
    // Disable interrupts during init
    __asm volatile("cpsid i");
    
    // Jump to main
    __asm volatile("ldr r0, =main\n bx r0\n");
}
```

### GPIO Direct Access
```c
// Enable GPIO25 as output
SIO_GPIO_OE_SET = (1 << 25);

// Blink
while (1) {
    SIO_GPIO_OUT_SET = (1 << 25);  // ON
    busy_wait(250ms);
    SIO_GPIO_OUT_CLR = (1 << 25);  // OFF
    busy_wait(250ms);
}
```

## Modifying Target Firmware

To create your own RAM firmware:

1. Start with `minimal_led.c` as a template
2. Keep the vector table and reset handler structure
3. Set VTOR to 0x20000000 in reset handler
4. Use direct register access (no SDK `runtime_init()`)
5. Rebuild target and regenerate header
6. Rebuild and reflash host

## Debugging

### Enable Debug Output

In `host/swd_load.cpp`, change:
```cpp
#if 0  // Debug disabled
```
to:
```cpp
#if 1  // Debug enabled
```

This halts the target after loading and dumps registers/PC location.

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
