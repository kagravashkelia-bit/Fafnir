"""Marketing pages: home, features, benchmarks, releases, changelog, roadmap."""

from site_core import (
    ASSETS,
    BENCH,
    BENCH_NODES,
    DL,
    ICONS,
    RELEASE_DATE,
    REPO,
    SITE,
    asset_rows,
    badge,
    breadcrumbs,
    btn,
    callout,
    card,
    code_block,
    console,
    docs_meta,
    download_card,
    e,
    emit,
    esc,
    icon_card,
    info_line,
    section_head,
    stat,
    table,
)


def home() -> None:
    console_lines = [
        ("cmd", "$ ./Fafnir"),
        ("cmd", "uci"),
        ("info", "id name Fafnir 2"),
        ("info", "id author Anthony"),
        ("info", "option name Hash type spin default 16 min 1 max 1024"),
        ("info", "option name Threads type spin default 1 min 1 max 128"),
        ("info", "option name MultiPV type spin default 1 min 1 max 256"),
        ("info", "option name Move Overhead type spin default 50 min 0 max 5000"),
        ("info", "option name Aggressivity type spin default 0 min 0 max 100"),
        ("ok", "uciok"),
        ("cmd", "isready"),
        ("ok", "readyok"),
        ("cmd", "position startpos"),
        ("cmd", "go depth 20"),
        ("info", "info depth 20 seldepth 34 score cp 35 nodes 553530 nps 464370 time 1192"),
        ("dim", "pv e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 d2d3 b7b5 a4b3 f8e7 e1g1 e8g8"),
        ("best", "bestmove e2e4 ponder e7e5"),
    ]

    body = (
        '<section class="hero"><div class="container"><div class="hero__grid">'
        '<div class="hero__copy">'
        '<span class="eyebrow">Fafnir 2.0 · SFNNv17 NNUE</span>'
        '<h1 class="display-xl hero__title">A chess engine that shows its work.</h1>'
        '<p class="hero__lead">Fafnir pairs an SFNNv17 neural evaluation with a Stockfish-style '
        'search, a small set of real UCI options, and native builds for Windows and Linux. '
        'Every claim on this site is documented, measured, or listed in the changelog.</p>'
        '<div class="btn-row hero__actions">'
        + btn("Download Fafnir 2.0", "releases.html#downloads", "primary", icon="download")
        + btn("Read the docs", "docs.html", "secondary")
        + "</div>"
        '<ul class="hero__meta"><li>GPLv3</li><li>Windows &amp; Linux x64</li>'
        '<li>No external network file</li><li>WASM in preparation</li></ul>'
        "</div>"
        '<div class="hero__panel">'
        + console(
            console_lines,
            title="fafnir — uci session",
            footer="<span>startpos · depth 20</span><span>553,530 nodes · 464,370 NPS</span>",
        )
        + "</div></div></div></section>"

        '<div class="container"><div class="logo-strip">'
        '<span class="logo-strip__item"><b>SFNNv17</b> evaluation</span>'
        '<span class="logo-strip__item"><b>UCI</b> protocol</span>'
        '<span class="logo-strip__item"><b>Lazy SMP</b> threads</span>'
        '<span class="logo-strip__item"><b>5</b> ISA targets</span>'
        '<span class="logo-strip__item"><b>GPLv3</b> license</span>'
        "</div></div>"

        '<section class="section"><div class="container">'
        + section_head(
            "What is inside",
            "Search, evaluation and protocol — without the fog.",
            "Fafnir is deliberately narrow: one engine, one protocol, one embedded network, and a build system that tells you which instructions it used.",
        )
        + '<div class="grid grid--3">'
        + card(
            "Search with receipts",
            "Transposition-table clusters, six history families, correction history, SEE, cuckoo repetition detection and a staged MovePicker.",
            icon="01",
            href="docs-search.html",
            link_label="Search internals",
        )
        + card(
            "NNUE in the binary",
            "The SFNNv17 network is embedded with incbin. No external .nnue file, no download at startup, no smudge step in git.",
            icon="02",
            href="docs-nnue.html",
            link_label="NNUE details",
        )
        + card(
            "Only real UCI options",
            "Hash, Threads, MultiPV, Move Overhead and Aggressivity are implemented. Anything else is answered honestly.",
            icon="03",
            href="docs-uci.html",
            link_label="UCI reference",
        )
        + card(
            "Native by default",
            "The default binary runs on any x86-64 CPU. AVX2 and AVX-512 builds are one make target away.",
            icon="04",
            href="docs-build.html",
            link_label="Build guide",
        )
        + card(
            "Measured on hardware",
            "A reproducible depth-20 benchmark across five instruction sets, with the machine, flags and protocol written down.",
            icon="05",
            href="benchmarks.html",
            link_label="See the numbers",
        )
        + card(
            "Open source, end to end",
            "GPLv3 engine, documented vendored code, and CI that compiles the same tree on Linux, Windows and Emscripten.",
            icon="06",
            href="license.html",
            link_label="License &amp; notices",
        )
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container">'
        '<div class="feature-split">'
        '<div><span class="eyebrow">Inside the search</span>'
        '<h2 class="feature-split__title">Every node has a reason to exist.</h2>'
        '<p class="feature-split__text">The search follows the Stockfish development playbook with an '
        'in-house implementation: a 3-entry transposition cluster, age-based replacement, LMR with '
        'shallow re-search, singular extensions with multicut, ProbCut, and null-move pruning with '
        'verification.</p>'
        '<ul class="feature-split__list">'
        "<li>Main, low-ply, capture, continuation (2×2), pawn and correction history</li>"
        "<li>MVV-LVA plus SEE ordering in a dedicated MovePicker</li>"
        "<li>Soft and hard time limits with ponder support</li>"
        "<li><code>Aggressivity</code> 0–100 modulates evaluation optimism</li>"
        "</ul>"
        '<div class="btn-row mt-md">' + btn("Open the search reference", "docs-search.html", "secondary") + "</div>"
        "</div>"
        '<div class="feature-split__media">'
        + info_line(18, 30, "cp 38", "412650", "458500", "900", "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 d2d3 b7b5 a4b3 f8e7 e1g1 e8g8")
        + '<div class="card card--pad-sm mt-sm"><div class="row row--between mb-md"><span class="title-sm">Evaluation</span>'
        '<span class="badge badge--outline">+0.38 pawns</span></div>'
        '<div class="evalbar evalbar--white"><span style="width:54%"></span></div>'
        '<div class="eval-legend"><span>Black</span><span>White</span></div>'
        '<p class="card__text mt-md">Score is normalized to pawn units using <code>NORMALIZE_TO_PAWN_VALUE = 356</code>. '
        'Positive values favour White.</p></div>'
        "</div></div></div></section>"

        '<section class="section"><div class="container">'
        '<div class="section-head section-head--split"><div><span class="eyebrow">Reference run</span>'
        "<h2>Numbers you can reproduce.</h2></div>"
        '<p class="lead">Intel Core i7-1165G7, Windows 11, g++ 15.2.0, <code>make all-arches</code>. '
        "Start position, depth 20, Hash 16 MB, one thread. Median of four interleaved runs.</p></div>"
        '<div class="stats">'
        + stat("553,530", "Nodes searched to complete depth 20")
        + stat("446,779", "NPS · AVX-512 + VNNI build")
        + stat("1.24 s", "Time to depth 20 on AVX-512")
        + stat("2.04×", "AVX-512 speed-up over the safe SSE2 build")
        + "</div>"
        '<div class="row row--between mt-lg"><p class="card__text mb-0">NPS measures throughput, not playing strength. '
        'The full table includes every instruction set and the exact flags.</p>'
        + btn("Full benchmark methodology", "benchmarks.html", "secondary") + "</div>"
        "</div></section>"

        '<section class="section section--soft"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Architecture</span><h2>A small surface area, documented.</h2>'
        '<p class="lead mt-md">The engine is split into a UCI front end, a search controller, a move picker, '
        'an NNUE bridge and a thin set of core types. Vendored Stockfish code is isolated behind the probe.</p>'
        '<ul class="checklist mt-md">'
        "<li>No JNI bridge: the unbuildable Java integration was removed in 2.0.</li>"
        "<li>WASM API kept behind <code>__EMSCRIPTEN__</code> with five C functions.</li>"
        "<li>Per-architecture object directories prevent flag mixing.</li>"
        "<li>One documented patch to the vendored SFNNv17 snapshot.</li>"
        "</ul>"
        '<div class="btn-row mt-md">' + btn("Repository map", "docs-architecture.html", "secondary") + "</div></div>"
        '<div class="diagram">'
        "<div><b>src/</b></div>"
        "<div>├─ <span class=\"box\">ui/</span>        main.cpp · uci.cpp · wasm_api.cpp</div>"
        "<div>├─ <span class=\"box\">engine/</span>    search.hpp · movepicker · SearchController</div>"
        "<div>├─ <span class=\"box\">bridges/</span>   nnue_probe.cpp</div>"
        "<div>└─ <span class=\"box\">core/</span>      types.hpp</div>"
        "<div>&nbsp;</div>"
        "<div><b>assets/nnue/</b>  nn-252f33942263.nnue  <span class=\"tok-c\"># incbin</span></div>"
        "<div><b>third_party/</b>  chesslib · stockfish/sfdev · incbin</div>"
        "<div><b>tests/</b>        tests.py · records/</div>"
        "<div><b>web/</b>          this site</div>"
        "<div><b>Makefile</b>      native builds · <b>Makefile.wasm</b> · Emscripten</div>"
        "</div></div></div></section>"

        '<section class="section"><div class="container">'
        + section_head(
            "Documentation",
            "Read the part you need.",
            "The docs are written as a reference, not a brochure: protocol tables, build flags, failure modes and the reasoning behind each limit.",
        )
        + '<div class="grid grid--4">'
        + card("Start here", "Download a binary, connect a GUI, run the first UCI session.", icon="→", href="docs-getting-started.html", link_label="Getting started")
        + card("UCI reference", "Commands, <code>go</code> parameters, options, info fields and exact error behaviour.", icon="⌘", href="docs-uci.html", link_label="Protocol")
        + card("NNUE evaluation", "SFNNv17 features, quantization, embedding and the network hash.", icon="◈", href="docs-nnue.html", link_label="Evaluation")
        + card("Build &amp; ship", "Make targets, ISA flags, Windows/MSYS2, Linux and WASM.", icon="⚙", href="docs-build.html", link_label="Build guide")
        + "</div>"
        '<div class="grid grid--4 mt-md">'
        + icon_card("⌕", "Search internals", "TT, histories, MovePicker stages and pruning.")
        + icon_card("✓", "Testing", "100 EPD positions and a strict best-move hit rule.")
        + icon_card("◇", "WebAssembly", "Five C functions, build output and current status.")
        + icon_card("!", "Troubleshooting", "Illegal instructions, silent GUIs and CI failures.")
        + "</div></div></section>"

        '<section class="section section--tight"><div class="container">'
        '<div class="card card--dark"><div class="release-hero">'
        '<div><div class="badge-row mb-md">' + badge("Latest release", "success") + badge("GPLv3", "outline") + "</div>"
        "<h2>Fafnir 2.0 is ready to play.</h2>"
        '<p class="lead mt-md" style="color:var(--on-dark-soft)">Six native archives for Windows and Linux, '
        'three instruction-set levels, and the SFNNv17 network already inside every binary. '
        'SHA-256 checksums are published for every asset.</p>'
        '<ul class="checklist checklist--dark mt-md">'
        "<li>Safe x86-64, SSE4.2 and AVX2 builds</li>"
        "<li>No external network file to install</li>"
        "<li>Engine identity <code>Fafnir 2</code></li>"
        "</ul>"
        '<div class="btn-row mt-md">' + btn("Download 2.0", "releases.html#downloads", "inverse", icon="download")
        + btn("Release notes", "release-2-0.html", "outline-inverse") + "</div></div>"
        '<div><div class="download-grid" style="grid-template-columns:1fr">'
        + download_card("windows-x64", "Windows · x86-64")
        + download_card("windows-avx2", "Windows · AVX2")
        + download_card("linux-x64", "Linux · x86-64")
        + download_card("linux-avx2", "Linux · AVX2")
        + "</div>"
        f'<p class="card__text mt-md mb-0" style="color:var(--on-dark-soft)">SHA256SUMS.txt · '
        f'<a href="{DL}SHA256SUMS.txt" style="color:var(--on-dark)">all checksums</a></p>'
        "</div></div></div></div></section>"

        '<section class="section"><div class="container">'
        + section_head(
            "Open source",
            "Built in the open, with the boring parts included.",
            "The repository contains the search, the build system, the tests and this website. Vendored code is listed with its license and the one local patch.",
        )
        + '<div class="grid grid--3">'
        + card("Source &amp; issues", "Browse every file, open a reproducible bug report, or read the commit that changed a behaviour.", icon="◎", href=REPO, link_label="github.com/kagravashkelia-bit/Fafnir")
        + card("Continuous integration", "Native Windows/Linux builds with a UCI smoke test, plus an Emscripten build on every push to main.", icon="⟳", href=REPO + "/actions", link_label="Workflow runs")
        + card("GPLv3 &amp; provenance", "The complete engine is GPLv3. ChessLib is MIT, incbin is Unlicense, and the SFNNv17 snapshot is GPLv3.", icon="§", href="license.html", link_label="License overview")
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container container--narrow">'
        + section_head("FAQ", "Short answers to fair questions.")
        + '<div class="faq">'
        '<details class="faq__item"><summary>Does Fafnir need an external NNUE file?</summary>'
        '<div class="faq__body"><p>No. The network is embedded with <code>incbin</code> at compile time. '
        'You can move the binary alone and it still evaluates positions. The WASM build disables embedding '
        'with <code>NNUE_EMBEDDING_OFF</code>.</p></div></details>'
        '<details class="faq__item"><summary>Which build should I download?</summary>'
        '<div class="faq__body"><p>Start with the safe <code>x86-64</code> archive if you are unsure. '
        'AVX2 is the right choice for most CPUs from 2013 onward; SSE4.2 covers older machines. '
        'The engine will not start on a CPU that lacks the instructions in the build you chose.</p></div></details>'
        '<details class="faq__item"><summary>Is this Stockfish?</summary>'
        '<div class="faq__body"><p>No. Fafnir is a separate engine with its own search implementation and UCI '
        'front end. It uses the Stockfish development NNUE architecture (SFNNv17) and vendored evaluation code, '
        'which is why the project is GPLv3. The search, move picker, controller and protocol layer are in this repository.</p></div></details>'
        '<details class="faq__item"><summary>Can I use it in a tournament or on a website?</summary>'
        '<div class="faq__body"><p>Yes, subject to the GPLv3 and the rules of the event or platform. '
        'The WASM public edition is still in preparation; the native UCI engine is the supported build today.</p></div></details>'
        '<details class="faq__item"><summary>Why are there only five UCI options?</summary>'
        '<div class="faq__body"><p>Because only five are implemented. Syzygy tablebases and opening books are not '
        'declared until they actually work. Unknown options return <code>info string unknown UCI option</code>.</p></div></details>'
        "</div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band cta-band--center">'
        "<h2>Put it on the board.</h2>"
        '<p class="lead--sm">Download a native build, or read the protocol first and wire it into your own GUI.</p>'
        '<div class="btn-row">' + btn("Download Fafnir 2.0", "releases.html#downloads", "primary", icon="download")
        + btn("Read the UCI reference", "docs-uci.html", "secondary") + "</div>"
        "</div></div></section>"
    )

    jsonld = (
        '<script type="application/ld+json">'
        '{"@context":"https://schema.org","@type":"SoftwareApplication","name":"Fafnir",'
        '"applicationCategory":"GameApplication","operatingSystem":"Windows, Linux",'
        '"softwareVersion":"2.0","license":"https://www.gnu.org/licenses/gpl-3.0.html",'
        f'"downloadUrl":"{REPO}/releases/tag/v2.0","url":"{SITE}/",'
        '"description":"UCI chess engine with SFNNv17 NNUE evaluation and native multi-architecture builds."}'
        "</script>"
    )
    emit(
        "index.html",
        "Fafnir — UCI chess engine with NNUE evaluation",
        "Fafnir is a UCI chess engine with SFNNv17 NNUE evaluation, a Stockfish-style search, real UCI options, and native Windows/Linux builds. Download 2.0 or read the documentation.",
        body,
        jsonld=jsonld,
    )


