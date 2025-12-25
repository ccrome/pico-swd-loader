#!/usr/bin/env python3
"""
Convert ELF firmware to C header file with embedded binary data.
Similar to the picovision pico-stick.h format.
"""

import sys
import struct
from elftools.elf.elffile import ELFFile

def elf_to_c_header(elf_path, output_path):
    """Convert ELF file sections to C header with data arrays"""
    
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)
        
        # Collect loadable segments (RAM sections)
        sections = []
        for segment in elf.iter_segments():
            if segment['p_type'] == 'PT_LOAD':
                data = segment.data()
                if len(data) > 0:  # Only include non-empty segments
                    sections.append({
                        'addr': segment['p_paddr'],
                        'data': data
                    })
        
        if not sections:
            print("Error: No loadable segments found!")
            sys.exit(1)
        
        # Get entry point
        entry_point = elf.header['e_entry']
        print(f"Entry point: 0x{entry_point:08x}")
        print(f"Found {len(sections)} loadable sections")
        
        # Write C header
        with open(output_path, 'w') as out:
            out.write("// Auto-generated from ELF firmware\n")
            out.write("// Target firmware sections\n\n")
            out.write("#pragma once\n\n")
            out.write("#include \"pico/types.h\"\n\n")
            
            # Write each section's data
            for i, section in enumerate(sections):
                addr = section['addr']
                data = section['data']
                
                print(f"Section {i}: addr=0x{addr:08x}, size={len(data)} bytes")
                
                out.write(f"// Section {i}: {len(data)} bytes at 0x{addr:08x}\n")
                out.write(f"const uint elf_data{i}_addr = 0x{addr:08x};\n")
                out.write(f"const uint elf_data{i}[] __attribute__((aligned(4))) = {{\n")
                
                # Write data as uint32_t array
                for j in range(0, len(data), 4):
                    # Pad last word if needed
                    chunk = data[j:j+4]
                    while len(chunk) < 4:
                        chunk += b'\x00'
                    
                    word = struct.unpack('<I', chunk)[0]
                    if j % 32 == 0:
                        out.write("    ")
                    out.write(f"0x{word:08x},")
                    if (j + 4) % 32 == 0:
                        out.write("\n")
                    else:
                        out.write(" ")
                
                out.write("\n};\n\n")
            
            # Write arrays of pointers/sizes
            out.write("const uint section_addresses[] = {\n")
            for i in range(len(sections)):
                out.write(f"    elf_data{i}_addr,\n")
            out.write("};\n\n")
            
            out.write("const uint* section_data[] = {\n")
            for i in range(len(sections)):
                out.write(f"    elf_data{i},\n")
            out.write("};\n\n")
            
            out.write("const uint section_data_len[] = {\n")
            for i in range(len(sections)):
                out.write(f"    sizeof(elf_data{i}),\n")
            out.write("};\n\n")
            
            out.write(f"const uint num_sections = {len(sections)};\n")
            out.write(f"const uint entry_point = 0x{entry_point:08x};\n")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.elf> <output.h>")
        sys.exit(1)
    
    elf_to_c_header(sys.argv[1], sys.argv[2])
    print(f"Generated {sys.argv[2]}")

