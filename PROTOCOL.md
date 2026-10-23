# Protokół komunikacyjny między serwerem a klientami
Cała komunikacja odbywa się przez AMQP z RabbitMQ jako broker.

Buildery i serwer wysyłają swoje wiadomości do tego samego exchange o nazwie "builders".
Buildery na początku generują uuidv4, które od tego momentu będzie ich unikalnie identyfikować.

Buildery wysyłają wszystkie wiadomości z `routing_key` o wartości "builder.\[uuid\]"
Serwer odsyła im wiadomości używając tego samego `routing_key`.

Najpierw builder tworzy swoją kolejkę i podpina ją pod exchange,
Tworzy Consumera i Publishera i podpina je do wcześniej utworzonej kolejki.

Serwer nasłuchuje wszystkich wiadomości z `routing_key` "builder.*"

# Inicjalizacja
Inicjalizacja buildera wymaga wysłania do wiadomości:
```json
{
  "message_uuid": "message_uuid_v4",
  "type": "initialize_builder",
  "content": {
	"name": "human readable name",
	"client_config": {...}
  }
}
```

`message_uuid` identyfikuje wiadomość. Używane do odpowiadania
`name` to niekoniecznie unikalna nazwa używana w logach
`client_config` to customowe informacje DLA SERWERA o kliencie.
Serwer używa tej informacji żeby samemu wygenerować `client_config`
wysyłany razem z taskiem.

Serwer po odczytaniu prośby o inicjalizację buildera odsyła mu wiadomość:
```json
{
  "message_uuid": "message_uuid_v4",
  "type": "initialize_builder_response",
  "content": {
	"response_uuid": "message_uuid_v4",
	"response": "success"
  }
}
```

Dowolna wartość `response` inna niż "success" oznacza nieprzyjęcie połączenia.
W takiej sytuacji serwer nie będzie wysyłać tasków do tego buildera.


# Wysyłanie tasków
Kiedy serwer zdecyduje się poprosić jakiegoś klienta o wykonanie zadania, wysyła mu wiadomość:
```json
{
  "message_uuid": "message_uuid_v4",
  "type": "task_request",
  "content": {
	"flake_url": "https://example.com/"
	"client_config": "{nix: \"expression\"}",
	"task_input": "{another_nix: \"expression\"}"
  }
}
```

# Odbieranie tasków
Kiedy klient odbierze task, to wysyła odpowiedź:
```json
{
  "message_uuid"": "message_uuid_v4",
  "type": "task_request_response",
  "content": {
	"request_uuid": "server_message_uuid",
	"response": "success"
  }
}
```

Klient zaczyna wykonywać task

# Wysyłanie logów
Klient może w dowolnym momencie po połączeniu wysłać tekst do serwera, żeby ten wpisał go do logów:
```json
{
  "message_uuid": "message_uuid_v4",
  "type": "log",
  "content": {
	"message": "Tekst!"
  }
}
```

Serwer zapisze go w logach, dopisując przy tym nazwę klienta, od którego dostał tę wiadomość.

# Wynik wykonywania taska
Kiedy klient skończy wykonywać taska (nieważne, czy zakończono go pozytywnie, czy nie), wysyła do serwera wynik:
```json
{
  "message_uuid": "message_uuid_v4",
  "type": "task_completed",
  "content": {
	"request_uuid": "server_message_uuid",
	"response": "success", // Dowolny inny tekst oznacza błąd
	"result": "result" // `cat ./result` po ukończeniu taska TODO: wynik powinien być wysyłany strumieniem?
  }
}
```

# Ping pong
Serwer i klienci mogą w każdej chwili poprosić drugą stronę o odpowiedzenie na ping.
```json
{
  "message_uuid": "message_uuid_v4",
  "type": "ping"
}
```

```json
{
  "message_uuid": "message_uuid_v4",
  "type": "pong",
  "content": {
	"ping_uuid": "ping_uuid"
	"timestamp": 1792750950
  }
}
```