def features() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Features", "")])
        + '<span class="eyebrow">Product</span>'
        '<h1 class="page-head__title">A search stack you can audit.</h1>'
        '<p class="page-head__lead">No hidden options, no mystery network file, no build that crashes on an older CPU. '
        "Fafnir exposes the parts that matter and documents how they fit together.</p>"
        '<div class="btn-row page-head__meta">' + btn("Read the UCI reference", "docs-uci.html", "primary")
        + btn("Browse the source", REPO, "secondary", icon="github") + "</div>"
        "</div></div>"

        '<section class="section"><div class="container">'
        + section_head(
            "Search",
            "The parts that decide a move.",
            "The search core follows the style of Stockfish development with an in-house implementation. "
            "The list below is the actual stack, not a feature checklist copied from another engine.",
        )
        + '<div class="grid grid--3">'
        + card("Transposition table", "Clusters of three entries, generation counters and age-based replacement. Configurable from 1 to 1024 MB and resizable at runtime.", icon="TT")
        + card("History heuristics", "Main history, low-ply history, capture history, continuation history (2×2), pawn history and four correction-history tables.", icon="H")
        + card("MovePicker", "TT move, good captures (MVV-LVA + SEE), history-ordered quiets, bad captures, bad quiets, evasions, ProbCut and qsearch.", icon="MP")
        + card("Pruning &amp; reductions", "Razoring, futility pruning, null-move pruning with verification, IIR, ProbCut, singular extensions with multicut, and LMR.", icon="↓")
        + card("Draw detection", "SEE with pinned pieces, cuckoo detection of imminent repetition, repetition draws and the fifty-move rule integrated into the search stack.", icon="=")
        + card("Time management", "Soft and hard limits, ponder support, and control by movetime, depth, wtime/btime, increments and movestogo.", icon="⏱")
        + "</div>"
        '<div class="mt-lg">'
        + table(
            ["Stage", "What is generated", "Ordering signal"],
            [
                ["1. TT move", "The hash move for this position", "Transposition table"],
                ["2. Good captures", "Captures that pass SEE", "MVV-LVA, then SEE"],
                ["3. Quiets", "All remaining quiet moves", "History + continuation history"],
                ["4. Bad captures", "Captures that lose material by SEE", "MVV-LVA"],
                ["5. Bad quiets", "Quiets deferred by pruning", "History"],
                ["6. Evasions", "Legal responses to check", "TT, captures, history"],
                ["7. ProbCut / qsearch", "Tactical subset at reduced depth", "SEE and capture gain"],
            ],
            cls="table--kv",
        )
        + "</div>"
        '<p class="card__text mt-md">Maximum search stack depth is <code>MAX_PLY = 246</code>. '
        "The root supports MultiPV from 1 to 256 variations.</p>"
        "</div></section>"

        '<section class="section section--soft"><div class="container"><div class="feature-split">'
        '<div><span class="eyebrow">Evaluation</span><h2 class="feature-split__title">A neural network that travels with the binary.</h2>'
        '<p class="feature-split__text">Fafnir embeds the default SFNNv17 network at link time. There is no '
        'sidecar file to lose, no path to configure, and no LFS pointer pretending to be a network.</p>'
        '<ul class="feature-split__list">'
        "<li>Single-output architecture without PSQT</li>"
        "<li>Feature sets: <code>half_ka_v2_hm</code>, <code>full_threats</code>, <code>pp_3wide</code></li>"
        "<li>Quantized evaluation normalized to pawn units</li>"
        "<li>98,668,604-byte network with a published SHA-256</li>"
        "</ul>"
        '<div class="btn-row mt-md">' + btn("NNUE reference", "docs-nnue.html", "secondary") + "</div></div>"
        '<div class="card"><div class="row row--between mb-md"><span class="title-sm">Network facts</span>'
        + badge("SFNNv17", "violet") + "</div>"
        + table(
            ["Property", "Value"],
            [
                ["Architecture", "SFNNv17, single output, no PSQT"],
                ["Default file", "<code>nn-252f33942263.nnue</code>"],
                ["Size", "98,668,604 bytes"],
                ["SHA-256", "<code>252f33…7ab3</code>"],
                ["Loading", "<code>incbin</code> at compile time"],
                ["External file", "Not required"],
            ],
            cls="table--kv table--compact",
        )
        + '<p class="card__text mt-sm mb-0">Evaluation is displayed in pawn units as in Stockfish 17.x '
        "(<code>NORMALIZE_TO_PAWN_VALUE = 356</code>).</p></div>"
        "</div></div></section>"

        '<section class="section"><div class="container">'
        + section_head("Protocol", "Five options. All of them real.", "The engine declares only what it implements. This is the complete option surface of Fafnir 2.0.")
        + table(
            ["Option", "Type", "Range", "Default", "What it actually does"],
            [
                ["<code>Hash</code>", "spin", "1–1024 MB", "16", "Resizable transposition table, applied immediately"],
                ["<code>Threads</code>", "spin", "1–128", "1", "Lazy SMP helper threads via SearchController"],
                ["<code>MultiPV</code>", "spin", "1–256", "1", "Root search by variation"],
                ["<code>Move Overhead</code>", "spin", "0–5000 ms", "50", "Safety margin subtracted from the clock"],
                ["<code>Aggressivity</code>", "spin", "0–100", "0", "Evaluation optimism applied during search"],
            ],
        )
        + callout(
            "An unrecognised option is not silently accepted. Fafnir answers "
            "<code>info string unknown UCI option</code>, which makes configuration mistakes visible in the GUI log.",
            "info",
            "Honest failure",
        )
        + '<p class="card__text">Syzygy tablebases and opening books are not declared. They will appear only '
        "together with a working implementation — not as placeholders.</p>"
        "</div></section>"

        '<section class="section section--soft"><div class="container">'
        + section_head(
            "Builds",
            "One source tree, five instruction sets.",
            "Every architecture compiles into its own object directory, so flags never leak between binaries. The default target is safe on any x86-64 CPU.",
        )
        + table(
            ["Target", "Instruction set", "Typical hardware", "Median NPS*"],
            [
                ["<code>make</code> / <code>x86-64</code>", "SSE2", "Any x86-64 CPU", "219,097"],
                ["<code>ARCH=x86-64-ssse3</code>", "SSSE3", "Core 2 and later", "302,072"],
                ["<code>ARCH=x86-64-sse4.2</code>", "SSE4.2 + POPCNT", "Nehalem / Bulldozer and later", "312,614"],
                ["<code>ARCH=x86-64-avx2</code>", "AVX2 + BMI2 + POPCNT", "Haswell / Excavator and later", "419,868"],
                ["<code>ARCH=x86-64-avx512</code>", "AVX-512 + VNNI + PEXT", "Skylake-X / Ice Lake and later", "446,779"],
                ["<code>ARCH=native</code>", "Detected at compile time", "This build machine only", "—"],
            ],
        )
        + '<p class="card__text">*Depth 20, start position, median of four interleaved runs on the reference '
        'machine. See <a href="benchmarks.html">benchmarks</a> for the full protocol.</p>'
        '<div class="grid grid--3 mt-lg">'
        + card("Windows", "Build with MSYS2 g++ or use a published archive. The Makefile handles cmd.exe and POSIX shells for mkdir.", icon="⊞", href="docs-build.html#windows")
        + card("Linux", "A C++17 toolchain and make. CI validates g++ on Ubuntu runners; other compilers are accepted through CXX= without CI coverage.", icon="◉", href="docs-build.html#linux")
        + card("WebAssembly", "Emscripten output in <code>build-wasm/Fafnir.js</code> and <code>Fafnir.wasm</code>. The public web edition is in preparation.", icon="◇", href="docs-wasm.html")
        + "</div></div></section>"

        '<section class="section"><div class="container">'
        + section_head("Interfaces", "Native first, web next.", "Fafnir speaks UCI. A small C API is reserved for the WebAssembly edition, and the old JNI bridge is gone.")
        + '<div class="grid grid--3">'
        + card("UCI front end", "The supported interface today: any GUI that speaks UCI can drive the engine, including Arena, Cute Chess, En Croissant and BanksiaGUI.", icon="⌘", href="docs-uci.html")
        + card("WASM C API", "Five functions: <code>fafnir_init</code>, <code>set_fen</code>, <code>go_movetime</code>, <code>stop</code> and <code>eval</code>.", icon="◇", href="docs-wasm.html")
        + card("No JNI", "The legacy Java bridge did not compile against the vendored SFNNv17 snapshot and was removed in 2.0 rather than shipped broken.", icon="×", href="changelog.html")
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container container--narrow">'
        + section_head("Deliberate limits", "What Fafnir does not pretend to do.")
        + '<div class="faq">'
        '<details class="faq__item"><summary>No Syzygy tablebases yet</summary><div class="faq__body"><p>The protocol does not declare a tablebase option. When support lands, it will be implemented and tested before it is announced.</p></div></details>'
        '<details class="faq__item"><summary>No opening book</summary><div class="faq__body"><p>Book selection belongs to the GUI or tournament manager. Fafnir does not ship a book or a book option.</p></div></details>'
        '<details class="faq__item"><summary>No WASM public build yet</summary><div class="faq__body"><p>CI produces a <code>Fafnir-WASM-Build</code> artifact, but the public web edition is still in preparation. The API is documented so it can be reviewed and tested early.</p></div></details>'
        '<details class="faq__item"><summary>No telemetry, no update check</summary><div class="faq__body"><p>The engine is a local process. It does not open network connections, phone home, or check for updates.</p></div></details>'
        "</div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Read the protocol, then break it.</h2>"
        '<p>A reference is only useful if it matches the binary. Every command and option here is implemented in the current source.</p></div>'
        '<div class="btn-row">' + btn("UCI reference", "docs-uci.html", "primary") + btn("Download 2.0", "releases.html#downloads", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "features.html",
        "Features",
        "Fafnir's search stack, embedded SFNNv17 NNUE evaluation, five real UCI options, multi-architecture builds and documented limits.",
        body,
    )


