"""Generate the Fafnir website into `web/`.

Usage:
    python scripts/build_site.py

The generated HTML, sitemap, robots file and web manifest are committed, so the
site can be served by GitHub Pages without a build step in CI.
"""

from __future__ import annotations

import json
import sys
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import pages_docs  # noqa: E402
import pages_marketing  # noqa: E402
import pages_project  # noqa: E402
from site_core import SITE, WEB  # noqa: E402

# slug, priority, changefreq — order matters for a stable sitemap.
PAGES = [
    ("index.html", "1.0", "weekly"),
    ("features.html", "0.9", "monthly"),
    ("benchmarks.html", "0.8", "monthly"),
    ("releases.html", "0.9", "weekly"),
    ("release-2-0.html", "0.8", "monthly"),
    ("changelog.html", "0.7", "weekly"),
    ("roadmap.html", "0.6", "monthly"),
    ("docs.html", "0.9", "weekly"),
    ("docs-getting-started.html", "0.8", "monthly"),
    ("docs-build.html", "0.8", "monthly"),
    ("docs-uci.html", "0.8", "monthly"),
    ("docs-search.html", "0.7", "monthly"),
    ("docs-nnue.html", "0.7", "monthly"),
    ("docs-testing.html", "0.7", "monthly"),
    ("docs-architecture.html", "0.7", "monthly"),
    ("docs-wasm.html", "0.6", "monthly"),
    ("docs-troubleshooting.html", "0.6", "monthly"),
    ("about.html", "0.7", "monthly"),
    ("license.html", "0.5", "yearly"),
    ("contributing.html", "0.6", "monthly"),
    ("community.html", "0.6", "monthly"),
    ("faq.html", "0.7", "monthly"),
    ("contact.html", "0.5", "yearly"),
    ("privacy.html", "0.3", "yearly"),
    ("terms.html", "0.3", "yearly"),
]

BUILDERS = [
    ("home", pages_marketing.home),
    ("features", pages_marketing.features),
    ("benchmarks", pages_marketing.benchmarks),
    ("releases", pages_marketing.releases),
    ("release 2.0", pages_marketing.release_2_0),
    ("changelog", pages_marketing.changelog),
    ("roadmap", pages_marketing.roadmap),
    ("docs hub", pages_docs.docs_hub),
    ("getting started", pages_docs.getting_started),
    ("build", pages_docs.build),
    ("uci", pages_docs.uci_reference),
    ("search", pages_docs.search_internals),
    ("nnue", pages_docs.nnue),
    ("testing", pages_docs.testing),
    ("architecture", pages_docs.architecture),
    ("wasm", pages_docs.wasm),
    ("troubleshooting", pages_docs.troubleshooting),
    ("about", pages_project.about),
    ("license", pages_project.license),
    ("contributing", pages_project.contributing),
    ("community", pages_project.community),
    ("faq", pages_project.faq),
    ("contact", pages_project.contact),
    ("privacy", pages_project.privacy),
    ("terms", pages_project.terms),
    ("404", pages_project.not_found),
]


def sitemap(today: str) -> None:
    entries = []
    for slug, priority, freq in PAGES:
        loc = SITE + "/" + ("" if slug == "index.html" else slug)
        entries.append(
            "  <url>\n"
            f"    <loc>{loc}</loc>\n"
            f"    <lastmod>{today}</lastmod>\n"
            f"    <changefreq>{freq}</changefreq>\n"
            f"    <priority>{priority}</priority>\n"
            "  </url>"
        )
    xml = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n'
        + "\n".join(entries)
        + "\n</urlset>\n"
    )
    (WEB / "sitemap.xml").write_text(xml, encoding="utf-8")
    print(f"  {'sitemap.xml':34} {len(xml):>7,} bytes")


def robots() -> None:
    text = (
        "User-agent: *\n"
        "Allow: /\n"
        "\n"
        f"Sitemap: {SITE}/sitemap.xml\n"
    )
    (WEB / "robots.txt").write_text(text, encoding="utf-8")
    print(f"  {'robots.txt':34} {len(text):>7,} bytes")


def manifest() -> None:
    data = {
        "name": "Fafnir — UCI chess engine",
        "short_name": "Fafnir",
        "description": (
            "A UCI chess engine with SFNNv17 NNUE evaluation, a Stockfish-style "
            "search and native Windows/Linux builds."
        ),
        "start_url": "./index.html",
        "scope": "./",
        "display": "standalone",
        "background_color": "#ffffff",
        "theme_color": "#ffffff",
        "icons": [
            {"src": "assets/img/fafnir-mark-192.png", "sizes": "192x192", "type": "image/png"},
            {"src": "assets/img/fafnir-mark-512.png", "sizes": "512x512", "type": "image/png"},
        ],
    }
    text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
    (WEB / "manifest.webmanifest").write_text(text, encoding="utf-8")
    print(f"  {'manifest.webmanifest':34} {len(text):>7,} bytes")


def main() -> None:
    today = date.today().isoformat()
    print(f"Building Fafnir site into {WEB}")
    for label, builder in BUILDERS:
        print(f"[{label}]")
        builder()
    print("[metadata]")
    sitemap(today)
    robots()
    manifest()

    html_files = sorted(WEB.glob("*.html"))
    total = sum(p.stat().st_size for p in html_files)
    print()
    print(f"Generated {len(html_files)} HTML pages ({total:,} bytes) plus metadata.")
    missing = [p.name for p in html_files if p.stat().st_size < 2000]
    if missing:
        print("WARNING: suspiciously small pages:", ", ".join(missing))
        raise SystemExit(1)


if __name__ == "__main__":
    main()
