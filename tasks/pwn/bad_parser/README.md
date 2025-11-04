# bad parser | pwn

## Информация

> Я вообще не понимаю что тут написано!
> 
> `nc <ip> 17003`
> 

## Деплой

```sh
cd deploy
docker compose up --build -d
```

## Выдать участинкам

Provide zip archive: [public/bad_parser_public.zip](public/bad_parser_public.zip).

## Описание

Linux userspace pwn
TOCTOU -> mmap OOB -> libc leak -> overwirte TLS/printf/etc to make code exec

## Решение

[Exploit](solve/remote_exploit.py)

## Флаг

ctfcup{r34lllly_b4d_f0000rrrmm4t_f1l3}