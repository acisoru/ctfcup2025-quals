from typing import Tuple, Callable
from Crypto.Cipher import ARC4
from pwn import context, asm, u64, p64
import struct
import itertools
import random
import string

context.arch = "amd64"


FLAG = b"ctfcup{c2f85700623151de5b4cfc78}"
STAGES = 100
FLAG_OFFSET = 0
FLAG_LEN = 32
CODE_LEN_OFFSET = 32
DATA_LEN_OFFSET = 40
CODE_OFFSET = 48


def rndstr(n: int = 16):
    return "".join(random.choices(string.ascii_letters + string.digits, k=n))


def random_prefix(n: int = 16):
    return "p" + rndstr(n - 1)


def generate_hash(start: int, mul: int) -> str:
    prefix = random_prefix()

    return f"""
    mov rbx, qword ptr [rdi + {CODE_LEN_OFFSET}]
    lea rcx, qword ptr [rdi + {CODE_OFFSET}]
    mov rdx, 0
    mov rax, {start}
{prefix}_hash_loop:
    cmp rdx, rbx
    jge {prefix}_hash_end
    xor al, byte ptr[rcx + rdx]
    imul rax, {mul}
    inc rdx
    jmp {prefix}_hash_loop
{prefix}_hash_end:
    """


def generate_random_crypt_stage(flag: bytes) -> Tuple[bytes, str]:
    flag = bytearray(flag)

    ops = []
    idxs = list(range(FLAG_LEN)) * 7
    random.shuffle(idxs)
    seen_idxs = set()
    for i in idxs:
        if i in seen_idxs:
            input = "rbx"
        else:
            input = "rax"
        seen_idxs.add(i)
        match random.choice(("mul", "xor", "sub", "add")):
            case "mul":
                key = random.randint(0, 127) * 2 + 1
                flag[i] = (flag[i] * key) % 256
                ops.append(
                    "\n".join(
                        (
                            f"mov cl, byte ptr [{input} + {i}]",
                            f"imul rcx, {key}",
                            f"mov byte ptr [rbx + {i}], cl",
                        )
                    )
                )
            case "xor":
                key = random.randint(0, 255)
                flag[i] ^= key
                ops.append(
                    "\n".join(
                        (
                            f"mov cl, byte ptr [{input} + {i}]",
                            f"xor cl, {key}",
                            f"mov byte ptr [rbx + {i}], cl",
                        )
                    )
                )
            case "sub":
                key = random.randint(0, 255)
                flag[i] = (flag[i] - key) % 256
                ops.append(
                    "\n".join(
                        (
                            f"mov cl, byte ptr [{input} + {i}]",
                            f"sub cl, {key}",
                            f"mov byte ptr [rbx + {i}], cl",
                        )
                    )
                )
            case "add":
                key = random.randint(0, 255)
                flag[i] = (flag[i] + key) % 256
                ops.append(
                    "\n".join(
                        (
                            f"mov cl, byte ptr [{input} + {i}]",
                            f"add cl, {key}",
                            f"mov byte ptr [rbx + {i}], cl",
                        )
                    )
                )

    asm = f"""
    lea rax, qword ptr [rdi + {FLAG_OFFSET}]
    lea rbx, qword ptr [rsi + {FLAG_OFFSET}]
    """ + "\n".join(
        ops
    )
    return flag, asm


def hash_stage(data: bytes, start: int, mul: int) -> bytes:
    res = start
    for el in data:
        res ^= el
        res *= mul
        res %= 2**64
    return res.to_bytes(8, "little")


def generate_stage(prev: bytes, flag_change_asm: str) -> Tuple[bytes]:
    prefix = random_prefix()
    start = random.randint(0, 2**64 - 1)
    mul = random.randint(0, 2**16 - 1) * 2 + 1

    code = (
        generate_hash(start, mul)
        + f"""
    mov qword ptr [rbp - 264 + 256], rax
    xor rcx, rcx
{prefix}_init_sbox:
    mov byte ptr [rbp - 264 + rcx], cl
    inc cl
    jnz {prefix}_init_sbox
    
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    
{prefix}_ksa_loop:
    mov al, byte ptr [rbp - 264 + rcx]
    add bl, al

    mov rdx, rcx
    and rdx, 0x7
    
    mov al, byte ptr [rbp - 264 + 256 + rdx]
    add bl, al
    
    mov dl, bl                  
    and rcx, 0xff
    mov al, byte ptr [rbp - 264 + rcx]       
    and rdx, 0xff
    mov r8b, byte ptr [rbp - 264 + rdx]
    mov byte ptr [rbp - 264 + rcx], r8b      
    mov byte ptr [rbp - 264 + rdx], al       
    
    inc cl                       
    jnz {prefix}_ksa_loop        

    mov r12, qword ptr [rdi + {DATA_LEN_OFFSET}]
    xor r13, r13
    lea r14, qword ptr [rdi + {CODE_OFFSET}]
    add r14, qword ptr [rdi + {CODE_LEN_OFFSET}]
    lea r15, qword ptr [rsi + {CODE_LEN_OFFSET}]
    xor rbx, rbx
    xor rcx, rcx
{prefix}_encrypt_loop:
    cmp r13, r12
    jz {prefix}_encrypt_end
    inc cl
    and rcx, 0xff
    mov al, byte ptr [rbp - 264 + rcx]
    add bl, al
    and rbx, 0xff
    mov al, byte ptr [rbp - 264 + rbx]
    and rcx, 0xff
    mov dl, byte ptr [rbp - 264 + rcx]
    mov byte ptr [rbp - 264 + rcx], al
    mov byte ptr [rbp - 264 + rbx], dl
    add al, dl
    and rax, 0xff
    mov al, byte ptr [rbp - 264 + rax]
    xor al, byte ptr [r14 + r13]
    mov byte ptr [r15 + r13], al
    inc r13
    jmp {prefix}_encrypt_loop
{prefix}_encrypt_end:
"""
        + flag_change_asm
        + f"""
        mov rax, rsi
        mov rsi, rdi
        mov rdi, rax
        add rax, {CODE_OFFSET}
        jmp rax
        """
    )

    encoded = asm(code)
    rc4_key = hash_stage(encoded, start, mul)
    cipher = ARC4.new(rc4_key)
    return (
        struct.pack("Q", len(encoded))
        + struct.pack("Q", len(prev))
        + encoded
        + cipher.encrypt(prev)
    )


def generate_final_stage(flag) -> str:
    prefix = random_prefix()
    cmp_asm = (
        f"""
    mov rbx, 0
    lea rax, qword ptr [rdi + {FLAG_OFFSET}]
        """
        + "\n".join(
            f"""
        mov cl, {flag[i]}
        xor cl, byte ptr [rax + {i}]
        or bl, cl
        """
            for i in range(FLAG_LEN)
        )
        + f"""
    mov rax, 1
    cmp bl, 0
    jz {prefix}_correct
    mov rax, 0
{prefix}_correct:
    leave
    ret
            """
    )

    return b"\x00" * 16 + asm(cmp_asm)


def main():
    flag_changes = []

    flag = FLAG
    for _ in range(STAGES):
        flag, flag_change_asm = generate_random_crypt_stage(flag)
        flag_changes.append(flag_change_asm)

    stage = generate_final_stage(flag)
    for asm in reversed(flag_changes):
        stage = generate_stage(stage, asm)
    with open("gen.bin", "wb") as f:
        f.write(stage)


if __name__ == "__main__":
    main()
