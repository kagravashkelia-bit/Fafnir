"""Shared layout, tokens and content helpers for the Fafnir website.

Every page is rendered by `emit()` so navigation, metadata, the docs shell and
the footer stay consistent. The generated HTML is committed to `web/`.
"""

from __future__ import annotations

import html
from pathlib import Path
from textwrap import dedent

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "web"

SITE = "https://kagravashkelia-bit.github.io/Fafnir"
REPO = "https://github.com/kagravashkelia-bit/Fafnir"
VERSION = "2.0"
RELEASE_DATE = "1 November 2026"
RELEASE_TAG = "v2.0"
OG_IMAGE = SITE + "/assets/img/og-image.png"

DL = REPO + "/releases/download/v2.0/"
ASSETS = {
    "windows-x64": {
        "file": "Fafnir-x86-64-windows-x64.zip",
        "sha": "cff0ba2274a3ac95b906110015a8518ce3be84ebfa69640dc1b80db398368305",
        "size": "75.4 MiB",
    },
    "windows-sse4.2": {
        "file": "Fafnir-x86-64-sse4.2-windows-x64.zip",
        "sha": "520da5fab090dfc4ec95b3caec75ade789909537c528f69dd1d5ef9530f5292b",
        "size": "75.4 MiB",
    },
    "windows-avx2": {
        "file": "Fafnir-x86-64-avx2-windows-x64.zip",
        "sha": "0fb8928c674141c3c7dd34fe3b8efae9ca090f6d22502234a130d7ff005573f2",
        "size": "75.4 MiB",
    },
    "linux-x64": {
        "file": "Fafnir-x86-64-linux-x64.tar.gz",
        "sha": "ffedc7c167f462b2be4057b09ad495b0afaeb7984f27df51d64d5dbfe2adff60",
        "size": "75.8 MiB",
    },
    "linux-sse4.2": {
        "file": "Fafnir-x86-64-sse4.2-linux-x64.tar.gz",
        "sha": "0aadf8205f4684bab3c8fbf67bf3ebf4b2a4e5e517b9741760eef2f06d8ea54a",
        "size": "75.8 MiB",
    },
    "linux-avx2": {
        "file": "Fafnir-x86-64-avx2-linux-x64.tar.gz",
        "sha": "f5b6f758556cb3ffe450832fa2538602e4f452719e57ff15f89563e74a18cef0",
        "size": "75.8 MiB",
    },
    "sums": {
        "file": "SHA256SUMS.txt",
        "sha": "e31bea187d5d9db8687049d00ee931eaf2906a8d1a11be6b898013b481b54252",
        "size": "609 B",
    },
}

# Fixed-depth benchmark medians measured on the reference machine.
BENCH = {
    "x86-64": {"nps": 219097, "ms": 2536, "label": "SSE2 baseline"},
    "x86-64-ssse3": {"nps": 302072, "ms": 1832, "label": "SSSE3"},
    "x86-64-sse4.2": {"nps": 312614, "ms": 1772, "label": "SSE4.2 + POPCNT"},
    "x86-64-avx2": {"nps": 419868, "ms": 1318, "label": "AVX2 + BMI2"},
    "x86-64-avx512": {"nps": 446779, "ms": 1239, "label": "AVX-512 + VNNI"},
}
BENCH_NODES = 553530

NAV_PRIMARY = [
    ("features.html", "Features"),
    ("benchmarks.html", "Benchmarks"),
    ("docs.html", "Docs"),
    ("releases.html", "Releases"),
    ("about.html", "About"),
]

NAV_MORE = [
    ("changelog.html", "Changelog", "Release history"),
    ("roadmap.html", "Roadmap", "What comes next"),
    ("faq.html", "FAQ", "Short answers"),
    ("license.html", "License", "GPLv3 and notices"),
    ("contributing.html", "Contributing", "Build, test, send a PR"),
]

