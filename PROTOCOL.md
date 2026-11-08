# Protokół komunikacyjny między serwerem a klientami
Cała komunikacja odbywa się przez AMQP z RabbitMQ jako broker.

Serwer wysyła swoje wiadomości do exchange o nazwie "builders".
Buildery wysyłają swoje wiadomości do exchange o nazwie "server".
Buildery na początku generują uuidv4, które od tego momentu będzie ich unikalnie identyfikować.

Serwer wysyła wiadomośći builderom, ustawiając `routing_key` na "builder.\[uuid_buildera\]".

Najpierw builder tworzy swoją kolejkę i podpina ją pod exchange,
Tworzy Cosumera i podpina go do wcześniej utworzonej kolejki.

Serwer nasłuchuje wszystkich wiadomości z exchange "server"

# Inicjalizacja
Inicjalizacja buildera wymaga wysłania do wiadomości:
```json
{
  "message_uuid": "message_uuid_v4",
  "sender": "client_uuid",
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
Można też użyć tej informacji, żeby dynamicznie móc wybierać który task
powinien być wykonany przez którego buildera.

Serwer po odczytaniu prośby o inicjalizację buildera odsyła mu wiadomość:
```json
{
  "message_uuid": "message_uuid_v4",
  "sender": "server",
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
  "sender": "server",
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
  "sender": "client_uuid",
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
  "sender": "client_uuid",
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
  "sender": "client_uuid",
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
  "sender": "client_uuid or server",
  "type": "ping"
}
```

```json
{
  "message_uuid": "message_uuid_v4",
  "sender": "client_uuid or server",
  "type": "pong",
  "content": {
	"ping_uuid": "ping_uuid"
	"timestamp": 1792750950
  }
}
```

# Discovery
Serwer prosi wszystkich builderów o zwrócenie ich aktualnej konfiguracji i stasusu.
Ta wiadomość wysyłana jest z `routing_key` ustawionym na "builder.broadcast".
```json
{
  "message_uuid": "message_uuid_v4",
  "sender": "server",
  "type": "discovery"
}
```

# Discovery response
Odpowiedź na zapytanie z konfiguracją klienta i czy aktualnie rozwiązuje jakiegoś taska.
Status to może być: "idle", "working"
```json
{
  "message_uuid": "message_uuid_v4",
  "sender": "client_uuid",
  "type": "discovery_response",
  "content": {
	"request_uuid": "server_message_uuid",
	"name": "human readable name",
	"client_config": {...},
	"status": "status"
  }
}
```
