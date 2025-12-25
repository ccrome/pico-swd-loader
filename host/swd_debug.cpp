/**
 * SWD Debug Helper
 * Adds detailed protocol-level debugging to help diagnose connection issues
 */

#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/gpio.h"

#define SWDIO_PIN 2
#define SWCLK_PIN 3

// Simple bit-bang to test basic connectivity
void test_gpio_pins() {
    printf("\n=== GPIO Pin Test ===\n");
    
    // Test SWCLK as output
    gpio_init(SWCLK_PIN);
    gpio_set_dir(SWCLK_PIN, GPIO_OUT);
    printf("SWCLK (GPIO%d) set as output\n", SWCLK_PIN);
    
    // Test SWDIO as output
    gpio_init(SWDIO_PIN);
    gpio_set_dir(SWDIO_PIN, GPIO_OUT);
    printf("SWDIO (GPIO%d) set as output\n", SWDIO_PIN);
    
    // Toggle SWCLK
    printf("Toggling SWCLK...\n");
    for(int i = 0; i < 10; i++) {
        gpio_put(SWCLK_PIN, 1);
        sleep_us(10);
        gpio_put(SWCLK_PIN, 0);
        sleep_us(10);
    }
    
    // Toggle SWDIO
    printf("Toggling SWDIO...\n");
    for(int i = 0; i < 10; i++) {
        gpio_put(SWDIO_PIN, 1);
        sleep_us(10);
        gpio_put(SWDIO_PIN, 0);
        sleep_us(10);
    }
    
    // Test SWDIO as input (check for pullup/pulldown)
    gpio_set_dir(SWDIO_PIN, GPIO_IN);
    gpio_pull_up(SWDIO_PIN);
    sleep_ms(1);
    bool pulled_high = gpio_get(SWDIO_PIN);
    
    gpio_pull_down(SWDIO_PIN);
    sleep_ms(1);
    bool pulled_low = !gpio_get(SWDIO_PIN);
    
    gpio_disable_pulls(SWDIO_PIN);
    
    printf("SWDIO pull test: high=%d, low=%d (should both be 1)\n", 
           pulled_high, pulled_low);
    
    if(pulled_high && pulled_low) {
        printf("✓ GPIO pins appear functional\n");
    } else {
        printf("✗ GPIO pin issue detected!\n");
    }
}

void print_connection_checklist() {
    printf("\n=== Connection Checklist ===\n");
    printf("1. Target must be POWERED (via USB or external)\n");
    printf("2. Host GPIO2 → Target SWDIO pin\n");
    printf("3. Host GPIO3 → Target SWCLK pin\n");
    printf("4. Host GND → Target GND\n");
    printf("5. Target must NOT be in bootsel mode\n");
    printf("6. Target should be running (not held in reset)\n");
    printf("\n");
    printf("RP2040 SWD Pins:\n");
    printf("  - On debug header (if available)\n");
    printf("  - OR GPIO0/GPIO1 can be used (requires boot2 mod)\n");
    printf("  - Check your board's documentation\n");
    printf("\n");
}

void print_saleae_tips() {
    printf("\n=== Saleae Logic Analyzer Tips ===\n");
    printf("\nWhat to capture:\n");
    printf("  Channel 0: SWCLK (GPIO3)\n");
    printf("  Channel 1: SWDIO (GPIO2)\n");
    printf("\n");
    printf("Sample rate: 10+ MHz minimum\n");
    printf("\n");
    printf("What to look for:\n");
    printf("  1. SWCLK should be toggling (clock signal)\n");
    printf("  2. SWDIO should show data pattern\n");
    printf("  3. Look for the magic sequence: 0xE79E (line reset)\n");
    printf("  4. After line reset: 0x6209F392 0x86852D95...\n");
    printf("  5. Check for ACK bits (should be 0b001)\n");
    printf("\n");
    printf("Common problems:\n");
    printf("  - All zeros: Target not connected/powered\n");
    printf("  - All ones: SWDIO stuck high (check pullup)\n");
    printf("  - No clock: PIO not running\n");
    printf("  - Wrong ACK: Target in wrong state\n");
    printf("\n");
}

void test_target_power() {
    printf("\n=== Target Power Test ===\n");
    printf("Checking if we can detect any signal on SWDIO...\n");
    
    gpio_init(SWDIO_PIN);
    gpio_set_dir(SWDIO_PIN, GPIO_IN);
    gpio_pull_up(SWDIO_PIN);
    
    int readings[10];
    for(int i = 0; i < 10; i++) {
        readings[i] = gpio_get(SWDIO_PIN);
        sleep_ms(10);
    }
    
    bool all_same = true;
    for(int i = 1; i < 10; i++) {
        if(readings[i] != readings[0]) {
            all_same = false;
            break;
        }
    }
    
    if(all_same) {
        printf("⚠ SWDIO is static (%s) - possible connection issue\n", 
               readings[0] ? "HIGH" : "LOW");
    } else {
        printf("✓ SWDIO shows activity\n");
    }
}