MOBILE_GROUPS = [
    ("Product", [
        ("features.html", "Features"),
        ("benchmarks.html", "Benchmarks"),
        ("releases.html", "Releases"),
        ("changelog.html", "Changelog"),
        ("roadmap.html", "Roadmap"),
    ]),
    ("Documentation", [
        ("docs.html", "Documentation hub"),
        ("docs-getting-started.html", "Getting started"),
        ("docs-build.html", "Build from source"),
        ("docs-uci.html", "UCI reference"),
        ("docs-search.html", "Search internals"),
        ("docs-nnue.html", "NNUE evaluation"),
        ("docs-wasm.html", "WebAssembly"),
        ("docs-testing.html", "Testing"),
        ("docs-architecture.html", "Architecture"),
        ("docs-troubleshooting.html", "Troubleshooting"),
    ]),
    ("Project", [
        ("about.html", "About"),
        ("community.html", "Community"),
        ("contributing.html", "Contributing"),
        ("license.html", "License"),
        ("faq.html", "FAQ"),
        ("contact.html", "Contact"),
    ]),
]

DOCS_NAV = [
    ("Start here", [
        ("docs.html", "Documentation"),
        ("docs-getting-started.html", "Getting started"),
        ("docs-build.html", "Build from source"),
    ]),
    ("Engine", [
        ("docs-uci.html", "UCI reference"),
        ("docs-search.html", "Search internals"),
        ("docs-nnue.html", "NNUE evaluation"),
        ("docs-testing.html", "Testing"),
    ]),
    ("Platforms", [
        ("docs-architecture.html", "Architecture"),
        ("docs-wasm.html", "WebAssembly"),
        ("docs-troubleshooting.html", "Troubleshooting"),
    ]),
]

DOCS_ORDER = [
    ("docs.html", "Documentation"),
    ("docs-getting-started.html", "Getting started"),
    ("docs-build.html", "Build from source"),
    ("docs-uci.html", "UCI reference"),
    ("docs-search.html", "Search internals"),
    ("docs-nnue.html", "NNUE evaluation"),
    ("docs-testing.html", "Testing"),
    ("docs-architecture.html", "Architecture"),
    ("docs-wasm.html", "WebAssembly"),
    ("docs-troubleshooting.html", "Troubleshooting"),
]

FOOTER_COLS = [
    ("Product", [
        ("features.html", "Features"),
        ("benchmarks.html", "Benchmarks"),
        ("releases.html", "Releases"),
        ("changelog.html", "Changelog"),
        ("roadmap.html", "Roadmap"),
    ]),
    ("Documentation", [
        ("docs.html", "Overview"),
        ("docs-getting-started.html", "Getting started"),
        ("docs-uci.html", "UCI reference"),
        ("docs-nnue.html", "NNUE"),
        ("docs-build.html", "Build"),
        ("docs-troubleshooting.html", "Troubleshooting"),
    ]),
    ("Project", [
        ("about.html", "About"),
        ("community.html", "Community"),
        ("contributing.html", "Contributing"),
        ("license.html", "License"),
        ("faq.html", "FAQ"),
        ("contact.html", "Contact"),
    ]),
]

ICONS = {
    "github": '<svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true"><path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27s1.36.09 2 .27c1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0 0 16 8c0-4.42-3.58-8-8-8Z"/></svg>',
    "download": '<svg width="15" height="15" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.4" aria-hidden="true"><path d="M8 1.5v8.2M4.8 6.8 8 10l3.2-3.2M2 12.2v1.3c0 .6.4 1 1 1h10c.6 0 1-.4 1-1v-1.3"/></svg>',
    "arrow": '<svg width="14" height="14" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><path d="M2.5 8h11M9 3.5 13.5 8 9 12.5"/></svg>',
    "check": '<svg width="14" height="14" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="m3 8.5 3.2 3.2L13 4.8"/></svg>',
}


def esc(value: str) -> str:
    return html.escape(str(value), quote=True)


def e(value: str) -> str:
    """Escape for text nodes where quotes do not matter."""
    return html.escape(str(value), quote=False)


def badge(text: str, kind: str = "") -> str:
    cls = "badge" + (f" badge--{kind}" if kind else "")
    return f'<span class="{cls}">{text}</span>'


