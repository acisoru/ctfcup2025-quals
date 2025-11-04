from pwn import *
import numpy as np
import sys

HOST = sys.argv[1]
ROUNDS = 120


def main():
    io = remote(HOST, 8118)
    for _ in range(ROUNDS):
        io.recvline()
        a = tuple(map(float, io.recvline().split()))
        io.recvline()
        b = tuple(map(float, io.recvline().split()))
        io.recvline()
        c = tuple(map(float, io.recvline().split()))

        io.recvline()
        da = float(io.recvline())
        io.recvline()
        db = float(io.recvline())
        io.recvline()
        dc = float(io.recvline())

        xa, ya = a
        xb, yb = b
        xc, yc = c

        A = 2 * (xb - xa)
        B = 2 * (yb - ya)
        C = da**2 - db**2 - xa**2 + xb**2 - ya**2 + yb**2

        D = 2 * (xc - xb)
        E = 2 * (yc - yb)
        F = db**2 - dc**2 - xb**2 + xc**2 - yb**2 + yc**2

        A_matrix = np.array([[A, B], [D, E]])
        b_vector = np.array([C, F])

        solution = np.linalg.solve(A_matrix, b_vector)
        x, y = solution[0], solution[1]
        io.sendline(f"{x} {y}".encode())
    print(io.recvline())


if __name__ == "__main__":
    main()
