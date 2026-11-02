# Fafnir — Official Website

Static multi-page website for the Fafnir UCI chess engine, published through
**GitHub Pages** at:

> <https://kagravashkelia-bit.github.io/Fafnir/>

The generated HTML, CSS, JavaScript and metadata are committed. GitHub Pages
serves `web/` directly; there is no build step in CI.

## Pages

| Path | Purpose |
|------|---------|
| `index.html` | Home: product overview, UCI console, reference numbers |
| `features.html` | Search, evaluation, protocol, builds and deliberate limits |
| `benchmarks.html` | Reproducible depth-20 results for five instruction sets |
| `releases.html` | Download matrix, build selection and SHA-256 digests |
| `release-2-0.html` | 2.0 release notes, upgrade notes and asset list |
| `changelog.html` | Filterable change history |
| `roadmap.html` | Shipped, next and purely exploratory work |
| `docs.html` + `docs-*.html` | Ten reference pages: getting started, build, UCI, search, NNUE, testing, architecture, WASM, troubleshooting |
| `about.html`, `community.html`, `contributing.html` | Project pages |
| `license.html`, `privacy.html`, `terms.html` | Legal pages |
| `faq.html`, `contact.html`, `404.html` | Support and fallbacks |

## Assets

| Path | Purpose |
|------|---------|
| `assets/css/site.css` | Design system: tokens, components, docs shell, mockups, responsive, print |
| `assets/js/site.js` | Mobile nav, dropdown, copy buttons, docs search, changelog filters, TOC scrollspy, reading progress |
| `assets/img/` | Mark, favicon, Apple touch icon and the 1200×630 social card |
| `assets/fafnir-logo.png` | Original 779×779 brand source used to derive the smaller marks |
| `sitemap.xml`, `robots.txt`, `manifest.webmanifest` | Generated metadata |

## Editing

Content lives in the generator, not in the HTML files:

```text
scripts/build_site.py        orchestrates the build and metadata
scripts/site_core.py         layout, navigation, footer, docs shell, helpers
scripts/pages_marketing.py   home, features, benchmarks, releases, changelog, roadmap
scripts/pages_docs.py        documentation pages
scripts/pages_project.py     about, legal, FAQ, contact, 404
```

Edit the relevant module, then regenerate and commit both the source and the
output:

```bash
python scripts/build_site.py
```

Every page is written by `emit()` in `scripts/site_core.py`, so header, footer,
metadata, canonical URL and asset paths stay consistent. Use root-level file
names (`docs-uci.html`, not `docs/uci.html`) so relative links keep working
under the GitHub Pages project path `/Fafnir/`.

## Design system

The visual language follows the supplied Cal.com analysis: white canvas,
`#111111` primary actions, light-gray cards, 96px section rhythm, 12px cards,
8px controls and a dark `#101010` footer. Display type is Inter 600 with
negative tracking (Cal Sans is not available); code uses JetBrains Mono.

Product-UI fragments — UCI console, engine output, evaluation bar, build
matrix, architecture tree — are HTML/CSS, not images, so they stay sharp and
readable at every breakpoint. All numeric claims come from the repository and
the measured benchmark run documented on the benchmarks page.

## Deployment

The workflow
[`.github/workflows/deploy-pages.yml`](../.github/workflows/deploy-pages.yml)
uploads `web/` as a Pages artifact and deploys it on every push that touches
`web/**`, the workflow itself, or `README.md`. Manual runs are available
through `workflow_dispatch`.

All asset paths are relative, so the site works both from a domain root and
under `/Fafnir/`.