def btn(label: str, href: str, kind: str = "primary", attrs: str = "", icon: str = "") -> str:
    cls = "btn btn--" + kind
    target = ' target="_blank" rel="noopener"' if href.startswith("http") else ""
    icon_html = ICONS.get(icon, "") if icon else ""
    return f'<a class="{cls}" href="{esc(href)}"{target}{(" " + attrs) if attrs else ""}>{icon_html}{label}</a>'


def stat(value: str, label: str, dark: bool = False) -> str:
    cls = "stat stat--dark" if dark else "stat"
    return f'<div class="{cls}"><span class="stat__value">{value}</span><span class="stat__label">{label}</span></div>'


def card(title: str, text: str, icon: str = "", href: str = "", cls: str = "card card--soft",
         extra: str = "", link_label: str = "Read more") -> str:
    icon_html = f'<div class="card__icon">{icon}</div>' if icon else ""
    link_html = ""
    if href:
        link_html = f'<div class="card__meta"><a class="link-arrow" href="{esc(href)}">{link_label}</a></div>'
    title_tag = f'<h3 class="card__title">{title}</h3>'
    return (
        f'<div class="{cls}">{icon_html}{title_tag}<p class="card__text">{text}</p>'
        f'{extra}{link_html}</div>'
    )


def icon_card(glyph: str, title: str, text: str) -> str:
    return (
        f'<div class="icon-card"><span class="icon-card__glyph">{glyph}</span>'
        f'<h4>{title}</h4><p>{text}</p></div>'
    )


def code_block(code: str, lang: str = "bash", title: str = "", light: bool = False,
               copy: bool = True) -> str:
    cls = "code code--light" if light else "code"
    head = ""
    if title or lang or copy:
        copy_btn = (
            '<button class="code-copy" type="button" data-copy aria-label="Copy code">Copy</button>'
            if copy else ""
        )
        head = (
            f'<div class="code__head"><span class="code__title">{esc(title or lang)}</span>'
            f'<span class="code__lang">{esc(lang)}</span>{copy_btn}</div>'
        )
    return (
        f'<div class="{cls}">{head}<div class="code__body"><pre><code>'
        f'{html.escape(dedent(code).strip())}</code></pre></div></div>'
    )


def table(headers: list[str], rows: list[list[str]], cls: str = "", wrap: bool = True) -> str:
    thead = "".join(f"<th>{h}</th>" for h in headers)
    body = "".join("<tr>" + "".join(f"<td>{c}</td>" for c in row) + "</tr>" for row in rows)
    tbl = f'<table class="table {cls}"><thead><tr>{thead}</tr></thead><tbody>{body}</tbody></table>'
    return f'<div class="table-wrap">{tbl}</div>' if wrap else tbl


def callout(text: str, kind: str = "info", title: str = "Note", icon: str = "i") -> str:
    return (
        f'<div class="callout callout--{kind}"><span class="callout__icon">{icon}</span>'
        f'<div><span class="callout__title">{title}</span>{text}</div></div>'
    )


def console(lines: list[tuple[str, str]], title: str = "fafnir — uci", footer: str = "") -> str:
    rows = []
    for kind, text in lines:
        cls = {
            "cmd": "console__cmd",
            "ok": "console__ok",
            "info": "console__info",
            "best": "console__best",
            "dim": "console__dim",
            "prompt": "console__prompt",
        }.get(kind, "")
        rows.append(f'<div class="console__line"><span class="{cls}">{esc(text)}</span></div>')
    foot = f'<div class="mockup__foot">{footer}</div>' if footer else ""
    return (
        '<div class="mockup">'
        f'<div class="mockup__bar"><span class="mockup__dots"><i></i><i></i><i></i></span>'
        f'<span class="mockup__title">{esc(title)}</span></div>'
        f'<div class="mockup__body"><div class="console"><div class="console__body">{"".join(rows)}</div></div></div>'
        f'{foot}</div>'
    )


def info_line(depth: int, seldepth: int, score: str, nodes: str, nps: str, ms: str, pv: str) -> str:
    return (
        f'<div class="engine-lines"><div><span class="depth">info depth {depth}</span> '
        f'seldepth {seldepth} <span class="score">score {score}</span> '
        f'nodes {nodes} nps {nps} time {ms} '
        f'<span class="pv">pv {pv}</span></div></div>'
    )


