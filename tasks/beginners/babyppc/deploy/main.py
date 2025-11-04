#!/usr/bin/env python3

from math import sqrt
import base64
import subprocess
import random
import os


ROUNDS = 120


def generate_point():
    return random.random(), random.random()


def dist(a, b):
    return sqrt(sum((a - b) ** 2 for a, b in zip(a, b)))


def main():
    try:
        for _ in range(ROUNDS):
            a, b, c = generate_point(), generate_point(), generate_point()
            point = generate_point()
            print("a: ")
            print(a[0], a[1])
            print("b: ")
            print(b[0], b[1])
            print("c: ")
            print(c[0], c[1])
            print("distance to a: ")
            print(dist(point, a))
            print("distance to b: ")
            print(dist(point, b))
            print("distance to c: ")
            print(dist(point, c))

            user_point = list(map(float, input("x y> ").split()))
            assert dist(user_point, point) < 1e-9

        print("you flag:", os.getenv("FLAG"))
    except Exception:
        raise
        print("fail")


if __name__ == "__main__":
    main()
