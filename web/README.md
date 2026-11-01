# Fafnir — Official Website

Static single-page website for the Fafnir UCI chess engine, published
through **GitHub Pages** at:

> <https://kagravashkelia-bit.github.io/Fafnir/>

## Contents

| Path            | Purpose                                   |
|-----------------|-------------------------------------------|
| `index.html`    | The site itself (semantic HTML, ES5-free) |
| `css/styles.css`| Tokens and components (Apple-style system)|
| `js/main.js`    | Mobile nav, scroll-spy, reveal, copy-to-clipboard |
| `assets/`       | Site imagery                              |

## Deployment

The deployment is fully automated: the workflow
[`.github/workflows/deploy-pages.yml`](../.github/workflows/deploy-pages.yml)
uploads the `web/` folder as a Pages artifact and deploys it on every push
that touches `web/**`, the workflow itself, or `README.md`. Manual runs are
available through the `workflow_dispatch` trigger.

No build step is required — the site is plain HTML, CSS and JavaScript with
all asset paths relative, so it works both from the repository root and
under a project sub-path (`/Fafnir/`).

## Editing

The design system is documented at the top of `css/styles.css` as CSS
custom properties (colours, radii, spacing). Keep every interactive
element on the single Action Blue accent (`#0066cc`), keep headlines
negative-tracked, and reserve the product drop-shadow for imagery only.

Content currently marked **"Coming soon"**: the WebAssembly edition.
