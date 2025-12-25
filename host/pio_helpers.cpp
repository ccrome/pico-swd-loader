/**
 * PIO Helper Functions for SWD Loader
 * Provides program loading/unloading for PIO state machines
 */

#include "hardware/pio.h"

static const pio_program_t* current_program = nullptr;
static uint current_offset = 0;

/**
 * Load a PIO program exclusively (remove old one if present)
 */
uint16_t pio_change_exclusive_program(PIO pio, const pio_program* prog) {
    // Remove old program if one was loaded
    if (current_program != nullptr) {
        pio_remove_program(pio, current_program, current_offset);
    }
    
    // Add new program
    current_offset = pio_add_program(pio, prog);
    current_program = prog;
    
    return current_offset;
}

/**
 * Remove the current exclusive program
 */
void pio_remove_exclusive_program(PIO pio) {
    if (current_program != nullptr) {
        pio_remove_program(pio, current_program, current_offset);
        current_program = nullptr;
        current_offset = 0;
    }
}

