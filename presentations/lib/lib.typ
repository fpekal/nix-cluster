#let pwr_logo = image("pwr.png", height: 20%)
#let palettes = (
  pwr: (
    background: color.rgb("#ffe5d9"),
    accent: color.rgb("#c07e6f"),
    text: color.rgb("#b32316"),
    text_secondary: color.rgb("#b32316").lighten(20%),
  ),
)

#let first_page(
  title_text: content,
  author: content,
  opiekun: content,
  logo: content,
  palette: dictionary,
) = {
  page(
    background: {
      place(center + horizon, rect(
        radius: 0pt,
        fill: palette.background,
        stroke: none,
        width: 100%,
        height: 100% + 1pt,
      ))
      place(left, rect(radius: 0pt, fill: palette.accent, stroke: none, width: 5%, height: 100% + 1pt))
    },
    {
      place(left + top, logo)

      place(horizon, dx: 23pt, {
        rect(title(text(fill: palette.text, title_text)), stroke: none, height: 100%)
      })

      text(size: 0.8em, fill: palette.text_secondary, {
        place(bottom + left, dx: 25pt, [
          Autor: #author\
          Opiekun: #opiekun
        ])
        place(bottom + right, datetime.today().display())
      })
    },
  )
}

#let presentation(
  title_text: content,
  author: content,
  opiekun: content,
) = it => {
  let palette = palettes.pwr
  set page(paper: "presentation-16-9", background: rect(
    radius: 0pt,
    fill: palette.background,
    stroke: none,
    width: 100%,
    height: 100% + 1pt,
  ))
  set text(size: 2.5em)
  set text(fill: palette.text_secondary, font: "Liberation Sans")
  show heading: text.with(fill: palette.text)

  show heading.where(depth: 1): it => {
    page(
      align(center + horizon, it),
    )
  }

  set rect(stroke: palette.accent + 0.1em, radius: 20pt)

  first_page(title_text: title_text, author: author, opiekun: opiekun, logo: pwr_logo, palette: palette)

  set page(
    numbering: "1/1",
    number-align: right,
    footer: align(right, text(size: 0.8em, context { counter(page).display("1/1", both: true) })),
  )

  counter(page).update(1)
  it
}