def download_card(key: str, label: str) -> str:
    asset = ASSETS[key]
    glyph = "ZIP" if asset["file"].endswith(".zip") else "TGZ"
    return (
        f'<a class="download" href="{DL}{asset["file"]}" download>'
        f'<span class="download__glyph">{glyph}</span>'
        f'<span class="download__body"><span class="download__name">{label}</span>'
        f'<span class="download__meta">{asset["file"]} · {asset["size"]}</span></span>'
        f'<span class="download__arrow">{ICONS["download"]}</span></a>'
    )


def asset_rows(keys: list[str]) -> str:
    rows = []
    for key in keys:
        asset = ASSETS[key]
        rows.append(
            f'<li class="asset"><span class="asset__name">{asset["file"]}</span>'
            f'<span class="asset__size">{asset["size"]}</span>'
            f'<span class="asset__meta">SHA-256</span>'
            f'<span class="asset__hash">{asset["sha"]}</span></li>'
        )
    return '<ul class="asset-list">' + "".join(rows) + "</ul>"


def pager(slug: str) -> str:
    index = next((i for i, (s, _) in enumerate(DOCS_ORDER) if s == slug), None)
    if index is None:
        return ""
    prev_item = DOCS_ORDER[index - 1] if index > 0 else None
    next_item = DOCS_ORDER[index + 1] if index + 1 < len(DOCS_ORDER) else None
    cells = []
    if prev_item:
        cells.append(
            f'<a class="doc-pager__link" href="{prev_item[0]}"><span class="doc-pager__dir">Previous</span>'
            f'<span class="doc-pager__title">← {prev_item[1]}</span></a>'
        )
    else:
        cells.append('<span></span>')
    if next_item:
        cells.append(
            f'<a class="doc-pager__link doc-pager__link--next" href="{next_item[0]}"><span class="doc-pager__dir">Next</span>'
            f'<span class="doc-pager__title">{next_item[1]} →</span></a>'
        )
    return '<nav class="doc-pager" aria-label="Documentation pagination">' + "".join(cells) + "</nav>"


def toc_html(items: list[tuple[str, str, bool]]) -> str:
    if not items:
        return ""
    lis = []
    for anchor, label, sub in items:
        cls = ' class="toc-sub"' if sub else ""
        lis.append(f'<li{cls}><a href="#{esc(anchor)}">{label}</a></li>')
    return (
        '<aside class="docs-toc" aria-label="On this page">'
        '<div class="docs-toc__label">On this page</div>'
        f'<ol>{"".join(lis)}</ol></aside>'
    )


def docs_sidebar(slug: str) -> str:
    groups = []
    for label, links in DOCS_NAV:
        items = []
        for link_slug, title in links:
            current = ' aria-current="page"' if link_slug == slug else ""
            items.append(
                f'<li><a class="docs-nav__link" href="{link_slug}"{current}>{title}</a></li>'
            )
        groups.append(
            f'<div class="docs-nav__group"><span class="docs-nav__label">{label}</span>'
            f'<ul class="docs-nav__group" style="list-style:none;margin:0;padding:0">{"".join(items)}</ul></div>'
        )
    return (
        '<aside class="docs-sidebar">'
        '<div class="docs-sidebar__search"><input id="docsSearch" type="search" '
        'placeholder="Filter pages" autocomplete="off" aria-label="Filter documentation pages"></div>'
        f'<nav class="docs-nav" aria-label="Documentation">{"".join(groups)}'
        '<div class="docs-no-results" id="docsNoResults">No page matches that search.</div></nav>'
        '</aside>'
    )


