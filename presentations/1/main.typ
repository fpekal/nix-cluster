#import "../lib/lib.typ": presentation

#set text(lang: "pl")

#show: presentation(
  title_text: text(size: 0.8em, par(
    justify: true,
    [System klastrowy z~automatyczną optymalizacją kompilacji zadań pod architekturę procesora z~wykorzystaniem Nix],
  )),
  author: [Filip Pękala],
  opiekun: [Łukasz Jeleń],
)

#let explanation_title(body) = {
  set text(size: 0.8em)
  place(center, body, dy: -30pt)
  block()
}

#explanation_title[#text(fill: red, [System klastrowy]) z automatyczną optymalizacją kompilacji zadań pod architekturę procesora z wykorzystaniem Nix]

== System klastrowy

- Docker Swarm
- Kubernetes

Połączone ze sobą komputery, wykonujące wspólnie jakieś zadanie

#pagebreak()

#explanation_title[System klastrowy z automatyczną optymalizacją kompilacji #text(fill: red, [zadań]) pod architekturę procesora z wykorzystaniem Nix]

== Zadanie
- Problem, który można podzielić na mniejsze problemy, ...
- ...które można rozwiązywać równolegle

  #align(center + horizon, block(
    height: 52%,
    clip: true,
    radius: 20pt,
    stroke: color.rgb("#c07e6f") + 0.1em,
    image("tree.webp"),
    fill: color.rgb("#ffe5d9").lighten(50%),
  ))

#pagebreak()

#explanation_title[System klastrowy z automatyczną optymalizacją kompilacji zadań pod architekturę procesora z wykorzystaniem #text(fill: red, [Nix])]

== Nix

#grid(columns: (1fr, 1fr))[
  #align(horizon)[
    - Menedżer pakietów
    - Build System
    - Deterministyczny
    - Ściśle określone środowisko
  ]
][
  #align(center + horizon, rect(height: 45%, image("nix_logo.png"), fill: color.rgb("#ffe5d9").lighten(50%)))
]

#pagebreak()

#align(center + horizon, block(
  height: 100%,
  clip: true,
  radius: 20pt,
  stroke: color.rgb("#c07e6f") + 0.1em,
  image("default-nix.png"),
  fill: color.rgb("#ffe5d9").lighten(50%),
))

#pagebreak()

#explanation_title[System klastrowy z automatyczną #text(fill: red, [optymalizacją kompilacji]) zadań pod architekturę procesora z wykorzystaniem Nix]

== Optymalizacja

- Domyślnie brak flagi `-march`
- Kompilacja programu z uwzględnieniem danych wejściowych

#align(center + horizon, block(
  height: 42%,
  clip: true,
  radius: 20pt,
  stroke: color.rgb("#c07e6f") + 0.1em,
  image("embed.png"),
  fill: color.rgb("#ffe5d9").lighten(50%),
))

= Cele pracy

== Cele pracy
- Zdefiniować protokół między serwerem, a klientami
- Zaimplementować framework
- Napisać przykładowy program korzystający z tego frameworka
- Testy (opcjonalnie)

= Studia literaturowe

#text(size: 0.9em)[
  - M. Kowalewski, P. Seeber, "_Sustainable packaging of quantum chemistry software with the Nix package manager_", Int. J. Quantum Chem. 2022, 122(9), e26872. https://doi.org/10.1002/qua.26872
  - R. Goswami, R. S., A. Goswami, S. Goswami and D. Goswami, "_Reproducible High Performance Computing without Redundancy with Nix,_" 2022 Seventh International Conference on Parallel, Distributed and Grid Computing (PDGC), Solan, Himachal Pradesh, India, 2022, pp. 238-242, doi: 10.1109/PDGC56933.2022.10053342.
]

= Problemy
== Problemy
- Wymyślenie jak ma działać cały system

= Pytania

// == Czym jest Nix??? #box(place(dy: -0.9em, dx: -0.1em, image("nix_logo.png", height: 1em)))
// Nie wiem

