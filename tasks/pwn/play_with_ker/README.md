# Play with Ker | pwn

## Информация

> Никто не хочет играть со мной в WoW. Все говорят, что это для стариков и скучно.
> 
> Что ж, может быть тогда вы будете не против сыграть со мной в более простую игру?
> 
> `nc <ip> 17002`
> 

## Деплой

```sh
cd deploy
docker compose up --build -d
```

## Выдать участинкам

Provide zip archive: [public/play_with_ker_public.zip](public/play_with_ker_public.zip).

## Описание

Linux kernel pwn
OOB in heap -> file spray -> leak KASLR via `cpu_entry_area` -> AW `modrpobe_path`

## Решение

[Exploit](solve/exploit.c)

## Флаг

ctfcup{th4nk5_f0r_th3_pl4y1ng_I_h0000p33_y00u_enj0y_1t!}
