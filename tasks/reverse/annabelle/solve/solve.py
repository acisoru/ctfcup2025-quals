from typing import List
import re
import angr
import claripy
from Crypto.Cipher import ARC4
from pwn import disasm, context, u64

context.arch = "amd64"
context.log_level = "error"


def solve_stage(stage: bytes, target: bytes) -> bytes:
    proj = angr.Project("/bin/true", auto_load_libs=False)

    state = proj.factory.blank_state(addr=0x1000)

    flag = claripy.BVS(f"f", 32 * 8)
    stage = bytearray(stage)
    code_len = u64(stage[0:8])

    rdi = 0x1000
    state.memory.store(rdi, flag)
    state.memory.store(rdi + 32, stage)

    rsi = 0x100000
    state.memory.store(rsi, b"\x00" * 32)
    state.memory.store(rsi + 32, stage)
    state.regs.rip = rdi + 48 + 0x119 - 16
    state.regs.rdi = rdi
    state.regs.rsi = rsi
    state.regs.rsp = 0x300000
    state.regs.rsp = state.regs.rsp + 264

    # state.memory.store(0x1000, code)

    simgr = proj.factory.simulation_manager(state)
    simgr.explore(find=rdi + 48 + code_len - 2)

    final_state = simgr.found[0]
    actual_output = final_state.memory.load(rsi, 32)
    solution = final_state.solver.eval(
        flag, extra_constraints=[actual_output == target]
    )
    solution_bytes = solution.to_bytes(32, "big")
    return solution_bytes


def hash_stage(data: bytes, start: int, mul: int) -> bytes:
    res = start
    for el in data:
        res ^= el
        res *= mul
        res %= 2**64
    return res.to_bytes(8, "little")


def decrypt_stages(data: bytes) -> List[bytes]:
    stages = [data]
    while True:
        if u64(stages[-1][0:8]) == 0:
            break
        code_len = u64(stages[-1][0:8])
        data_len = u64(stages[-1][8:16])
        d = disasm(stages[-1][16 : 16 + 50], arch="amd64")
        start = int(re.search(r"movabs\s*rax,\s*(.*)", d).group(1), 0)
        mul = int(re.search(r"imul\s*rax,\s*rax, (.*)", d).group(1), 0)
        hash = hash_stage(stages[-1][16 : 16 + code_len], start, mul)
        cipher = ARC4.new(hash)
        stages.append(
            cipher.decrypt(stages[-1][16 + code_len : 16 + code_len + data_len])
        )

    return stages


def get_final_target(stage: bytes) -> bytes:
    d = disasm(stage[16:])
    return bytes(int(i, 0) for i in re.findall(r"mov\s*cl,\s*(.*)", d))


def main():
    with open("gen.bin", "rb") as f:
        data = f.read()

    stages = decrypt_stages(data)
    final_stage = stages[-1]
    stages = stages[:-1]
    target = get_final_target(final_stage)
    for stage in reversed(stages):
        print(target)
        target = solve_stage(stage, target)
    print(target)


if __name__ == "__main__":
    main()
