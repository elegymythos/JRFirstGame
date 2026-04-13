#!/usr/bin/env python3
import sys
def embed(infile, outfile, array_name, size_name):
    with open(infile, 'rb') as f:
        data = f.read()
    with open(outfile, 'w') as f:
        f.write('#pragma once\n#include <cstddef>\n\n')
        f.write(f'const unsigned char {array_name}[] = {{')
        for i, b in enumerate(data):
            if i % 16 == 0: f.write('\n    ')
            f.write(f'0x{b:02x}, ')
        f.write(f'\n}};\n')
        f.write(f'const size_t {size_name} = {len(data)};\n')
if __name__ == '__main__':
    if len(sys.argv) != 5:
        print('Usage: embed.py <infile> <outfile> <array_name> <size_name>')
        sys.exit(1)
    embed(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
