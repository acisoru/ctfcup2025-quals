#!/usr/bin/env python3

import base64
import subprocess


def main():
    with 
    try:
        code = base64.b64decode(input("input base64 encoded code> "))
        with open("kek.rs", "wb") as f:
            f.write(code)

        with subprocess.Popen(["cargo", "run"]) as p:
            pass
    except Exception as e:
        print(f"encountered exception {e}")


if __name__ == "__main__":
    main()
