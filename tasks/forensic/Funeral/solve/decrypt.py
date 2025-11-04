#!/usr/bin/env python3
from Crypto.Cipher import AES
from binascii import unhexlify
import sys

k = unhexlify("0102030405060708090A0B0C0D0E0F10000102030405060708090A0B0C0D0E0F")
ctx = AES.new(k, AES.MODE_ECB)
data = open(sys.argv[1], 'rb').read()

dec = ctx.decrypt(data)

fd = open("decrypted.pcap", 'wb')
fd.write(dec)
fd.close()