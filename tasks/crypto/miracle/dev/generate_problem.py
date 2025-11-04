import random
import gmpy2

FLAG_NUMBER = 36241960344221299
N = 56


def random_superincreasing_sequence(n, min_step=1, max_step=1337):
    if n <= 0:
        return []

    sequence = []
    current_sum = 0

    for i in range(n):
        lower_bound = current_sum + min_step
        upper_bound = current_sum + max_step

        next_element = random.randint(lower_bound, upper_bound)

        sequence.append(next_element)
        current_sum += next_element

    return sequence


def main():
    sequence = random_superincreasing_sequence(N)
    modulus = int(gmpy2.next_prime(sum(sequence) + random.randint(1, 1337)))
    multiple = random.randint(2, modulus - 1)

    flag_bits = bin(FLAG_NUMBER)[2:].zfill(N)[::-1]
    ciphertext = sum(
        int(bit) * (multiple * seq_elem % modulus)
        for bit, seq_elem in zip(flag_bits, sequence)
    )
    print("FLAG_NUMBER =", FLAG_NUMBER)
    print("N =", N)
    print("MODULUS =", modulus)
    print("COEFFICIENTS =", [multiple * seq_elem % modulus for seq_elem in sequence])
    print("CIPHERTEXT =", ciphertext)


if __name__ == "__main__":
    main()