def nav_html(slug: str) -> str:
    links = []
    for href, label in NAV_PRIMARY:
        current = ""
        if href == slug or (href == "docs.html" and slug.startswith("docs")):
            current = ' aria-current="page"'
        links.append(f'<li><a class="nav__link" href="{href}"{current}>{label}</a></li>')

    more_items = "".join(
        f'<a class="nav-menu__item" href="{href}"><span class="nav-menu__icon">↗</span>'
        f'<span><strong>{label}</strong><span>{desc}</span></span></a>'
        for href, label, desc in NAV_MORE
    )
    links.append(
        '<li><details class="nav-menu"><summary class="nav__link">More</summary>'
        f'<div class="nav-menu__panel">{more_items}</div></details></li>'
    )

    mobile = []
    for label, items in MOBILE_GROUPS:
        anchors = "".join(f'<a href="{href}">{title}</a>' for href, title in items)
        mobile.append(
            f'<div class="mobile-nav__group"><div class="mobile-nav__label">{label}</div>{anchors}</div>'
        )
    mobile.append(
        '<div class="mobile-nav__group">'
        f'{btn("Download Fafnir 2.0", "releases.html#downloads", "primary", icon="download")}'
        f'{btn("View on GitHub", REPO, "secondary", icon="github")}'
        "</div>"
    )

    return (
        '<div class="announcement"><div class="announcement__inner">'
        f'{badge("v2.0")}'
        '<span class="announcement__long">Fafnir 2.0 is out — native AVX2 builds and the SFNNv17 network embedded.</span>'
        '<span class="announcement__short">Fafnir 2.0 is out.</span>'
        '<a href="release-2-0.html">Release notes</a></div></div>'
        '<header class="site-header" id="top">'
        '<div class="container"><nav class="nav" aria-label="Primary">'
        '<a class="brand" href="index.html" aria-label="Fafnir home">'
        '<img class="brand__mark" src="assets/img/fafnir-mark-64.png" alt="" width="32" height="32">'
        '<span class="brand__name">Fafnir</span><span class="brand__version">v2.0</span></a>'
        f'<ul class="nav__links">{"".join(links)}</ul>'
        '<div class="nav__actions">'
        f'<a class="btn btn--secondary btn--sm" href="{REPO}" target="_blank" rel="noopener">{ICONS["github"]}<span>GitHub</span></a>'
        f'{btn("Download", "releases.html#downloads", "primary", attrs="data-download", icon="download")}'
        '<button class="nav-toggle" id="navToggle" type="button" aria-expanded="false" aria-controls="mobileNav" aria-label="Open menu">'
        '<span class="nav-toggle__bars"></span></button>'
        '</div></nav></div></header>'
        f'<div class="mobile-nav" id="mobileNav">{"".join(mobile)}</div>'
    )


def footer_html() -> str:
    columns = []
    for title, links in FOOTER_COLS:
        lis = "".join(f'<li><a href="{href}">{label}</a></li>' for href, label in links)
        columns.append(f'<div class="footer__col"><h4>{title}</h4><ul>{lis}</ul></div>')
    columns.append(
        '<div class="footer__col"><h4>Resources</h4><ul>'
        '<li><a href="faq.html">FAQ</a></li>'
        '<li><a href="contact.html">Contact</a></li>'
        '<li><a href="privacy.html">Privacy</a></li>'
        '<li><a href="terms.html">Terms</a></li>'
        '<li><a href="sitemap.xml">Sitemap</a></li>'
        '</ul></div>'
    )
    return (
        '<footer class="site-footer"><div class="container">'
        '<div class="footer__top"><div class="footer__brand">'
        '<a class="brand" href="index.html"><img class="brand__mark" src="assets/img/fafnir-mark-64.png" alt="" width="32" height="32">'
        '<span class="brand__name">Fafnir</span><span class="brand__version">v2.0</span></a>'
        '<p class="footer__tagline">A UCI chess engine with SFNNv17 NNUE evaluation, a Stockfish-style search, and native builds for Windows and Linux.</p>'
        '<span class="footer__status">Engine v2.0 · GPLv3</span>'
        '</div>' + "".join(columns) + '</div>'
        '<div class="footer__bottom"><p>© 2026 Fafnir · Anthony. Engine source under the GNU GPL v3.</p>'
        '<div class="footer__legal">'
        f'<a href="{REPO}" target="_blank" rel="noopener">GitHub</a>'
        '<a href="license.html">License</a><a href="privacy.html">Privacy</a><a href="terms.html">Terms</a>'
        '<a href="sitemap.xml">Sitemap</a></div></div>'
        '</div></footer>'
    )


