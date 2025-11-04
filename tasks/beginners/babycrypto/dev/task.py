#!/usr/bin/env python3
import random
import time
import hashlib
from typing import List, Tuple

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
    
    def _layer1_substitution(self, data: bytes, seed: int) -> bytes:
        table = self._generate_substitution_table(seed)
        return bytes(table[b] for b in data)
    
    def _layer2_xor(self, data: bytes, nonce: int) -> bytes:
        nonce_bytes = nonce.to_bytes(8, 'little')
        extended_nonce = (nonce_bytes * (len(data) // 8 + 1))[:len(data)]
        return self._custom_xor(data, extended_nonce)
    
    def _layer3_permutation(self, data: bytes, key_hash: bytes) -> bytes:
        result = bytearray()
        key_len = len(key_hash)
        
        for i, byte in enumerate(data):
            result.append(byte ^ key_hash[i % key_len])
        
        return bytes(result)
    
    def _layer4_encoding(self, data: bytes) -> str:
        custom_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
        result = ""
        
        for i in range(0, len(data), 3):
            chunk = data[i:i+3]
            if len(chunk) == 3:
                packed = (chunk[0] << 16) | (chunk[1] << 8) | chunk[2]
                for j in range(4):
                    result += custom_chars[(packed >> (18 - j * 6)) & 0x3F]
            else:
                if len(chunk) == 1:
                    packed = chunk[0] << 4
                    result += custom_chars[(packed >> 6) & 0x3F]
                    result += custom_chars[packed & 0x3F]
                    result += "=="
                elif len(chunk) == 2:
                    packed = (chunk[0] << 10) | (chunk[1] << 2)
                    result += custom_chars[(packed >> 12) & 0x3F]
                    result += custom_chars[(packed >> 6) & 0x3F]
                    result += custom_chars[packed & 0x3F]
                    result += "="
        
        return result
    
    def encrypt(self, plaintext: str) -> str:
        data = plaintext.encode('utf-8')
        
        time_seed = int(time.time()) % 100000
        key_seed = int.from_bytes(self.key_hash[:4], 'little') % 1000000

        data = self._layer1_substitution(data, key_seed)
        data = self._layer2_xor(data, time_seed)        
        data = self._layer3_permutation(data, self.key_hash)        
        return self._layer4_encoding(data)

def task():
    master_key = "ctfcup_master_key_0x1337"
    crypto = CustomCrypto(master_key)
    
    flag = "ctfcup{REDACTED}"
    plaintext = f"Your flag is: {flag}"
    
    encrypted = crypto.encrypt(plaintext)
    return encrypted, master_key


if __name__ == "__main__":
    enc, m_k = task()
    print(f"enc: {enc}\nm_k: {m_k}")