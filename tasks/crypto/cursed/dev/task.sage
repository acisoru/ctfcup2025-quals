#!/usr/bin/env sage

FLAG = 'ctfcup{******************}'
assert len(FLAG) == 26


def func(M_i, prime, MS_K, K):
    E_i = MS_K.one()

    tmp = M_i
    value = K(1)

    for j in range(1, prime):
        value *= K(j)

        if value.gcd(prime^2) != 1:
            break

        term = tmp * (1 / value)
        E_i += term

        if tmp == MS_K.zero():
            break

        tmp *= M_i

    return E_i


def to_bits(text):
    bit_list = []

    for byte in text.encode():
        for i in range(7, -1, -1):
            bit = (byte >> i) & 1
            bit_list.append(bit)

    return bit_list


def main():
    n = 8

    p = next_prime(getrandbits(384))
    q = next_prime(getrandbits(384))
    r = next_prime(getrandbits(512))
    s = next_prime(getrandbits(512))

    N = p^2 * q^2
    R = N * r * s

    MS_N = MatrixSpace(Zmod(N), n, n)
    MS_R = MatrixSpace(Zmod(R), n, n)

    M_parts = []
    E_parts = []

    for prime in [p, q]:
        K = Zmod(prime^2)
        MS_K = MatrixSpace(K, n, n)

        L = Zmod(prime)
        MS_L = MatrixSpace(L, n, n)
        T = MS_L.random_element()

        M_i = MS_K(T) * K(prime)
        M_parts.append(M_i)

        E_i = func(M_i, prime, MS_K, K)
        E_parts.append(E_i)

    M_coords = []
    E_coords = []

    for i in range(n):
        for j in range(n):
            M_values = [M_k[i, j].lift() for M_k in M_parts]
            E_values = [E_k[i, j].lift() for E_k in E_parts]

            M_element = crt(M_values, [p^2, q^2])
            E_element = crt(E_values, [p^2, q^2])

            M_coords.append(M_element)
            E_coords.append(E_element)

    M = MS_N(M_coords)
    E = MS_N(E_coords)

    flag_bits = to_bits(FLAG[7:-1])
    indices = [i for i in range(len(flag_bits)) if flag_bits[i]]

    B = [MS_N.zero() for _ in range(len(flag_bits))]
    B_sum = MS_N.zero()

    for i in indices[:-1]:
        B[i] = MS_N.random_element()
        B_sum += B[i]

    B[indices[-1]] = M - B_sum

    for i in range(len(flag_bits)):
        if i not in indices:
            B[i] = MS_N.random_element()

    E = MS_R(E)
    B = [MS_R(B_i) for B_i in B]

    hint = p

    save([E, B, hint], 'output.sobj')
    # [E, B, hint] = load('output.sobj')


if __name__ == '__main__':
    main()
