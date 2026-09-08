2 możliwości zrobienia klienta

1. Specjalny program, który będzie nasłuchiwał na jakimś websocket (albo rabbitmq czy coś) czy są nowe taski.
2. Użycie flagi `--builders` komendy `nix build` żeby wymusić budowanie na remote komputerze z zainstalowanym `nix`.

Pierwsza opcja jest mniej elegancka, ale może być bardziej wartościowa dla pracy inżynierkiej.
Trzeba do niej użyć właśnie jakiegoś mechanizmu komunikowania się między serwerem, a klientami
i wymyślić protokół.
Dodatkowo tylko klient będzie musiał umieć połączyć się z serwerem. W przypadku `--builders` serwer
będzie musiał umieć nawiązać połączenie z klientami (to nie będzie jakiś niesamowicie jaki problem,
bo można ustawić VPN'a)

Minusem jest to, że wtedy klient musi samemu umieć pozyskać całą definicję taska, więc ALBO flake taska
będzie musiał być publiczny, albo będzie wysyłany jako część protokołu.
