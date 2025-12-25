#include "pico/stdlib.h"
#include <stdio.h>

int main(void) {
    // Initialize stdio for USB CDC
    stdio_init_all();
    
    // Wait for USB connection (optional, but helpful)
    // The USB CDC device needs to be enumerated by the host
    sleep_ms(1000);
    
    printf("\n=== USB CDC Loopback ===\n");
    printf("Type characters and they will be echoed back.\n");
    printf("Press Ctrl+C or send 'q' to quit.\n\n");
    
    // Simple loopback: read from stdin, write to stdout
    while (true) {
        int c = getchar_timeout_us(0);  // Non-blocking read with 0 timeout
        
        if (c != PICO_ERROR_TIMEOUT) {
            // Character received, echo it back
            putchar(c);
            
            // Flush output to ensure it's sent immediately
            if (c == '\n' || c == '\r') {
                fflush(stdout);
            }
            
            // Optional: quit on 'q'
            if (c == 'q' || c == 'Q') {
                printf("\nQuitting...\n");
                break;
            }
        }
        
        // Small delay to prevent CPU spinning
        sleep_us(100);
    }
    
    return 0;
}

