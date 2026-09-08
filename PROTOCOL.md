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
