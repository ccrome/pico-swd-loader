/**
 * Simple SWD Loader - Hello World Example
 * 
 * This program loads firmware into a slave RP2040 via SWD protocol
 * using GPIO pins 2 (SWDIO) and 3 (SWCLK).
 */

#include "pico/stdlib.h"
#include "stdio.h"
#include "swd_load.hpp"
#include "target_firmware.h"

#define LED_PIN 25  // Onboard LED on host Pico

int main() {
    // Initialize stdio for debug output
    stdio_init_all();
    
    printf("\n========================================\n");
    printf("  SWD Loader - Hello World Example\n");
    printf("========================================\n\n");
    
    // Initialize LED on host
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("Host: Initializing...\n");
    printf("Target firmware: %d sections, entry=0x%08x\n", 
           num_sections, entry_point);
    
    // Blink LED to show we're alive
    for(int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    printf("\n--- Loading firmware to slave device ---\n");
    
    // Load the firmware via SWD
    bool success = swd_load_program(
        section_addresses,
        section_data,
        section_data_len,
        num_sections,
        entry_point | 0x1,  // Thumb mode bit
        0x20042000,         // Stack pointer
        true                // Use XIP as RAM
    );
    
    if (success) {
        printf("\n=== SUCCESS! ===\n");
        printf("Firmware loaded and started on slave device.\n");
        printf("The slave device should now be blinking its LED.\n");
        
        // Blink host LED rapidly to celebrate
        for(int i = 0; i < 10; i++) {
            gpio_put(LED_PIN, 1);
            sleep_ms(50);
            gpio_put(LED_PIN, 0);
            sleep_ms(50);
        }
    } else {
        printf("\n=== FAILED ===\n");
        printf("Could not load firmware to slave device.\n");
        printf("Check connections:\n");
        printf("  GPIO2 (SWDIO) -> Slave SWDIO\n");
        printf("  GPIO3 (SWCLK) -> Slave SWCLK\n");
        printf("  GND -> Slave GND\n");
        
        // Blink SOS pattern
        while(true) {
            // S
            for(int i = 0; i < 3; i++) {
                gpio_put(LED_PIN, 1);
                sleep_ms(100);
                gpio_put(LED_PIN, 0);
                sleep_ms(100);
            }
            sleep_ms(300);
            // O
            for(int i = 0; i < 3; i++) {
                gpio_put(LED_PIN, 1);
                sleep_ms(300);
                gpio_put(LED_PIN, 0);
                sleep_ms(300);
            }
            sleep_ms(300);
            // S
            for(int i = 0; i < 3; i++) {
                gpio_put(LED_PIN, 1);
                sleep_ms(100);
                gpio_put(LED_PIN, 0);
                sleep_ms(100);
            }
            sleep_ms(2000);
        }
    }
    
    // Keep host LED on steadily to show we're done
    gpio_put(LED_PIN, 1);
    
    printf("\nHost loader complete. Press reset to reload.\n");
    
    // Infinite loop
    while(true) {
        tight_loop_contents();
    }
    
    return 0;
}

