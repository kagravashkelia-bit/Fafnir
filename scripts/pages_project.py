"""Project and legal pages: about, license, contributing, community, FAQ,
contact, privacy, terms and 404."""

from site_core import (
    REPO,
    badge,
    breadcrumbs,
    btn,
    callout,
    card,
    code_block,
    emit,
    icon_card,
    table,
)


def about() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("About", "")])
        + '<span class="eyebrow">Project</span>'
        '<h1 class="page-head__title">A dragon, a pawn, and a lot of search output.</h1>'
        '<p class="page-head__lead">Fafnir is an independent UCI chess engine written in C++17. It is not a '
        "Stockfish fork with a new name: the search, move picker, controller and protocol layer live in this "
        "repository. The evaluation uses the Stockfish development SFNNv17 architecture, which is why the "
        "whole project is GPLv3.</p>"
        "</div></div>"

        '<section class="section"><div class="container"><div class="split split--7-5">'
        '<div><span class="eyebrow">Origin</span><h2>From FlashBit to Fafnir.</h2>'
        '<p class="lead mt-md">The engine was previously published as FlashBit. The rename was not cosmetic: it '
        "came with a new identity chain, a rebuilt public history, a multi-architecture build system and a "
        "documentation site that matches the binary.</p>"
        "<p>On 1 November 2026 the public history was consolidated into a single descriptive snapshot. The goal "
        "was not to erase the past but to make the current tree auditable: one commit, 127 files, the real "
        "98,668,604-byte network as a normal Git blob, and no build artifacts or leftover scripts. Earlier "
        "revisions remain reachable through the <code>fafnir2</code> and <code>1.3</code> tags.</p>"
        '<div class="callout callout--info mt-md"><span class="callout__icon">i</span><div>'
        '<span class="callout__title">What “independent” means here</span>'
        "Fafnir does not claim to be stronger than any other engine, and it does not publish invented Elo. "
        "It publishes its search design, its build flags, its test suite and its measured throughput. "
        "Strength claims belong to game results, not to a marketing page.</div></div></div>"
        '<div class="card card--soft"><span class="eyebrow">Identity</span>'
        + table(
            ["Field", "Value"],
            [
                ["Name", "Fafnir"],
                ["Version", "2.0"],
                ["Author", "Anthony"],
                ["Language", "C++17"],
                ["Protocol", "UCI"],
                ["License", "GNU GPL v3"],
                ["Evaluation", "SFNNv17 NNUE, single output"],
                ["Native targets", "Windows x64, Linux x86_64"],
                ["WASM", "In preparation"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section section--soft"><div class="container">'
        + '<div class="section-head section-head--center"><span class="eyebrow">Principles</span>'
        "<h2>Four rules the project actually follows.</h2></div>"
        '<div class="grid grid--4">'
        + card("Ship it or omit it", "No placeholder UCI options, no disabled menu entries, no “coming soon” in the protocol. If it does not work, it is not declared.", icon="01")
        + card("Measure it or qualify it", "Performance numbers include the machine and the protocol. Strength is not inferred from NPS.", icon="02")
        + card("Document the removal", "Dropped interfaces are recorded in the changelog. Silent deletion is a bug in the documentation.", icon="03")
        + card("Keep the binary portable", "The default build is safe on any x86-64 CPU. Faster builds are opt-in and clearly labelled.", icon="04")
        + "</div></div></section>"

        '<section class="section"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Technical lineage</span><h2>What is ours, what is borrowed.</h2>'
        '<p class="lead mt-md">The boundary matters for both attribution and licensing. The table lists every '
        "major component and whether it is compiled into the engine.</p>"
        '<div class="btn-row mt-md">' + btn("License overview", "license.html", "secondary")
        + btn("Vendored provenance", REPO + "/blob/main/third_party/VENDORED.md", "ghost", icon="github") + "</div></div>"
        '<div class="card">'
        + table(
            ["Component", "Origin", "License", "Compiled"],
            [
                ["Search, MovePicker, controller, UCI", "This repository", "GPLv3", "Yes"],
                ["SFNNv17 evaluation", "Stockfish development snapshot", "GPLv3", "Subset"],
                ["Board and move generation", "ChessLib 0.9.4 (Disservin)", "MIT", "Yes"],
                ["Network embedding", "incbin (Dale Weiler)", "Unlicense", "Yes"],
                ["NNUE network", "Upstream SFNNv17 default", "Upstream license", "Data"],
                ["Legacy probe", "Older Stockfish snapshot", "GPLv3", "No"],
                ["Website", "This repository", "GPLv3 as part of the project", "Static"],
            ],
            cls="table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="split">'
        '<div><span class="eyebrow">The name</span><h2>Fafnir.</h2>'
        '<p class="lead mt-md">In Norse myth, Fafnir is a dragon guarding a hoard. The logo keeps the dragon and '
        "adds a pawn: the hoard is a chess position, and the engine is the thing that refuses to give it up.</p>"
        "<p>The mark is deliberately monochrome with a small blue circuit accent. It is the only brand flourish on "
        "the site; the interface stays white, black and light gray.</p></div>"
        '<div class="card card--soft center"><img src="assets/img/fafnir-mark-192.png" alt="Fafnir logo: a silver dragon coiled around a chess pawn" width="192" height="192" style="margin:0 auto 20px;border-radius:16px">'
        '<p class="card__text mb-0">The Fafnir mark, derived from the project logo.</p></div>'
        "</div></div></section>"

        '<section class="section"><div class="container">'
        + '<div class="section-head"><span class="eyebrow">Credits</span><h2>Standing on documented shoulders.</h2>'
        '<p class="lead">ChessLib provides the board representation. The Stockfish development snapshot provides '
        "the NNUE implementation. incbin makes the network portable. The mistakes are ours.</p></div>"
        '<div class="grid grid--3">'
        + card("Anthony", "Engine author, maintainer and the person behind the Fafnir identity. Responsible for the search, protocol layer and build system.", icon="A")
        + card("Disservin / ChessLib", "A fast, header-only chess library used for board state, move generation and FEN/UCI helpers. MIT licensed.", icon="C")
        + card("Stockfish contributors", "The SFNNv17 evaluation architecture, NNUE feature sets and the reference search style that informed this implementation. GPLv3.", icon="S")
        + "</div>"
        '<p class="card__text mt-md">The full provenance, including the single local patch to the vendored '
        'snapshot, is in <a href="' + REPO + '/blob/main/third_party/VENDORED.md">third_party/VENDORED.md</a>.</p>'
        "</div></section>"

        '<section class="section section--soft"><div class="container"><div class="cta-band">'
        "<div><h2>Read the code, not just the page.</h2><p>Every claim here has a file behind it. The repository is the primary source.</p></div>"
        '<div class="btn-row">' + btn("Open GitHub", REPO, "primary", icon="github") + btn("Documentation", "docs.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "about.html",
        "About",
        "About Fafnir: the FlashBit rename, an independent C++17 UCI engine, its principles, technical lineage, name and credits.",
        body,
    )


def license() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("License", "")])
        + '<span class="eyebrow">Legal</span><h1 class="page-head__title">License &amp; notices</h1>'
        '<p class="page-head__lead">The complete engine is distributed under the GNU General Public License, '
        "version 3. This page is a practical overview, not legal advice and not a substitute for the license text.</p>"
        '<div class="btn-row page-head__meta">' + btn("Read the GPLv3", REPO + "/blob/main/LICENSE", "primary", icon="github")
        + btn("Vendored provenance", REPO + "/blob/main/third_party/VENDORED.md", "secondary") + "</div>"
        "</div></div>"

        '<section class="section"><div class="container container--narrow"><div class="prose prose--legal">'
        '<div class="callout callout--warning"><span class="callout__icon">!</span><div>'
        '<span class="callout__title">Why GPLv3 applies to the whole engine</span>'
        "Fafnir links and distributes vendored Stockfish code, which is GPLv3. The combined work must therefore "
        "be distributed under the GPLv3. The license text is replicated at the repository root in "
        "<code>LICENSE</code>.</div></div>"
        "<h2 id=\"summary\">In plain language</h2>"
        "<p>You may run Fafnir for any purpose, study how it works, modify it and redistribute it. If you "
        "distribute a modified version, the GPLv3 requires you to make the corresponding source available under "
        "the same license and to preserve the notices. There is no warranty.</p>"
        "<p>This summary is intentionally incomplete. The binding terms are in the GPLv3 text. Where this page "
        "and the license disagree, the license wins.</p>"
        "<h2 id=\"components\">Component breakdown</h2>"
        + table(
            ["Component", "License", "Obligation / note"],
            [
                ["Fafnir engine as a whole", "GNU GPL v3", "Source must be made available when distributed"],
                ["Search, MovePicker, SearchController, UCI", "GNU GPL v3", "Part of the combined work"],
                ["Stockfish SFNNv17 snapshot", "GNU GPL v3", "Vendored; only the NNUE subset is compiled"],
                ["ChessLib 0.9.4 (Disservin)", "MIT", "Copyright and permission notice retained in the header"],
                ["incbin (Dale Weiler)", "Unlicense", "Public domain dedication; no additional restriction"],
                ["NNUE network", "Upstream license", "Redistributed with the SFNNv17 snapshot"],
                ["Website", "GPLv3 as part of the project", "May be reused as a reference under the same terms"],
            ],
        )
        + "<h2 id=\"source\">Source availability</h2>"
        "<p>The complete corresponding source is the public repository at "
        f'<a href="{REPO}">{REPO}</a>. The release archives are built from the same tree that contains the '
        "engine, the Makefiles, the tests, the vendored dependencies and the network. No binary-only components "
        "are required to build or run the engine.</p>"
        "<h2 id=\"network\">The NNUE network</h2>"
        "<p><code>assets/nnue/nn-252f33942263.nnue</code> is the default network of the vendored SFNNv17 snapshot. "
        "It is redistributed under the upstream license. Its SHA-256 is:</p>"
        + code_block(
            "252f33942263bc8b8f740ba8aec3fed5a159ff148113c47a55c18c33d6627ab3",
            "text",
            "Network SHA-256",
            light=True,
        )
        + "<h2 id=\"patches\">Local patch to vendored code</h2>"
        "<p>There is one documented local patch: <code>third_party/stockfish/sfdev/src/evaluate.h</code> changes "
        "<code>EvalFileDefaultName</code> to point at <code>assets/nnue/nn-252f33942263.nnue</code>. The patch is "
        "recorded in <code>third_party/VENDORED.md</code>. The legacy <code>third_party/stockfish/probe/</code> "
        "directory is present for provenance but is not compiled into any binary.</p>"
        "<h2 id=\"distribution\">If you redistribute a build</h2>"
        "<ul>"
        "<li>Include the GPLv3 license text and the copyright notices.</li>"
        "<li>Make the complete corresponding source available, including your modifications.</li>"
        "<li>Do not add restrictions that the GPLv3 forbids.</li>"
        "<li>Keep the network licensing information with the network.</li>"
        "<li>Do not imply that your build is an official Fafnir release unless it is.</li>"
        "</ul>"
        "<h2 id=\"warranty\">No warranty</h2>"
        "<p>The software is provided “as is”, without warranty of any kind, express or implied, including but "
        "not limited to the warranties of merchantability, fitness for a particular purpose and "
        "non-infringement. The authors are not liable for any claim, damages or other liability arising from "
        "the use of the software.</p>"
        "<h2 id=\"website\">Website content</h2>"
        "<p>The website is part of the project repository and may be reused as a reference under the GPLv3 as "
        "part of the project as a whole. The Fafnir name and logo identify the project; do not use them to "
        "suggest endorsement of an unrelated build.</p>"
        "<h2 id=\"contact\">License questions</h2>"
        "<p>Open an issue in the repository if something in the notices is unclear or appears incomplete. "
        "For a legal decision, consult a qualified lawyer; this page is not legal advice.</p>"
        "</div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="cta-band">'
        "<div><h2>Read the binding text.</h2><p>The GPLv3 is the actual license. The repository root contains the verbatim copy.</p></div>"
        '<div class="btn-row">' + btn("GPLv3 text", REPO + "/blob/main/LICENSE", "primary", icon="github")
        + btn("Vendored notices", REPO + "/blob/main/third_party/VENDORED.md", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "license.html",
        "License & notices",
        "Fafnir licensing: GPLv3 for the engine, MIT for ChessLib, Unlicense for incbin, the embedded NNUE network, source availability and redistribution duties.",
        body,
    )


def contributing() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Contributing", "")])
        + '<span class="eyebrow">Contribute</span><h1 class="page-head__title">Contributing</h1>'
        '<p class="page-head__lead">Bug reports, test positions, documentation fixes and focused patches are all '
        "welcome. The project is small, so the best contributions are specific, reproducible and easy to review.</p>"
        '<div class="btn-row page-head__meta">' + btn("Open an issue", REPO + "/issues", "primary", icon="github")
        + btn("Read the code", REPO, "secondary", icon="github") + "</div>"
        "</div></div>"

        '<section class="section"><div class="container">'
        + '<div class="section-head"><span class="eyebrow">Before you start</span><h2>Four useful ways to help.</h2></div>'
        '<div class="grid grid--4">'
        + icon_card("◎", "Reproduce a bug", "A minimal command, FEN or GUI action plus the raw output is worth more than a description.")
        + icon_card("✓", "Add a test", "A legal EPD position with a clear best move strengthens the regression net.")
        + icon_card("⌘", "Fix a narrow issue", "One behaviour per pull request. Avoid unrelated reformatting.")
        + icon_card("¶", "Improve the docs", "If a page is wrong about the binary, that is a bug in the docs.")
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container container--narrow"><div class="prose">'
        "<h2 id=\"setup\">Development setup</h2>"
        + code_block(
            """git clone https://github.com/kagravashkelia-bit/Fafnir.git
cd Fafnir
make ARCH=native -j8
printf 'uci\\nisready\\nquit\\n' | ./bin/Fafnir""",
            "bash",
            "Build and smoke test",
        )
        + "<p>On Windows, use MSYS2 with g++ and make. The default <code>make</code> target builds a safe "
        "x86-64 binary; <code>ARCH=native</code> is faster for local work but must not be published as a "
        "portable archive.</p>"
        "<h2 id=\"tests\">Run the tests</h2>"
        + code_block(
            """python tests/tests.py bin/Fafnir.exe --depth 18
python tests/tests.py bin/Fafnir --depth 18 --hash 64 --threads 1""",
            "bash",
            "EPD regression",
        )
        + "<p>Record the score before and after a search or evaluation change. A drop in one position may be "
        "noise; a drop across a category is a regression. See <a href=\"docs-testing.html\">Testing</a> for the "
        "suite structure and limitations.</p>"
        "<h2 id=\"code\">Code guidelines</h2>"
        "<ul>"
        "<li><strong>C++17.</strong> Match the surrounding style; do not introduce a parallel convention.</li>"
        "<li><strong>One concern per patch.</strong> A search change and a formatting sweep in the same PR are hard to review.</li>"
        "<li><strong>No unrelated reformatting.</strong> It obscures the diff and makes bisecting harder.</li>"
        "<li><strong>Keep the protocol honest.</strong> Do not declare a UCI option before it is implemented and tested.</li>"
        "<li><strong>Preserve portability.</strong> The Makefile must work in cmd.exe, MSYS2 sh, Linux and macOS shells.</li>"
        "<li><strong>Do not mutate vendored code</strong> unless the patch is documented in <code>third_party/VENDORED.md</code>.</li>"
        "</ul>"
        "<h2 id=\"search\">Changes to search or evaluation</h2>"
        "<p>Search and evaluation changes need evidence, not just a plausible argument. Include:</p>"
        "<ul>"
        "<li>The exact change and the reasoning.</li>"
        "<li>EPD results before and after, with the same depth, Hash and Threads.</li>"
        "<li>Any speed impact, measured with the documented UCI sequence.</li>"
        "<li>A note on whether the change affects reproducibility or the network.</li>"
        "</ul>"
        + callout(
            "A patch that gains one EPD position and loses three is not obviously an improvement. Report the "
            "whole result, including the losses.",
            "warning",
            "Report the misses too",
        )
        + "<h2 id=\"commits\">Commit messages</h2>"
        "<p>The project uses descriptive one-line messages rather than semantic presets. A good message says what "
        "changed and, when it is not obvious, why:</p>"
        + code_block(
            """Search: verify null-move cutoffs at shallow depth
UCI: answer unknown options with an info string
Build: make mkdir idempotent on cmd.exe
Docs: correct the Hash range in the UCI table""",
            "text",
            "Examples",
        )
        + "<h2 id=\"pr\">Pull request checklist</h2>"
        "<ul>"
        "<li>The tree builds with <code>make ARCH=native -j8</code>.</li>"
        "<li>The UCI smoke test prints <code>id name Fafnir 2</code>, <code>uciok</code> and <code>readyok</code>.</li>"
        "<li>Search/eval changes include EPD results before and after.</li>"
        "<li>The change does not add an unimplemented UCI option.</li>"
        "<li>Documentation is updated in the same PR when behaviour changes.</li>"
        "<li>No generated binaries, build directories or local editor files are committed.</li>"
        "</ul>"
        "<h2 id=\"bugs\">Bug reports</h2>"
        "<p>Include the operating system, CPU model, archive or commit, the exact command or GUI action, and the "
        "raw engine output. For an illegal instruction, state which ISA build you used. For a protocol issue, "
        "paste the complete handshake rather than a single line.</p>"
        "<h2 id=\"security\">Security</h2>"
        "<p>Do not open a public issue for a vulnerability that could affect users. Follow the process in "
        "<code>SECURITY.md</code> in the repository. The engine is local and does not open network connections, "
        "but build and download integrity still matter.</p>"
        "<h2 id=\"conduct\">Code of conduct</h2>"
        "<p>Be direct about code and considerate about people. Harassment, personal attacks and deliberate "
        "disruption are not welcome. The repository includes <code>CODE_OF_CONDUCT.md</code>; read it before a "
        "contentious discussion.</p>"
        "</div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Have a reproducible issue?</h2><p>Open it with the command, the position and the output. That is the fastest path to a fix.</p></div>"
        '<div class="btn-row">' + btn("Open an issue", REPO + "/issues", "primary", icon="github") + btn("Testing guide", "docs-testing.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "contributing.html",
        "Contributing",
        "Contribute to Fafnir: development setup, EPD testing, code guidelines, commit style, pull request checklist, bug reports and security.",
        body,
    )


def community() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Community", "")])
        + '<span class="eyebrow">Community</span><h1 class="page-head__title">A small project with a public trail.</h1>'
        '<p class="page-head__lead">There is no chat server and no newsletter. GitHub is the source of truth: '
        "issues, commits, workflow runs and releases all live in one place, and the history is public.</p>"
        "</div></div>"

        '<section class="section"><div class="container"><div class="grid grid--3">'
        + card("Issues", "Report a bug, ask a protocol question or propose a concrete change. Include a command or FEN and the raw output.", icon="◎", href=REPO + "/issues", link_label="Open issues")
        + card("Releases", "Download the current archives and read the published SHA-256 checksums. v2.0 is the only supported release.", icon="↓", href="releases.html", link_label="Releases")
        + card("Actions", "Native Windows/Linux builds, Emscripten builds and the Pages deployment run in public.", icon="⟳", href=REPO + "/actions", link_label="Workflow runs")
        + "</div>"
        '<div class="grid grid--3 mt-md">'
        + card("Control games", "PGN records of games against Stockfish and Reckless 0.9.0 are archived in <code>tests/records/</code>. They are records, not a current rating claim.", icon="♟", href=REPO + "/tree/main/tests/records", link_label="Browse records")
        + card("Documentation", "Protocol, build, NNUE and testing references, all generated from the same repository.", icon="¶", href="docs.html", link_label="Read docs")
        + card("License", "GPLv3, with a documented component breakdown and the single patch to the vendored snapshot.", icon="§", href="license.html", link_label="License")
        + "</div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">How to participate</span><h2>What actually helps.</h2>'
        '<ul class="feature-split__list mt-md">'
        "<li><strong>A reproducible report.</strong> Command, FEN, build and output — not a summary.</li>"
        "<li><strong>A test position.</strong> Legal, with a clear best move and a source.</li>"
        "<li><strong>A narrow patch.</strong> One behaviour, one reason, one test result.</li>"
        "<li><strong>A documentation correction.</strong> If the site disagrees with the binary, the site is wrong.</li>"
        "<li><strong>A benchmark run.</strong> Same protocol, different hardware — useful context, not a competition.</li>"
        "</ul></div>"
        '<div class="card card--soft"><span class="eyebrow">Expectations</span>'
        '<p class="card__text">Fafnir is maintained in spare time. There is no support SLA and no guaranteed '
        "response window. A clear report is more likely to get attention than a repeated bump.</p>"
        '<div class="defs mt-md">'
        '<div class="defs__row"><dt>Bug reports</dt><dd>GitHub issues, with logs</dd></div>'
        '<div class="defs__row"><dt>Feature ideas</dt><dd>Issues, marked as proposals</dd></div>'
        '<div class="defs__row"><dt>Security</dt><dd>Follow <code>SECURITY.md</code></dd></div>'
        '<div class="defs__row"><dt>License</dt><dd>Open an issue; not legal advice</dd></div>'
        "</div></div></div></div></section>"

        '<section class="section"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Etiquette</span><h2>Direct about code, considerate about people.</h2>'
        '<p class="lead mt-md">Disagreement is normal in engine development. A patch can be rejected without the '
        "author being rejected. Harassment, personal attacks and deliberate disruption are not welcome.</p>"
        '<p>The repository includes <code>CODE_OF_CONDUCT.md</code> and <code>SECURITY.md</code>. Read both before '
        "a contentious thread or a vulnerability report.</p></div>"
        '<div class="card"><span class="eyebrow">Credits</span>'
        + table(
            ["Contribution", "Credit"],
            [
                ["Engine, search, UCI, build system", "Anthony"],
                ["ChessLib board and move generation", "Disservin"],
                ["SFNNv17 evaluation and NNUE features", "Stockfish contributors"],
                ["incbin", "Dale Weiler"],
                ["Test positions", "Bratko-Kopec, Kaufman, Nolot, LCT-II, Rybka Forum"],
                ["Control games", "En Croissant / Lichess records"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="cta-band">'
        "<div><h2>Start with the repository.</h2><p>Read the code, run the tests, then open an issue if something does not match the docs.</p></div>"
        '<div class="btn-row">' + btn("GitHub", REPO, "primary", icon="github") + btn("Contributing", "contributing.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "community.html",
        "Community",
        "Fafnir community: GitHub issues, releases, CI runs, archived control games, contribution paths, expectations and credits.",
        body,
    )


def faq() -> None:
    qa = [
        ("Engine", [
            ("Is Fafnir a Stockfish fork?", "No. Fafnir is a separate engine with its own search, move picker, controller and UCI layer. It uses the Stockfish development SFNNv17 NNUE architecture and vendored evaluation code, which is why the combined work is GPLv3."),
            ("How strong is it?", "The project does not publish an invented Elo. It publishes its search design, a 100-position EPD regression suite and measured throughput. Strength is a property of game results, not a marketing page."),
            ("Does it use an opening book or tablebases?", "No. Neither is implemented or declared in 2.0. The GUI or tournament manager can supply a book; Syzygy support is on the roadmap but unscheduled."),
            ("Does the engine connect to the internet?", "No. It is a local process. There is no telemetry, update check or network request."),
        ]),
        ("Builds", [
            ("Which archive should I download?", "Start with the safe x86-64 build if you are unsure. AVX2 is the right choice for most CPUs from 2013 onward; SSE4.2 covers older machines. A build compiled for an ISA your CPU lacks will crash with an illegal instruction."),
            ("Why is the default build slower?", "The default target uses SSE2 so the binary runs on any x86-64 CPU. The AVX2 build is about 1.9× faster on the reference benchmark, and AVX-512 about 2.0×."),
            ("Can I build with Clang or MSVC?", "The Makefile accepts other compilers through CXX=, but CI validates g++ on Linux and Windows. A successful local build with another compiler is not a tested configuration."),
            ("The Windows build failed with mkdir. What now?", "That bug is fixed in 2.0. Pull the current Makefile, run make clean, and rebuild. The mkdir step is now idempotent."),
            ("Can I build for AVX-512?", "Yes: make ARCH=x86-64-avx512. The official release does not include an AVX-512 archive, but the build target exists and is benchmarked locally."),
        ]),
        ("Protocol", [
            ("Which UCI commands are supported?", "uci, isready, ucinewgame, position, go, stop, setoption and quit. The full reference lists the accepted go parameters and output fields."),
            ("Why are there only five options?", "Because only five are implemented: Hash, Threads, MultiPV, Move Overhead and Aggressivity. Declaring a non-functional option breaks GUIs and makes configuration dishonest."),
            ("What happens if I set an unknown option?", "The engine answers info string unknown UCI option. It does not silently accept the setting."),
            ("Does it support MultiPV?", "Yes, from 1 to 256 root variations. The multipv field identifies the rank in the info lines."),
            ("Does it support pondering?", "go ponder is accepted and the engine declares ponder support. ponderhit is not part of the documented command surface in 2.0; check the UCI reference before relying on it."),
        ]),
        ("NNUE", [
            ("Do I need to download a .nnue file?", "No. The native build embeds the network with incbin. The published archives are self-contained with respect to evaluation."),
            ("Which network is embedded?", "The SFNNv17 default network nn-252f33942263.nnue, 98,668,604 bytes, SHA-256 252f33942263bc8b8f740ba8aec3fed5a159ff148113c47a55c18c33d6627ab3."),
            ("Can I replace the network?", "Only at build time, and only with a compatible SFNNv17 network. The file name is referenced by EvalFileDefaultName in the vendored evaluate.h. Re-run the EPD suite after any change."),
            ("Why is the WASM build different?", "WASM is compiled with NNUE_EMBEDDING_OFF because a 94 MB network inside a browser artifact is impractical. The public web edition is still in preparation."),
        ]),
        ("Project", [
            ("Is there a public WASM build?", "Not yet. CI produces a Fafnir-WASM-Build artifact, and the C API is documented, but there is no supported browser release."),
            ("Why was the Git history consolidated?", "To make the current tree auditable: one snapshot, the real network as a normal blob, no build artifacts, and a documented provenance. Earlier tags remain reachable."),
            ("What happened to FlashBit?", "It was renamed to Fafnir. The rename covered the repository, website and identity strings; the changelog records it."),
            ("Where do I report a bug?", "Open a GitHub issue with the OS, CPU, build, exact command or GUI action, and the raw engine output."),
            ("Is the website open source?", "Yes, it lives in web/ in the same repository and is generated from scripts/build_site.py."),
        ]),
    ]

    blocks = []
    for group, items in qa:
        details = "".join(
            f'<details class="faq__item"><summary>{q}</summary><div class="faq__body"><p>{a}</p></div></details>'
            for q, a in items
        )
        blocks.append(f'<h2 id="{group.lower()}">{group}</h2><div class="faq">{details}</div>')

    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("FAQ", "")])
        + '<span class="eyebrow">Help</span><h1 class="page-head__title">Frequently asked questions</h1>'
        '<p class="page-head__lead">Short answers, grouped by the part of the project they concern. If the answer '
        "depends on the build, the docs page is linked next to it.</p>"
        "</div></div>"
        '<section class="section"><div class="container container--narrow">'
        + "".join(blocks)
        + '<div class="callout callout--info mt-lg"><span class="callout__icon">i</span><div>'
        '<span class="callout__title">Still stuck?</span>'
        'Read <a href="docs-troubleshooting.html">Troubleshooting</a> for diagnostics, or open an issue with the '
        "raw output. The more reproducible the report, the faster the answer.</div></div>"
        "</div></section>"
    )
    emit(
        "faq.html",
        "FAQ",
        "Fafnir FAQ: Stockfish lineage, strength claims, builds, UCI options, NNUE embedding, WASM status and project history.",
        body,
    )


def contact() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Contact", "")])
        + '<span class="eyebrow">Contact</span><h1 class="page-head__title">The repository is the contact surface.</h1>'
        '<p class="page-head__lead">There is no published support email and no chat server. GitHub issues keep the '
        "report, the discussion and the fix in one public thread, which is better for a project of this size.</p>"
        "</div></div>"

        '<section class="section"><div class="container"><div class="contact-grid">'
        '<div class="stack">'
        + card("Bug report", "Open an issue with the OS, CPU model, archive or commit, the exact command or GUI action, and the raw engine output. For an illegal instruction, state which ISA build you used.", icon="◎", href=REPO + "/issues/new", link_label="Open an issue", cls="card card--soft")
        + card("Feature proposal", "Describe the behaviour, the expected result, and why it belongs in the engine rather than the GUI. Proposals are discussed before implementation.", icon="＋", href=REPO + "/issues/new", link_label="Propose a change", cls="card card--soft")
        + card("Security", "Do not open a public issue for a vulnerability that could affect users. Follow the process in SECURITY.md in the repository.", icon="!", href=REPO + "/blob/main/SECURITY.md", link_label="Security policy", cls="card card--soft")
        + card("License question", "Open an issue if a notice is unclear or appears incomplete. This is not legal advice; consult a lawyer for a legal decision.", icon="§", href="license.html", link_label="License overview", cls="card card--soft")
        + "</div>"
        '<div><div class="contact-card"><span class="eyebrow">What to include</span><ol class="steps" style="margin-top:12px">'
        "<li><h3>Environment</h3><p>OS version, CPU model and build archive or commit.</p></li>"
        "<li><h3>Reproduction</h3><p>The shortest command, FEN or GUI action that shows the problem.</p></li>"
        "<li><h3>Output</h3><p>The raw engine log, not a paraphrase. Include the full UCI handshake for protocol issues.</p></li>"
        "<li><h3>Expectation</h3><p>What you expected and what happened instead.</p></li>"
        "</ol>"
        '<div class="callout callout--warning mt-md"><span class="callout__icon">!</span><div>'
        '<span class="callout__title">No SLA</span>'
        "Fafnir is maintained in spare time. A clear report is more likely to get attention; repeated bumps are "
        "not a priority signal.</div></div>"
        "</div></div></div></div></section>"

        '<section class="section section--soft"><div class="container"><div class="split split--top">'
        '<div><span class="eyebrow">Channels</span><h2>Where things live.</h2>'
        '<p class="lead mt-md">Each channel has one job. Using the right one keeps the trail searchable.</p></div>'
        '<div class="card">'
        + table(
            ["Channel", "Use it for"],
            [
                ["Issues", "Bugs, protocol questions, documented proposals"],
                ["Releases", "Downloading a supported build and checking hashes"],
                ["Actions", "CI status, native builds, WASM builds, Pages deployment"],
                ["Security policy", "Vulnerability reports, privately"],
                ["Code of conduct", "Community standards and enforcement"],
            ],
            cls="table--kv table--compact",
        )
        + "</div></div></div></section>"

        '<section class="section"><div class="container"><div class="cta-band">'
        "<div><h2>Ready to write a useful report?</h2><p>Include the command, the position and the output. That is usually enough.</p></div>"
        '<div class="btn-row">' + btn("Open an issue", REPO + "/issues/new", "primary", icon="github") + btn("Troubleshooting", "docs-troubleshooting.html", "secondary") + "</div>"
        "</div></div></section>"
    )
    emit(
        "contact.html",
        "Contact",
        "Contact the Fafnir project: GitHub issues for bugs and proposals, the security policy for vulnerabilities, and what to include in a report.",
        body,
    )


def privacy() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Privacy", "")])
        + '<span class="eyebrow">Legal</span><h1 class="page-head__title">Privacy</h1>'
        '<p class="page-head__lead">This website is static. It has no accounts, no forms, no analytics and no '
        "cookies. The only third-party requests are the font files and the links you choose to follow.</p>"
        '<p class="card__text mt-md">Last updated: 2 November 2026</p>'
        "</div></div>"
        '<section class="section"><div class="container container--narrow"><div class="prose prose--legal">'
        "<h2 id=\"site\">What the site collects</h2>"
        "<p>Nothing directly. The site is a set of static HTML, CSS, JavaScript and image files. It does not "
        "include an analytics script, an advertising pixel, a comment system or a contact form. It does not set "
        "cookies and does not write to <code>localStorage</code> or <code>sessionStorage</code>.</p>"
        "<h2 id=\"hosting\">Hosting</h2>"
        "<p>The site is published with GitHub Pages. GitHub, as the hosting provider, may process technical "
        "request data such as IP addresses, user agents and timestamps to deliver the site and protect its "
        "service. That processing is governed by GitHub's privacy statement, not by this project. The project "
        "does not receive or store those logs.</p>"
        "<h2 id=\"fonts\">Fonts and external resources</h2>"
        "<p>The pages load Inter and JetBrains Mono from Google Fonts. A request to Google Fonts discloses the "
        "visitor's IP address and browser metadata to Google. If you prefer to avoid that request, block the "
        "font domain or use a browser setting that prevents third-party font loading; the site falls back to "
        "system fonts.</p>"
        "<p>Links to GitHub, the release downloads and the Actions pages are ordinary links. Nothing is fetched "
        "from those domains unless you activate a link.</p>"
        "<h2 id=\"downloads\">Downloads</h2>"
        "<p>Release archives are hosted by GitHub Releases. Downloading one is a request to GitHub and is subject "
        "to GitHub's terms and privacy practices. The project does not embed a download counter or tracking "
        "parameter in the links.</p>"
        "<h2 id=\"engine\">The engine</h2>"
        "<p>Fafnir is a local UCI process. It does not open network connections, send telemetry, check for updates "
        "or transmit game data. Any data it uses stays on the machine where it runs.</p>"
        "<h2 id=\"children\">Children</h2>"
        "<p>The site is a technical documentation resource and is not directed at children. It does not knowingly "
        "collect personal information from anyone.</p>"
        "<h2 id=\"rights\">Your choices</h2>"
        "<p>Because the project does not collect personal data, there is no account to delete and no profile to "
        "export. For questions about hosting logs, contact GitHub. For a question about this page, open an issue "
        "in the repository.</p>"
        "<h2 id=\"changes\">Changes</h2>"
        "<p>If the site starts using analytics, a form or another data source, this page will be updated before "
        "the change takes effect. The date at the top identifies the current version.</p>"
        "<h2 id=\"contact\">Contact</h2>"
        "<p>Open an issue at <a href=\"" + REPO + "/issues\">github.com/kagravashkelia-bit/Fafnir/issues</a> "
        "for a privacy question about this site. Do not include personal or sensitive information in a public issue.</p>"
        "</div></div></section>"
    )
    emit(
        "privacy.html",
        "Privacy",
        "Fafnir website privacy: no analytics, cookies or forms; GitHub Pages hosting, Google Fonts requests and local engine behaviour.",
        body,
        progress=True,
    )