def benchmarks() -> None:
    rows = []
    for arch, data in BENCH.items():
        rel = data["nps"] / BENCH["x86-64"]["nps"]
        rows.append([
            f"<code>{arch}</code>",
            data["label"],
            f"{data['nps']:,}",
            f"{data['ms'] / 1000:.2f} s",
            f"{rel:.2f}×",
        ])

    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Benchmarks", "")])
        + '<span class="eyebrow">Performance</span>'
        '<h1 class="page-head__title">Measured, not marketed.</h1>'
        '<p class="page-head__lead">One machine, one position, one protocol. The table below is reproducible '
        "with the published binaries and the same command. No cherry-picked opponent, no invented Elo.</p>"
        '<div class="btn-row page-head__meta">' + btn("Reproduce the run", "#reproduce", "primary")
        + btn("Read the testing docs", "docs-testing.html", "secondary") + "</div>"
        "</div></div>"

        '<section class="section"><div class="container">'
        '<div class="stats">'
        + stat("553,530", "Nodes to complete depth 20")
        + stat("446,779", "NPS · AVX-512 median")
        + stat("1.24 s", "Depth-20 time · AVX-512")
        + stat("2.04×", "AVX-512 vs safe SSE2")
        + "</div>"
        '<div class="callout callout--info mt-lg"><span class="callout__icon">i</span><div>'
        '<span class="callout__title">What this is not</span>'
        "NPS is a throughput measurement, not a rating. Two engines with the same NPS can differ by hundreds "
        "of Elo because of search quality, evaluation and time management. Fafnir publishes speed here and "
        "regression results in the repository; it does not claim an Elo from either.</div></div>"
        "</div></section>"

        '<section class="section section--soft"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Reference machine</span><h2>The hardware, in full.</h2>'
        '<p class="lead mt-md">Benchmarks without a machine description are anecdotes. This is the exact environment used for the table.</p>'
        '<div class="btn-row mt-md">' + badge("Windows 11", "outline") + badge("x86-64", "outline") + badge("No overclock", "outline") + "</div></div>"
        '<div class="card">'
        + table(
            ["Component", "Specification"],
            [
                ["CPU", "Intel Core i7-1165G7 (Tiger Lake), 4 cores / 8 threads, 2.80 GHz base"],
                ["Operating system", "Windows 11 x64"],
                ["Compiler", "g++ 15.2.0 (MSYS2, Rev13)"],
                ["Build", "<code>make all-arches -j8</code>"],
                ["Optimisation", "<code>-O3 -flto=auto -funroll-loops -fomit-frame-pointer</code>"],
                ["Position", "<code>startpos</code>"],
                ["Depth", "20"],
                ["Hash", "16 MB (default)"],
                ["Threads", "1"],
                ["Book / tablebase", "None"],
                ["Runs", "4 interleaved rounds, median reported (2 Nov 2026)"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section" id="results"><div class="container">'
        + section_head(
            "Fixed-depth results",
            "All five instruction sets, same node count.",
            "Every build searches the identical 553,530-node tree at depth 20, so the time difference is instruction-level throughput rather than a different search.",
        )
        + table(
            ["Build", "ISA baseline", "Median NPS", "Depth-20 time", "Relative"],
            rows,
        )
        + '<div class="grid grid--3 mt-lg">'
        + card("AVX2 is the practical choice", "Most x86-64 CPUs from Haswell (2013) and Excavator (2015) onward support AVX2. It delivers 1.92× the SSE2 throughput in this test.", icon="↑")
        + card("AVX-512 adds a small step", "The AVX-512 + VNNI build reaches 2.04× SSE2 here. Gains depend on the CPU's vector width and clock behaviour under load.", icon="＋")
        + card("SSE4.2 ≈ SSSE3 in this position", "POPCNT helps many workloads but not this one enough to separate the two builds reliably; the medians differ by about 3%.", icon="≈")
        + "</div>"
        + callout(
            "The medians were collected in interleaved rounds to reduce thermal bias. Individual runs ranged from "
            "205k to 232k NPS on SSE2 and 404k to 464k NPS on AVX-512. A laptop CPU is not a laboratory instrument.",
            "warning",
            "Variance is real",
        )
        + "</div></section>"

        '<section class="section section--soft"><div class="container"><div class="feature-split">'
        '<div><span class="eyebrow">Three-second run</span><h2>What a real session looks like.</h2>'
        '<p class="feature-split__text">With <code>go movetime 3000</code> on the same machine, the native build '
        'reached depth 22 and returned <code>e2e4</code>. The exact output is below; no fields were edited.</p>'
        '<ul class="feature-split__list">'
        "<li>Depth 22, seldepth 40</li>"
        "<li>955,912 nodes in 2,057 ms</li>"
        "<li>464,711 NPS</li>"
        "<li><code>bestmove e2e4 ponder c7c5</code></li>"
        "</ul></div>"
        '<div class="console"><div class="console__head"><span class="mockup__dots"><i></i><i></i><i></i></span>'
        "<span>movetime 3000 · native</span></div><div class=\"console__body\">"
        '<div class="console__line"><span class="console__cmd">go movetime 3000</span></div>'
        '<div class="console__line"><span class="console__info">info depth 20 seldepth 34 score cp 35 nodes 553530 nps 464760 time 1191</span></div>'
        '<div class="console__line"><span class="console__info">info depth 21 seldepth 34 score cp 45 nodes 744394 nps 466412 time 1596</span></div>'
        '<div class="console__line"><span class="console__info">info depth 22 seldepth 40 score cp 46 nodes 955912 nps 464711 time 2057</span></div>'
        '<div class="console__line"><span class="console__dim">pv e2e4 c7c5 g1f3 b8c6 f1b5 g7g6 e1g1 f8g7 c2c3 e7e5</span></div>'
        '<div class="console__line"><span class="console__best">bestmove e2e4 ponder c7c5</span></div>'
        "</div></div></div></div></section>"

        '<section class="section" id="reproduce"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Methodology</span><h2>Reproduce it in five steps.</h2>'
        '<ol class="steps">'
        "<li><h3>Build every architecture</h3><p><code>make all-arches -j8</code> writes <code>bin/Fafnir-&lt;arch&gt;</code>.</p></li>"
        "<li><h3>Close everything else</h3><p>Benchmarks are sensitive to background CPU load. Use the same power profile and leave the machine idle.</p></li>"
        "<li><h3>Send the same commands</h3><p><code>uci</code>, <code>isready</code>, <code>position startpos</code>, <code>go depth 20</code>. Default Hash and Threads.</p></li>"
        "<li><h3>Read the final info line</h3><p>The last <code>info depth 20</code> line gives nodes, NPS and time. Ignore the ponder move.</p></li>"
        "<li><h3>Interleave and repeat</h3><p>Run each build in a different order, at least three times, and report the median.</p></li>"
        "</ol></div>"
        '<div>'
        + code_block(
            """# Linux / MSYS2
make all-arches -j8

for arch in x86-64 x86-64-ssse3 x86-64-sse4.2 x86-64-avx2 x86-64-avx512; do
  printf 'uci\\nisready\\nposition startpos\\ngo depth 20\\nquit\\n' \\
    | ./bin/Fafnir-$arch
 done""",
            "bash",
            "Fixed-depth benchmark",
        )
        + code_block(
            """# Windows PowerShell
make all-arches -j8

"uci`nisready`nposition startpos`ngo depth 20`nquit`n" |
  .\\bin\\Fafnir-x86-64-avx2.exe""",
            "powershell",
            "Single build",
        )
        + callout(
            "The engine has no <code>bench</code> command. Use the UCI sequence above so the result is comparable "
            "with the table on this page.",
            "info",
            "No hidden benchmark mode",
        )
        + "</div></div></div></section>"

        '<section class="section section--soft"><div class="container container--narrow">'
        + section_head("Caveats", "What can move these numbers.")
        + '<div class="faq">'
        '<details class="faq__item"><summary>Compiler and flags</summary><div class="faq__body"><p>The project builds with <code>-O3</code>, LTO and gc-sections. A different compiler version, a different standard library, or PGO can shift throughput. CI validates g++ on Linux and Windows; other compilers are accepted but not covered.</p></div></details>'
        '<details class="faq__item"><summary>Thermals and power limits</summary><div class="faq__body"><p>A thin laptop can lose 15–25% NPS after a few minutes of sustained search. The interleaved medians above are an attempt to reduce that bias, not to eliminate it.</p></div></details>'
        '<details class="faq__item"><summary>Hash size and position</summary><div class="faq__body"><p>Larger hash tables reduce re-search on long games but do not change raw NPS much. Tactical positions with many captures can hit different evaluation paths; <code>startpos</code> is a convention, not a universal proxy.</p></div></details>'
        '<details class="faq__item"><summary>Operating system</summary><div class="faq__body"><p>Windows and Linux use different schedulers and C runtimes. The published archives are built by GitHub Actions on each platform; compare numbers within one OS.</p></div></details>'
        "</div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Want the correctness numbers too?</h2><p>The EPD suite measures move quality on 100 positions, with a strict best-move hit rule.</p></div>"
        '<div class="btn-row">' + btn("Testing guide", "docs-testing.html", "primary") + btn("Download 2.0", "releases.html#downloads", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "benchmarks.html",
        "Benchmarks",
        "Reproducible Fafnir benchmarks across SSE2, SSSE3, SSE4.2, AVX2 and AVX-512 on an Intel i7-1165G7, with exact flags, commands and caveats.",
        body,
    )


def releases() -> None:
    all_keys = ["windows-x64", "windows-sse4.2", "windows-avx2", "linux-x64", "linux-sse4.2", "linux-avx2"]
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Releases", "")])
        + '<div class="release-hero"><div>'
        + '<div class="badge-row mb-md">' + badge("Latest", "success") + badge("GPLv3", "outline") + badge("1 November 2026", "outline") + "</div>"
        '<h1 class="page-head__title">Fafnir 2.0</h1>'
        '<p class="page-head__lead">The first consolidated release: multi-architecture builds, an honest UCI surface, '
        "the SFNNv17 network committed as a plain blob, and a WASM build that compiles again.</p>"
        '<div class="btn-row page-head__meta">' + btn("Download", "#downloads", "primary", icon="download")
        + btn("Release notes", "release-2-0.html", "secondary") + btn("Changelog", "changelog.html", "ghost") + "</div>"
        "</div>"
        '<div class="card card--soft"><div class="row row--between mb-md"><span class="title-sm">At a glance</span>'
        + badge("Stable", "success") + "</div>"
        + table(
            ["Field", "Value"],
            [
                ["Tag", "<code>v2.0</code>"],
                ["Published", "1 November 2026"],
                ["Identity", "<code>id name Fafnir 2</code>"],
                ["License", "GNU GPL v3"],
                ["Assets", "6 archives + SHA256SUMS.txt"],
                ["Network", "SFNNv17 embedded"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></div>"

        '<section class="section" id="downloads"><div class="container">'
        + section_head(
            "Downloads",
            "Pick the build your CPU can execute.",
            "All archives contain the same engine source revision. The only difference is the instruction set compiled into the binary. When in doubt, use the safe x86-64 build.",
        )
        + '<div class="grid grid--2 grid--loose">'
        + '<div><div class="row row--between mb-sm"><span class="title-sm">Windows x64</span><span class="badge badge--outline">.zip</span></div>'
        '<div class="download-grid" style="grid-template-columns:1fr">'
        + download_card("windows-x64", "Safe · any x86-64 CPU")
        + download_card("windows-sse4.2", "SSE4.2 · 2009 and later")
        + download_card("windows-avx2", "AVX2 · Haswell / Ryzen")
        + "</div></div>"
        '<div><div class="row row--between mb-sm"><span class="title-sm">Linux x64</span><span class="badge badge--outline">.tar.gz</span></div>'
        '<div class="download-grid" style="grid-template-columns:1fr">'
        + download_card("linux-x64", "Safe · any x86-64 CPU")
        + download_card("linux-sse4.2", "SSE4.2 · 2009 and later")
        + download_card("linux-avx2", "AVX2 · Haswell / Ryzen")
        + "</div></div></div>"
        '<div class="card card--soft mt-lg"><div class="row row--between">'
        "<div><span class=\"title-sm\">All checksums</span>"
        '<p class="card__text mb-0">SHA-256 for every archive, plus the checksum file itself.</p></div>'
        + btn("SHA256SUMS.txt", DL + "SHA256SUMS.txt", "secondary", icon="download") + "</div></div>"
        "</div></section>"

        '<section class="section section--soft"><div class="container">'
        + section_head("Which build?", "The instruction set is not a suggestion.", "A binary compiled for AVX2 will crash with an illegal instruction on a CPU that does not support AVX2. Choose deliberately, or start safe.")
        + table(
            ["Build", "Requires", "Good default for"],
            [
                ["<code>x86-64</code>", "SSE2 only", "Any x86-64 machine, VMs, unknown hardware"],
                ["<code>sse4.2</code>", "SSE4.2 + POPCNT", "Intel Nehalem (2008) / AMD Bulldozer (2011) and later"],
                ["<code>avx2</code>", "AVX2 + BMI2 + POPCNT", "Intel Haswell (2013) / AMD Excavator (2015) and later"],
            ],
        )
        + callout(
            "The release does not include an AVX-512 archive. You can build one locally with "
            "<code>make ARCH=x86-64-avx512</code>; the benchmark page includes its measured throughput, "
            "but it is not a published binary.",
            "warning",
            "AVX-512 is build-it-yourself",
        )
        + '<div class="grid grid--3 mt-lg">'
        + card("Windows", "Unzip the archive and point your GUI at <code>Fafnir.exe</code>. No installer and no DLLs to register.", icon="⊞")
        + card("Linux", "Extract with <code>tar -xzf</code>, make the binary executable if needed, and pass its path to the GUI.", icon="◉")
        + card("Network", "Already inside the binary. Do not download a separate .nnue file for these archives.", icon="◈")
        + "</div></div></section>"

        '<section class="section"><div class="container">'
        + section_head("Assets", "Every file and every digest.", "These are the exact SHA-256 values published in SHA256SUMS.txt for v2.0.")
        + asset_rows(all_keys + ["sums"])
        + '<div class="grid grid--2 mt-lg">'
        + code_block(
            """# Linux / macOS
sha256sum Fafnir-x86-64-avx2-linux-x64.tar.gz
# compare with SHA256SUMS.txt, then:
sha256sum -c SHA256SUMS.txt""",
            "bash",
            "Verify on Linux",
        )
        + code_block(
            """# Windows PowerShell
Get-FileHash .\\Fafnir-x86-64-avx2-windows-x64.zip -Algorithm SHA256
# compare the Hash field with SHA256SUMS.txt""",
            "powershell",
            "Verify on Windows",
        )
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Release policy</span><h2>What a release means here.</h2>'
        '<ul class="checklist mt-md">'
        "<li>The tag, the source tree and the binaries refer to the same revision.</li>"
        "<li>Every asset has a published SHA-256 digest.</li>"
        "<li>The engine identity is part of the release: <code>id name Fafnir 2</code>.</li>"
        "<li>No asset is rebuilt silently after publication.</li>"
        "<li>Earlier tags are historical markers, not supported downloads.</li>"
        "</ul>"
        '<p class="card__text mt-md">The repository history was consolidated on 1 November 2026. The tags '
        "<code>fafnir2</code> and <code>1.3</code> remain reachable, but the releases they once backed were removed. "
        "Fafnir 2.0 is the only published release.</p></div>"
        '<div class="card"><span class="eyebrow">Compatibility</span>'
        + table(
            ["Item", "Fafnir 2.0"],
            [
                ["Protocol", "UCI"],
                ["Windows", "x64"],
                ["Linux", "x86_64"],
                ["WASM", "In preparation"],
                ["Tablebases", "Not declared"],
                ["Opening book", "Not included"],
                ["External network", "Not required"],
                ["Java / JNI", "Removed"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Need the exact changes?</h2><p>The release notes list every addition, removal and fix in 2.0, including the JNI removal and the WASM repair.</p></div>"
        '<div class="btn-row">' + btn("Release notes", "release-2-0.html", "primary") + btn("Full changelog", "changelog.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "releases.html",
        "Releases",
        "Download Fafnir 2.0 for Windows and Linux in safe x86-64, SSE4.2 and AVX2 builds, with SHA-256 checksums and build-selection guidance.",
        body,
    )


def release_2_0() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Releases", "releases.html"), ("2.0", "")])
        + '<div class="release-hero"><div>'
        + '<div class="badge-row mb-md">' + badge("Stable", "success") + badge("GPLv3", "outline") + "</div>"
        '<div class="release-hero__title"><h1>Fafnir 2.0</h1><span class="badge badge--outline">1 November 2026</span></div>'
        '<p class="page-head__lead">A consolidation release: the public history was rebuilt, the build system '
        "became multi-architecture, the UCI surface was reduced to what actually works, and the website and "
        "CI were brought back into the same tree.</p>"
        '<div class="btn-row page-head__meta">' + btn("Download assets", "releases.html#downloads", "primary", icon="download")
        + btn("Changelog", "changelog.html", "secondary") + "</div></div>"
        '<div class="card card--soft"><span class="eyebrow">Release identity</span>'
        + table(
            ["Field", "Value"],
            [
                ["Tag", "<code>v2.0</code>"],
                ["Engine id", "<code>Fafnir 2</code>"],
                ["Author", "<code>Anthony</code>"],
                ["Network", "SFNNv17, embedded"],
                ["License", "GNU GPL v3"],
                ["Assets", "6 archives + checksums"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></div>"

        '<section class="section"><div class="container">'
        + section_head("Highlights", "Four changes define this release.")
        + '<div class="grid grid--2">'
        + card(
            "Multi-architecture native builds",
            "One Makefile now produces safe x86-64, SSSE3, SSE4.2, AVX2 and AVX-512 binaries, each in its own object directory. Published archives cover Windows and Linux at three ISA levels.",
            icon="01",
            href="docs-build.html",
            link_label="Build guide",
        )
        + card(
            "A smaller, honest UCI surface",
            "Only Hash, Threads, MultiPV, Move Overhead and Aggressivity are declared. Unknown options answer <code>info string unknown UCI option</code> instead of pretending to work.",
            icon="02",
            href="docs-uci.html",
            link_label="UCI reference",
        )
        + card(
            "The network is a normal git blob",
            "The 98,668,604-byte SFNNv17 network is committed directly. Clones and CI embed the real file without an LFS smudge step, and the binary needs no external network.",
            icon="03",
            href="docs-nnue.html",
            link_label="NNUE details",
        )
        + card(
            "WASM builds again",
            "The workflow had been picking up the JNI bridge and the legacy probe. Both were excluded or removed; the first artifact with the real embedded network was produced on 1 November 2026.",
            icon="04",
            href="docs-wasm.html",
            link_label="WASM status",
        )
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container container--narrow"><div class="prose">'
        "<h2 id=\"added\">Added</h2>"
        "<ul>"
        "<li>Native multi-architecture build system in the root <code>Makefile</code>: <code>x86-64</code> safe default, "
        "<code>ssse3</code>, <code>sse4.2</code>, <code>avx2</code>, <code>avx512</code> and <code>native</code>, with "
        "per-architecture object directories and suffix-signed binaries (<code>bin/Fafnir-&lt;arch&gt;</code>).</li>"
        "<li>CI workflow <code>build-native.yml</code>: Linux and Windows builds for x86-64, SSE4.2 and AVX2, a UCI smoke "
        "test, and packaged artifacts.</li>"
        "<li>GPL v3 <code>LICENSE</code> at the repository root and a provenance overview in <code>third_party/VENDORED.md</code>.</li>"
        "<li>This documentation site, generated from a single source of truth and deployed to GitHub Pages.</li>"
        "</ul>"
        "<h2 id=\"changed\">Changed</h2>"
        "<ul>"
        "<li>The UCI interface declares only implemented options; unrecognised options answer "
        "<code>info string unknown UCI option</code>.</li>"
        "<li>Project renamed FlashBit → Fafnir across the repository, website and identity strings.</li>"
        "<li>The NNUE network is committed as a plain git blob again (no Git LFS).</li>"
        "<li><code>Makefile.wasm</code> and the CI build are portable across cmd.exe, MSYS2 sh, Linux and macOS shells "
        "(idempotent mkdir, no <code>2&gt;nul</code>).</li>"
        "</ul>"
        "<h2 id=\"removed\">Removed</h2>"
        "<ul>"
        "<li>The unbuildable JNI bridge (<code>src/bridges/java_api.{h,cpp}</code>, <code>integrations/java-jni/</code>): "
        "its legacy probe API does not compile against the vendored <code>sfdev</code> snapshot.</li>"
        "<li>Long-dead Stockfish <code>probe/</code> code is no longer referenced by any build. CI no longer mutates the "
        "vendored <code>incbin.h</code>; network embedding is disabled for WASM via <code>NNUE_EMBEDDING_OFF</code>.</li>"
        "</ul>"
        "<h2 id=\"fixed\">Fixed</h2>"
        "<ul>"
        "<li><code>build-wasm.yml</code> compiles again. It had been broken since the 31 October source reorganisation "
        "pulled in JNI and legacy probe sources.</li>"
        "<li>Windows <code>mkdir</code> no longer aborts when the directory already exists during native builds.</li>"
        "</ul>"
        "</div></div></section>"

        '<section class="section"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Upgrade notes</span><h2>What changes for you.</h2>'
        '<div class="defs">'
        '<div class="defs__row"><dt>Binary name</dt><dd>Published archives use <code>Fafnir.exe</code> on Windows and a Linux binary in the extracted directory.</dd></div>'
        '<div class="defs__row"><dt>Network file</dt><dd>Delete any external network path from your GUI configuration; 2.0 embeds the network.</dd></div>'
        '<div class="defs__row"><dt>Unknown options</dt><dd>Configuration typos now produce an explicit <code>info string</code> line instead of silence.</dd></div>'
        '<div class="defs__row"><dt>JNI users</dt><dd>The Java bridge is gone. Use UCI, or the documented WASM C API when the web edition ships.</dd></div>'
        '<div class="defs__row"><dt>History</dt><dd>Tags <code>fafnir2</code> and <code>1.3</code> remain historical markers; their releases were removed.</dd></div>'
        "</div></div>"
        '<div class="card"><span class="eyebrow">Known limitations</span>'
        + table(
            ["Area", "Status in 2.0"],
            [
                ["Syzygy tablebases", "Not implemented, not declared"],
                ["Opening book", "Not included"],
                ["WASM public edition", "In preparation"],
                ["AVX-512 archive", "Build locally; not published"],
                ["Other compilers", "Accepted via <code>CXX=</code>; CI validates g++ only"],
                ["Java / JNI", "Removed"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section section--soft"><div class="container">'
        + section_head("Assets", "Fafnir 2.0 downloads", "Six archives and a checksum file. Every digest below is the published value.")
        + asset_rows(["windows-x64", "windows-sse4.2", "windows-avx2", "linux-x64", "linux-sse4.2", "linux-avx2", "sums"])
        + '<div class="btn-row mt-md">' + btn("Open the release on GitHub", REPO + "/releases/tag/v2.0", "secondary", icon="github") + "</div>"
        "</div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Read the full history.</h2><p>The changelog includes the 2.0 consolidation and the earlier historical tags.</p></div>"
        '<div class="btn-row">' + btn("Changelog", "changelog.html", "primary") + btn("Roadmap", "roadmap.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "release-2-0.html",
        "Fafnir 2.0 release notes",
        "Fafnir 2.0 release notes: multi-architecture builds, honest UCI options, the embedded SFNNv17 network, WASM fixes, assets and SHA-256 checksums.",
        body,
    )


def changelog() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Changelog", "")])
        + '<span class="eyebrow">History</span><h1 class="page-head__title">Changelog</h1>'
        '<p class="page-head__lead">Every notable change, in the order it happened. The project uses descriptive '
        "commit messages rather than semantic presets, and earlier tags are kept as historical markers.</p>"
        "</div></div>"

        '<section class="section"><div class="container container--narrow">'
        '<div class="filter-bar" data-filter-group="changelog">'
        '<span class="filter-bar__label">Filter</span>'
        '<button class="filter is-active" type="button" data-filter="all">All</button>'
        '<button class="filter" type="button" data-filter="added">Added</button>'
        '<button class="filter" type="button" data-filter="changed">Changed</button>'
        '<button class="filter" type="button" data-filter="removed">Removed</button>'
        '<button class="filter" type="button" data-filter="fixed">Fixed</button>'
        "</div>"

        '<article class="changelog-entry" data-entry>'
        '<div class="changelog-entry__head"><span class="changelog-entry__version">2.0</span>'
        '<span class="changelog-entry__date">1 November 2026</span>' + badge("Current", "success") + "</div>"
        '<p class="card__text">The public history was consolidated into a single descriptive snapshot. Earlier '
        "development revisions remain reachable through the <code>fafnir2</code> and <code>1.3</code> tags; the "
        "older releases they once backed were removed.</p>"

        '<div class="change-group" data-tag="added"><h4>Added</h4><ul>'
        "<li>Native multi-architecture build system in the root <code>Makefile</code> — safe <code>x86-64</code>, "
        "<code>ssse3</code>, <code>sse4.2</code>, <code>avx2</code>, <code>avx512</code> and <code>native</code> — "
        "with per-architecture object directories and <code>bin/Fafnir-&lt;arch&gt;</code> outputs.</li>"
        "<li><code>build-native.yml</code>: Linux and Windows builds for x86-64, SSE4.2 and AVX2 with a UCI smoke test "
        "and packaged artifacts.</li>"
        "<li>GPL v3 <code>LICENSE</code> and <code>third_party/VENDORED.md</code>, documenting ChessLib (MIT), the "
        "Stockfish SFNNv17 snapshot (GPLv3), the NNUE network and incbin (Unlicense).</li>"
        "<li>A full documentation site with protocol, build, NNUE, WASM, testing and architecture references.</li>"
        "</ul></div>"

        '<div class="change-group" data-tag="changed"><h4>Changed</h4><ul>'
        "<li>The UCI interface declares only implemented options (<code>Hash</code>, <code>Threads</code>, "
        "<code>MultiPV</code>, <code>Move Overhead</code>, <code>Aggressivity</code>). Unrecognised options answer "
        "<code>info string unknown UCI option</code>.</li>"
        "<li>Project renamed FlashBit → Fafnir (repository, website, identity strings).</li>"
        "<li>The NNUE network is committed as a plain git blob again (no Git LFS), so clones and CI embed the real "
        "network without an LFS smudge step.</li>"
        "<li><code>Makefile.wasm</code> and the CI build are portable across cmd.exe, MSYS2 sh, Linux and macOS shells "
        "(idempotent mkdir, no <code>2&gt;nul</code>).</li>"
        "</ul></div>"

        '<div class="change-group" data-tag="removed"><h4>Removed</h4><ul>'
        "<li>The unbuildable JNI bridge (<code>src/bridges/java_api.{h,cpp}</code>, <code>integrations/java-jni/</code>). "
        "Its legacy probe API does not compile against the vendored <code>sfdev</code> snapshot.</li>"
        "<li>Long-dead Stockfish <code>probe/</code> code is no longer referenced by any build. CI no longer mutates the "
        "vendored <code>incbin.h</code>; network embedding is disabled for WASM via <code>NNUE_EMBEDDING_OFF</code>.</li>"
        "</ul></div>"

        '<div class="change-group" data-tag="fixed"><h4>Fixed</h4><ul>'
        "<li><code>build-wasm.yml</code> compiles again after the 31 October reorganisation broke it by pulling in JNI "
        "and legacy probe sources.</li>"
        "<li>Windows <code>mkdir</code> no longer aborts when directories already exist during native builds.</li>"
        "</ul></div>"
        "</article>"

        '<article class="changelog-entry" data-entry>'
        '<div class="changelog-entry__head"><span class="changelog-entry__version">Earlier</span>'
        '<span class="changelog-entry__date">Before 1 November 2026</span>' + badge("Historical", "outline") + "</div>"
        '<p class="card__text">Revisions before the consolidation are tagged <code>fafnir2</code> (engine snapshot) and '
        "<code>1.3</code>. Their published releases were removed, so these tags are historical markers rather than "
        "supported downloads.</p>"
        '<div class="change-group" data-tag="changed"><h4>Context</h4><ul>'
        "<li>FlashBit was the previous project name.</li>"
        "<li>The engine moved to the Stockfish development SFNNv17 evaluation architecture with a single-output network.</li>"
        "<li>The public Git history was reduced to keep the repository auditable and the network as a normal blob.</li>"
        "</ul></div>"
        "</article>"

        '<div class="filter-empty" data-filter-empty>No entries match this filter.</div>'

        '<div class="mt-lg">' + callout(
            "Changelog entries describe shipped behaviour, not intentions. Forward-looking work lives on the "
            '<a href="roadmap.html">roadmap</a> and is explicitly marked as unscheduled.',
            "info",
            "Shipped vs planned",
        ) + "</div>"
        "</div></section>"
    )
    emit(
        "changelog.html",
        "Changelog",
        "Fafnir changelog: the 2.0 consolidation, multi-architecture builds, UCI option changes, NNUE blob handling, JNI removal and WASM fixes.",
        body,
    )


def roadmap() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Roadmap", "")])
        + '<span class="eyebrow">Direction</span><h1 class="page-head__title">Roadmap</h1>'
        '<p class="page-head__lead">No dates, no promises disguised as plans. This page records what has shipped, '
        "what is actively being prepared, and what is only being explored. Anything not backed by working code stays "
        "out of the engine and out of the UCI option list.</p>"
        "</div></div>"

        '<section class="section"><div class="container">'
        '<div class="roadmap">'
        '<div class="roadmap__col"><div class="roadmap__head"><h3>Shipped</h3><span class="roadmap__count">5 items</span></div>'
        '<div class="roadmap__card">' + badge("2.0", "success") + "<h4>Multi-architecture native builds</h4><p>Safe x86-64 default plus SSSE3, SSE4.2, AVX2, AVX-512 and native targets.</p></div>"
        '<div class="roadmap__card">' + badge("2.0", "success") + "<h4>SFNNv17 network embedded</h4><p>98,668,604-byte network loaded with incbin; no external file required.</p></div>"
        '<div class="roadmap__card">' + badge("2.0", "success") + "<h4>Honest UCI surface</h4><p>Five implemented options; unknown options rejected with an info string.</p></div>"
        '<div class="roadmap__card">' + badge("2.0", "success") + "<h4>WASM CI repaired</h4><p>Emscripten build produces an artifact again, with embedding disabled.</p></div>"
        '<div class="roadmap__card">' + badge("2.0", "success") + "<h4>Documentation site</h4><p>Protocol, build, NNUE, testing, architecture and troubleshooting references.</p></div>"
        "</div>"

        '<div class="roadmap__col"><div class="roadmap__head"><h3>Next</h3><span class="roadmap__count">3 items</span></div>'
        '<div class="roadmap__card">' + badge("In preparation", "warning") + "<h4>Public WASM edition</h4><p>Wrap the five C functions in a browser build with a small board UI. The API is documented before the UI exists.</p></div>"
        '<div class="roadmap__card">' + badge("Planned", "outline") + "<h4>Syzygy tablebase support</h4><p>Implement, test and only then declare the UCI options. No placeholder options in the meantime.</p></div>"
        '<div class="roadmap__card">' + badge("Planned", "outline") + "<h4>More regression coverage</h4><p>Keep the 100-position EPD suite green and add targeted positions for every search change.</p></div>"
        "</div>"

        '<div class="roadmap__col"><div class="roadmap__head"><h3>Exploring</h3><span class="roadmap__count">3 items</span></div>'
        '<div class="roadmap__card">' + badge("Unscheduled", "outline") + "<h4>Evaluation tuning</h4><p>Measure whether the current network and search parameters are the right trade-off on the project's own test set.</p></div>"
        '<div class="roadmap__card">' + badge("Unscheduled", "outline") + "<h4>Alternative compilers in CI</h4><p>Clang and MSVC are accepted through the Makefile but are not validated by CI today.</p></div>"
        '<div class="roadmap__card">' + badge("Unscheduled", "outline") + "<h4>Opening book support</h4><p>Only if it can be implemented without turning the engine into a book viewer. The GUI remains the better place for books.</p></div>"
        "</div></div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">How this works</span><h2>Three rules for the roadmap.</h2>'
        '<ul class="feature-split__list mt-md">'
        "<li><strong>Ship before declaring.</strong> A UCI option appears only when it does something.</li>"
        "<li><strong>Measure before claiming.</strong> Performance changes need a reproducible run, not a feeling.</li>"
        "<li><strong>Document the removal.</strong> Dropped interfaces are recorded in the changelog, not deleted silently.</li>"
        "</ul></div>"
        '<div class="card card--soft"><span class="eyebrow">Not on the roadmap</span>'
        + table(
            ["Not planned", "Why"],
            [
                ["Closed-source binaries", "The engine links GPLv3 code."],
                ["Telemetry or update checks", "A local engine should stay local."],
                ["Fake UCI options", "Declaring unimplemented features breaks GUIs."],
                ["A built-in GUI", "Use the GUI you already trust."],
                ["Elo marketing", "Speed and test results are published; ratings are not invented."],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Have a concrete proposal?</h2><p>Open an issue with the behaviour, the expected result and a reproducible position or command.</p></div>"
        '<div class="btn-row">' + btn("Open an issue", REPO + "/issues", "primary", icon="github") + btn("Contributing guide", "contributing.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "roadmap.html",
        "Roadmap",
        "Fafnir roadmap: shipped 2.0 work, the WASM public edition and Syzygy plans, and explicitly unscheduled explorations. No invented dates.",
        body,
    )
