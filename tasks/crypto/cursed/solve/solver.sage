#!/usr/bin/env sage

n = 8
FLAG_LENGTH = 18 * 8


def powm(M_i, prime, MS_K, K):
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


def logm(E_i, prime, MS_K, K):
    diff = E_i - MS_K.identity_matrix()

    M_i = MS_K.zero()
    tmp = diff

    for j in range(1, prime):
        j_mod = K(j)

        if j_mod.gcd(prime^2) != 1:
            break

        term = tmp * (1 / j_mod)

        if j % 2 == 0:
            M_i -= term
        else:
            M_i += term

        if tmp == MS_K.zero():
            break

        tmp *= diff

    return M_i


def factor(E, N):
    D = E - identity_matrix(Zmod(N), n)

    p_factor = N

    for i in range(n):
        for j in range(n):
            d_ij = int(D[i, j])

            if d_ij != 0 and d_ij != N and d_ij != -N:
                p_factor = gcd(d_ij, N)

                if p_factor > 1 and p_factor < N:
                    return p_factor
                elif p_factor == N:
                    continue

    return None


def from_bits(bit_array):
    bytes_list = []

    for i in range(0, len(bit_array), 8):
        byte_bits = bit_array[i:i+8]
        byte_value = 0

        for bit in byte_bits:
            byte_value = (byte_value << 1) | bit

        bytes_list.append(byte_value)

    return bytes(bytes_list)


def main():
    [E, B, p] = load('output.sobj')
    R = E.base_ring().characteristic()

    f = factor(E, R)
    q = f // p

    N = p^2 * q^2

    E = E.change_ring(Zmod(N))
    B = [B_i.change_ring(Zmod(N)) for B_i in B]

    B_new = []

    for i in range(len(B)):
        if i % 8 != 7:
            B_new.append(B[::-1][i])

    B = B_new[::-1]

    M_recovered_components = []
    E_check_components = []

    for p_i in [p, q]:
        Q_i = p_i^2
        K_i = Zmod(Q_i)
        R_i = MatrixSpace(K_i, n, n)
        E_i_attack = R_i(E)
        M_i_recovered = logm(E_i_attack, p_i, R_i, K_i)
        M_recovered_components.append(M_i_recovered)
        E_i_check = powm(M_i_recovered, p_i, R_i, K_i)
        E_check_components.append(E_i_check)
        assert E_i_check == E_i_attack

    M_recovered_coords = []
    R_SPACE_ATTACK = MatrixSpace(Zmod(N), n, n)

    for i in range(n):
        for j in range(n):
            M_coords = [M_k[i, j].lift() for M_k in M_recovered_components]
            M_element = crt(M_coords, [p^2, q^2])
            M_recovered_coords.append(M_element)

    M_recovered = R_SPACE_ATTACK(M_recovered_coords)

    trace_M = M_recovered.trace().lift()
    trace_B_list = [B_i.trace().lift() for B_i in B]
    N_scalar = len(B)

    T_vec = vector(ZZ, trace_B_list)
    B_scalar_lattice = block_matrix(ZZ, [
        [identity_matrix(N_scalar), matrix(ZZ, T_vec).transpose()],
        [matrix(ZZ, 1, N_scalar), N * identity_matrix(1)] # N используется здесь
    ])
    target_t_scalar = vector(QQ, [0]*N_scalar + [trace_M])

    B_reduced_scalar = B_scalar_lattice.BKZ()

    def babai_closest_vector(B_reduced, target_t):
        try:
            B_inv = B_reduced.inverse()
            x_coord = target_t * B_inv
        except Exception as e:
            return None

        z_coord = vector(ZZ, [round(c) for c in x_coord])
        v_approx = z_coord * B_reduced

        return vector(ZZ, v_approx)

    v_solution_scalar = babai_closest_vector(B_reduced_scalar, target_t_scalar)

    if v_solution_scalar is None:
        print("failed")
        return

    v_diff_scalar = vector(ZZ, target_t_scalar) - v_solution_scalar
    recovered_x_scalar = vector(ZZ, [abs(c) for c in v_diff_scalar[:N_scalar]])

    ptr = 0
    flag_bits = [0] * FLAG_LENGTH

    for i in range(FLAG_LENGTH):
        if i % 8 != 7:
            flag_bits[i] = recovered_x_scalar[::-1][ptr]
            ptr += 1

    flag = from_bits(flag_bits[::-1])

    print(b'ctfcup{' + flag + b'}')


if __name__ == '__main__':
    main()