def terms() -> None:
    body = (
        '<div class="page-head"><div class="container">'
        + breadcrumbs([("Fafnir", "index.html"), ("Terms", "")])
        + '<span class="eyebrow">Legal</span><h1 class="page-head__title">Terms of use</h1>'
        '<p class="page-head__lead">These terms cover the website, not the engine license. The engine is '
        "distributed under the GNU GPL v3; the license text governs the software.</p>"
        '<p class="card__text mt-md">Last updated: 2 November 2026</p>'
        "</div></div>"
        '<section class="section"><div class="container container--narrow"><div class="prose prose--legal">'
        "<h2 id=\"informational\">Informational purpose</h2>"
        "<p>This site documents Fafnir and provides links to its source code, releases and documentation. It is "
        "not a commercial service and does not provide an account, a hosted engine or a support contract.</p>"
        "<h2 id=\"engine\">Engine license</h2>"
        "<p>The engine, including the vendored Stockfish components and the embedded NNUE network, is distributed "
        "under the GNU General Public License, version 3. Your rights and obligations for the software come from "
        "that license, not from this page. See <a href=\"license.html\">License &amp; notices</a> for the component "
        "breakdown and a link to the binding text.</p>"
        "<h2 id=\"downloads\">Downloads and no warranty</h2>"
        "<p>Binaries and source are provided “as is”, without warranty of any kind, express or implied. The "
        "authors are not liable for any claim, damages or other liability arising from the use of the software or "
        "from reliance on the documentation. Always verify the published SHA-256 checksum before running an "
        "archive.</p>"
        "<h2 id=\"accuracy\">Accuracy</h2>"
        "<p>The documentation describes the current release as accurately as possible. Software changes; a page "
        "may lag behind the source. If the site and the binary disagree, the binary and the source repository are "
        "authoritative. Report a documentation error through the issue tracker.</p>"
        "<h2 id=\"acceptable\">Acceptable use</h2>"
        "<p>Do not use the site to distribute malware, impersonate the project, or misrepresent a modified build "
        "as an official Fafnir release. Do not attempt to disrupt the hosting service or the repository.</p>"
        "<h2 id=\"trademarks\">Names and marks</h2>"
        "<p>The Fafnir name and logo identify the project. They are not a warranty of origin for third-party "
        "builds. If you redistribute a modified engine, make clear that it is not the official release.</p>"
        "<h2 id=\"external\">External links</h2>"
        "<p>The site links to GitHub, Google Fonts and other third-party resources. Those services have their own "
        "terms and privacy practices. The project does not control their content or availability.</p>"
        "<h2 id=\"changes\">Changes</h2>"
        "<p>These terms may be updated when the site changes. The date at the top identifies the current version. "
        "Continued use of the site after a change means you accept the updated terms.</p>"
        "<h2 id=\"contact\">Contact</h2>"
        "<p>Questions about these terms can be opened as an issue at "
        "<a href=\"" + REPO + "/issues\">github.com/kagravashkelia-bit/Fafnir/issues</a>. This page is not legal "
        "advice.</p>"
        "</div></div></section>"
    )
    emit(
        "terms.html",
        "Terms of use",
        "Fafnir website terms: informational purpose, GPLv3 engine license, no warranty, accuracy, acceptable use and external links.",
        body,
        progress=True,
    )


def not_found() -> None:
    body = (
        '<section class="error-page"><div class="container">'
        '<span class="error-page__code">HTTP 404</span>'
        "<h1>This line is not in the book.</h1>"
        '<p class="lead">The page you requested does not exist. It may have been renamed, or the link may be '
        "missing a segment. The engine is still here.</p>"
        '<div class="btn-row">' + btn("Back to home", "index.html", "primary")
        + btn("Documentation", "docs.html", "secondary") + "</div>"
        '<div class="error-page__links"><div class="container container--narrow">'
        '<div class="grid grid--3">'
        + card("Docs", "Protocol, build, NNUE and testing references.", href="docs.html", link_label="Open docs", cls="card card--soft")
        + card("Releases", "Download the current Windows and Linux archives.", href="releases.html", link_label="Releases", cls="card card--soft")
        + card("FAQ", "Short answers about builds, options and NNUE.", href="faq.html", link_label="Read FAQ", cls="card card--soft")
        + "</div></div></div>"
        "</div></section>"
    )
    emit(
        "404.html",
        "Page not found",
        "The requested Fafnir page does not exist. Return to the homepage, documentation, releases or FAQ.",
        body,
    )
