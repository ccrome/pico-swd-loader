#!/usr/bin/env python3
"""
Simple monitor script for SWD Loader
Connects to host's USB serial and displays debug output
"""

import serial
import serial.tools.list_ports
import sys
import time

def find_pico():
    """Find Raspberry Pi Pico USB serial port"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Look for Pico/RP2040 device
        if "2e8a:000a" in f"{port.vid:04x}:{port.pid:04x}":
            return port.device
    return None

def monitor(port=None):
    """Monitor serial output from the loader"""
    if port is None:
        port = find_pico()
        if port is None:
            print("Error: No Pico found. Available ports:")
            for p in serial.tools.list_ports.comports():
                print(f"  {p.device}: {p.description}")
            sys.exit(1)
    
    print(f"Connecting to {port}...")
    
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            print("Connected! Monitoring output (Ctrl+C to exit):\n")
            print("=" * 60)
            
            while True:
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting)
                    try:
                        text = data.decode('utf-8', errors='replace')
                        print(text, end='', flush=True)
                    except Exception as e:
                        print(f"[Decode error: {e}]")
                time.sleep(0.01)
                
    except KeyboardInterrupt:
        print("\n\nMonitoring stopped.")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        monitor(sys.argv[1])
    else:
        monitor()

