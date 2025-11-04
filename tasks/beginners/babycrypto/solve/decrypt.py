#!/usr/bin/env python3
import random
import hashlib
from typing import List

class CustomCrypto:
    def __init__(self, master_key: str):
        self.master_key = master_key.encode()
        self.key_hash = hashlib.sha256(self.master_key).digest()
        
    def _generate_substitution_table(self, seed: int) -> List[int]:
        random.seed(seed)
        table = list(range(256))
        random.shuffle(table)
        return table
    
    def _bit_rotate(self, value: int, amount: int) -> int:
        return ((value << amount) | (value >> (8 - amount))) & 0xFF
    
    def _custom_xor(self, data: bytes, key: bytes) -> bytes:
        result = bytearray()
        key_len = len(key)
        
        for i, byte in enumerate(data):
            key_byte = key[i % key_len]
            rotated_key = self._bit_rotate(key_byte, (i % 7) + 1)
            result.append(byte ^ rotated_key)
        
        return bytes(result)
    
    def _layer2_xor(self, data: bytes, nonce: int) -> bytes:
        nonce_bytes = nonce.to_bytes(8, 'little')
        extended_nonce = (nonce_bytes * (len(data) // 8 + 1))[:len(data)]
        return self._custom_xor(data, extended_nonce)
    
    def decrypt(self, ciphertext: str) -> str:
        data = self._decode_custom(ciphertext)
        
        data = self._reverse_permutation(data, self.key_hash)
        
        for time_seed in range(100000):
            test_data = self._layer2_xor(data, time_seed)
            test_data = self._reverse_substitution(test_data, int.from_bytes(self.key_hash[:4], 'little') % 1000000)
            
            try:
                result = test_data.decode('utf-8')
                if all(ord(c) < 128 and c.isprintable() for c in result):
                    return result
            except:
                continue
        
        return "Decryption failed"
    
    def _decode_custom(self, encoded: str) -> bytes:
        custom_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
        char_to_val = {c: i for i, c in enumerate(custom_chars)}
        
        encoded = encoded.rstrip('=')
        result = bytearray()
        
        for i in range(0, len(encoded), 4):
            chunk = encoded[i:i+4]
            if len(chunk) == 4:
                packed = 0
                for j, c in enumerate(chunk):
                    packed |= char_to_val[c] << (18 - j * 6)
                
                result.append((packed >> 16) & 0xFF)
                result.append((packed >> 8) & 0xFF)
                result.append(packed & 0xFF)
            elif len(chunk) == 3:
                packed = 0
                for j, c in enumerate(chunk):
                    packed |= char_to_val[c] << (12 - j * 6)
                
                result.append((packed >> 8) & 0xFF)
                result.append(packed & 0xFF)
            elif len(chunk) == 2:
                packed = 0
                for j, c in enumerate(chunk):
                    packed |= char_to_val[c] << (6 - j * 6)
                
                result.append(packed & 0xFF)
        
        return bytes(result)
    
    def _reverse_permutation(self, data: bytes, key_hash: bytes) -> bytes:
        result = bytearray()
        key_len = len(key_hash)
        
        for i, byte in enumerate(data):
            result.append(byte ^ key_hash[i % key_len])
        
        return bytes(result)
    
    def _reverse_substitution(self, data: bytes, seed: int) -> bytes:
        table = self._generate_substitution_table(seed)
        reverse_table = [0] * 256
        for i, val in enumerate(table):
            reverse_table[val] = i
        return bytes(reverse_table[b] for b in data)


if __name__ == "__main__":
    crypto = CustomCrypto("ctfcup_master_key_0x1337")
    print("decrypt: ", crypto.decrypt("HNzfqswupoO5eyhbKlOe/f6yLnlbjtaY+me+3SkoDoNOzHWDJdD4NOMOFeuyoOmrb4q2XnQWyZguzD+c"))