def head(slug: str, title: str, description: str, jsonld: str = "", extra: str = "") -> str:
    canonical = SITE + "/" + ("" if slug == "index.html" else slug)
    full_title = title if title.endswith("— Fafnir") or title == "Fafnir" else title + " — Fafnir"
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(full_title)}</title>
<meta name="description" content="{esc(description)}">
<link rel="canonical" href="{canonical}">
<meta name="theme-color" content="#ffffff">
<meta name="color-scheme" content="light">
<meta property="og:type" content="{'article' if slug.startswith(('release-', 'docs-')) else 'website'}">
<meta property="og:site_name" content="Fafnir">
<meta property="og:title" content="{esc(full_title)}">
<meta property="og:description" content="{esc(description)}">
<meta property="og:url" content="{canonical}">
<meta property="og:image" content="{OG_IMAGE}">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="{esc(full_title)}">
<meta name="twitter:description" content="{esc(description)}">
<meta name="twitter:image" content="{OG_IMAGE}">
<link rel="icon" type="image/png" sizes="32x32" href="assets/img/favicon-32.png">
<link rel="apple-touch-icon" href="assets/img/apple-touch-icon.png">
<link rel="manifest" href="manifest.webmanifest">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
<link rel="stylesheet" href="assets/css/site.css">
{jsonld}
{extra}
</head>"""


def emit(slug: str, title: str, description: str, body: str, *, layout: str = "page",
         toc: list[tuple[str, str, bool]] | None = None, progress: bool = False,
         jsonld: str = "", extra_head: str = "", docs_title: str = "",
         docs_lead: str = "", docs_meta: str = "", breadcrumbs: str = "",
         pager_links: str | None = None) -> None:
    """Render one page into web/ with the shared chrome."""
    if layout == "docs":
        header = (
            '<div class="docs-header">'
            f'{breadcrumbs}'
            f'<h1>{docs_title or title}</h1>'
            f'<p class="lead">{docs_lead}</p>'
            f'{docs_meta}'
            '</div>'
        )
        content = (
            '<div class="container container--docs"><div class="docs-shell">'
            + docs_sidebar(slug)
            + '<main class="docs-main"><div class="docs-layout-3"><div class="docs-content">'
            + header + body + '</div>' + toc_html(toc or []) + '</div>'
            + (pager(slug) if pager_links is None else pager_links)
            + '</main></div></div>'
        )
    else:
        content = body

    progress_bar = '<div class="reading-progress" id="readingProgress" aria-hidden="true"></div>' if progress else ""
    doc = (
        head(slug, title, description, jsonld=jsonld, extra=extra_head)
        + "<body>"
        + '<a class="skip-link" href="#main">Skip to content</a>'
        + nav_html(slug)
        + progress_bar
        + f'<main id="main">{content}</main>'
        + footer_html()
        + '<script src="assets/js/site.js" defer></script>'
        + "</body></html>\n"
    )
    target = WEB / slug
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(doc, encoding="utf-8")
    print(f"  {slug:34} {len(doc):>7,} bytes")


def section_head(eyebrow: str, title: str, lead: str = "", center: bool = False,
                 split: bool = False) -> str:
    cls = "section-head"
    if center:
        cls += " section-head--center"
    if split:
        cls += " section-head--split"
    lead_html = f'<p class="lead">{lead}</p>' if lead else ""
    return f'<div class="{cls}"><div><span class="eyebrow">{eyebrow}</span><h2>{title}</h2></div>{lead_html}</div>'


def breadcrumbs(items: list[tuple[str, str]]) -> str:
    lis = []
    for i, (label, href) in enumerate(items):
        if href:
            lis.append(f'<li><a href="{href}">{label}</a></li>')
        else:
            lis.append(f'<li aria-current="page">{label}</li>')
    return f'<ol class="breadcrumbs">{"".join(lis)}</ol>'


def docs_meta(version: str = "Fafnir 2.0", updated: str = "2 November 2026", extra: str = "") -> str:
    return (
        '<div class="docs-meta">'
        f'<span>{badge(version, "outline")}</span>'
        f'<span>Updated {updated}</span>'
        f'{extra}'
        '</div>'
    )
