#!/usr/bin/env python3
import socket
import base64
from arc4 import ARC4
import json

class RC4:
    """RC4 cipher implementation"""

    def __init__(self, key):
        self.key = key.encode() if isinstance(key, str) else key
        self.sbox = self._init_sbox()

    def _init_sbox(self):
        """Initialize S-box"""
        sbox = list(range(256))
        j = 0
        for i in range(256):
            j = (j + sbox[i] + self.key[i % len(self.key)]) % 256
            sbox[i], sbox[j] = sbox[j], sbox[i]
        return sbox

    def encrypt(self, data):
        """Encrypt data using RC4"""
        data_bytes = data.encode() if isinstance(data, str) else data
        sbox = self.sbox.copy()
        result = []
        i = j = 0

        for byte in data_bytes:
            i = (i + 1) % 256
            j = (j + sbox[i]) % 256
            sbox[i], sbox[j] = sbox[j], sbox[i]
            k = sbox[(sbox[i] + sbox[j]) % 256]
            result.append(byte ^ k)

        return bytes(result)

    def decrypt(self, data):
        """Decrypt data using RC4 (same as encrypt for RC4)"""
        return self.encrypt(data)


if __name__ == "__main__":
    _hash = 'd91bf246270157aa9f369093a5e3f29b'
    _filename = '60843cbbdcec4da25bfa97e00b164991'

    packet = {'cmd':'auth', 'hash':_hash}
    packet = json.dumps(packet)

    ctx = RC4('std::vector_key')
    enc_packet = ctx.encrypt(packet)
    enc_packet = base64.b64encode(enc_packet)

    s = socket.socket()

    s.connect(('94.139.253.45', 4444))

    s.send(enc_packet + b'\n')

    data = s.recv(4096)
    print(ctx.decrypt(base64.b64decode(data)))

    packet = {'cmd':'download', 'filename':_filename}
    packet = json.dumps(packet)
    enc_packet = ctx.encrypt(packet)
    enc_packet = base64.b64encode(enc_packet)

    s.send(enc_packet + b'\n')
    s.settimeout(0.5)

    data = b''
    while True:
        try:
            tmp = s.recv(1024 * 1024)
            if tmp != b'':
                data += tmp
            else:
                break
        except:
            break
    
    data = ctx.decrypt(base64.b64decode(data))
    data = base64.b64decode(json.loads(data)['data'])

    fd = open('encrypted_file_from_server.bin', 'wb')
    fd.write(data)
    fd.close()

    s.close()