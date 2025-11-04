#!/usr/bin/env python3
import ctypes

def decrypt(key, input_data):
    if len(key) != 16:
        raise ValueError("Key must be exactly 16 bytes")
    
    data_len = len(input_data)
    output = (ctypes.c_ubyte * data_len)()
    
    for pos in range(data_len):
        byte = ctypes.c_uint32(input_data[pos])
        
        # Step 1: XOR with multiplication result (32-bit wrap)
        temp = (pos * 0x5A827999) & 0xFFFFFFFF
        byte.value ^= (temp >> 8) & 0xFF
        
        # Step 2: XOR with key byte and position
        byte.value ^= key[(pos + 7) % 16] ^ (pos & 0xFF)
        
        # Step 3: XOR with constant and rotate bits
        byte.value ^= 0xAA
        byte.value = ((byte.value >> 3) | (byte.value << 5)) & 0xFF
        
        # Step 4: Bit rotation based on key and position
        rot_amount = (pos + key[pos % 16]) & 7
        byte.value = ((byte.value >> rot_amount) | (byte.value << (8 - rot_amount))) & 0xFF
        
        # Step 5: Final XOR with key
        byte.value ^= key[pos % 16]
        
        output[pos] = byte.value
    
    return bytes(output)

data = open('encrypted_file_from_server.bin', 'rb').read()
key = [0xae, 0x55, 0xb8, 0x99, 0x4d, 0x13, 0x0, 0x44, 0x8d, 0xb, 0x18, 0xb7, 0x1, 0x12, 0x8f, 0xb]

out = decrypt(key, data)

fd = open("decrypted_file.png", 'wb')
fd.write(out)
fd.close()