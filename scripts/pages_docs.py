"""Documentation pages: hub, getting started, build, UCI, search, NNUE, testing,
architecture, WebAssembly and troubleshooting."""

from site_core import (
    BENCH,
    DOCS_ORDER,
    REPO,
    asset_rows,
    badge,
    breadcrumbs,
    btn,
    callout,
    card,
    code_block,
    docs_meta,
    emit,
    icon_card,
    info_line,
    table,
)


def _crumbs(label: str) -> str:
    return breadcrumbs([("Fafnir", "index.html"), ("Docs", "docs.html"), (label, "")])


def _doc(slug: str, title: str, lead: str, toc: list[tuple[str, str, bool]], body: str,
         nav_label: str = "", updated: str = "2 November 2026") -> None:
    emit(
        slug,
        title,
        lead,
        body,
        layout="docs",
        toc=toc,
        progress=True,
        docs_title=title,
        docs_lead=lead,
        docs_meta=docs_meta(extra="<span>Reference</span>"),
        breadcrumbs=_crumbs(nav_label or title),
    )


def docs_hub() -> None:
    body = (
        '<div class="prose">'
        "<p>Fafnir is a UCI chess engine written in C++17. It uses the Stockfish development SFNNv17 "
        "evaluation architecture, a Stockfish-style search implemented in this repository, and native builds "
        "for Windows and Linux. The documentation is organised by the question you are trying to answer.</p>"
        '<div class="callout callout--info"><span class="callout__icon">i</span><div>'
        '<span class="callout__title">Version note</span>'
        "These pages describe Fafnir 2.0, engine identity <code>id name Fafnir 2</code>. If a command is not "
        "listed here, it is not part of the supported interface — even if an older build accepted it.</div></div>"
        "<h2 id=\"start\">Start here</h2>"
        "<p>If you have never run a UCI engine, follow the getting-started guide. It covers choosing a binary, "
        "connecting a GUI and reading the first session. If you want to compile the engine, go straight to the "
        "build guide.</p>"
        '<div class="grid grid--2 mt-md">'
        + card("Getting started", "Download a build, connect a GUI, run the first UCI session and verify the network is embedded.", icon="01", href="docs-getting-started.html", link_label="Open guide")
        + card("Build from source", "Requirements, Make targets, ISA flags, Windows/MSYS2, Linux and WebAssembly.", icon="02", href="docs-build.html", link_label="Build guide")
        + card("UCI reference", "Every supported command, go parameter, option and info field — with exact ranges.", icon="03", href="docs-uci.html", link_label="Protocol")
        + card("Troubleshooting", "Illegal instructions, silent GUIs, missing WASM tools and CI failures.", icon="04", href="docs-troubleshooting.html", link_label="Fix a problem")
        + "</div>"
        "<h2 id=\"map\">Documentation map</h2>"
        "<p>The table is the complete list of reference pages. Each page is self-contained; the sidebar keeps the "
        "rest of the documentation one click away.</p>"
        + table(
            ["Page", "What it answers"],
            [
                ['<a href="docs-getting-started.html">Getting started</a>', "How do I run Fafnir for the first time?"],
                ['<a href="docs-build.html">Build from source</a>', "Which make target should I use, and why?"],
                ['<a href="docs-uci.html">UCI reference</a>', "What exactly does the engine accept and emit?"],
                ['<a href="docs-search.html">Search internals</a>', "How does the engine choose a move?"],
                ['<a href="docs-nnue.html">NNUE evaluation</a>', "What network is embedded, and how is it loaded?"],
                ['<a href="docs-testing.html">Testing</a>', "How is correctness measured, and how do I reproduce it?"],
                ['<a href="docs-architecture.html">Architecture</a>', "Where does each responsibility live in the tree?"],
                ['<a href="docs-wasm.html">WebAssembly</a>', "What is the C API, and what is the current status?"],
                ['<a href="docs-troubleshooting.html">Troubleshooting</a>', "Why did the engine fail to start or analyse?"],
            ],
        )
        + "<h2 id=\"conventions\">Conventions used in these docs</h2>"
        "<ul>"
        "<li><strong>UCI commands</strong> are shown exactly as they are sent, without a trailing newline.</li>"
        "<li><strong>Ranges and defaults</strong> come from the engine's own option declarations, not from a generic UCI assumption.</li>"
        "<li><strong>Measured numbers</strong> identify the machine and protocol. They are never presented as universal constants.</li>"
        "<li><strong>Not implemented</strong> means the feature is absent from the protocol, not merely disabled by default.</li>"
        "<li><strong>Paths</strong> are relative to the repository root unless stated otherwise.</li>"
        "</ul>"
        "<h2 id=\"status\">Status of the interfaces</h2>"
        + table(
            ["Interface", "Status", "Where"],
            [
                ["Native UCI", '<span class="badge badge--success">Supported</span>', "Windows x64, Linux x86_64"],
                ["Multi-architecture builds", '<span class="badge badge--success">Supported</span>', "<code>Makefile</code>"],
                ["WebAssembly C API", '<span class="badge badge--warning">In preparation</span>', "<code>Makefile.wasm</code>"],
                ["JNI / Java", '<span class="badge badge--danger">Removed in 2.0</span>', "—"],
                ["Syzygy tablebases", '<span class="badge badge--outline">Not declared</span>', "—"],
                ["Opening book", '<span class="badge badge--outline">Not included</span>', "—"],
            ],
        )
        + "<h2 id=\"next\">Where to go next</h2>"
        "<p>For a first run, continue to <a href=\"docs-getting-started.html\">Getting started</a>. For the "
        "protocol contract, jump to the <a href=\"docs-uci.html\">UCI reference</a>. For the build matrix and "
        "performance differences, read <a href=\"docs-build.html\">Build from source</a> and the "
        "<a href=\"benchmarks.html\">benchmark page</a>.</p>"
        "</div>"
    )
    _doc(
        "docs.html",
        "Documentation",
        "The complete Fafnir reference: getting started, building, UCI protocol, search, NNUE, testing, architecture, WebAssembly and troubleshooting.",
        [("start", "Start here", False), ("map", "Documentation map", False), ("conventions", "Conventions", False), ("status", "Interface status", False), ("next", "Next", False)],
        body,
        nav_label="Overview",
    )


def getting_started() -> None:
    body = (
        '<div class="prose">'
        "<p>This guide takes you from a downloaded archive to a working engine inside a chess GUI. It assumes "
        "no prior UCI experience and no compiler. If you want to build Fafnir yourself, read the "
        '<a href="docs-build.html">build guide</a> instead.</p>'
        '<div class="callout callout--info"><span class="callout__icon">i</span><div>'
        '<span class="callout__title">What you need</span>'
        "A 64-bit Windows or Linux machine, a chess GUI that speaks UCI, and the Fafnir archive that matches "
        "your CPU. The neural network is already inside the binary.</div></div>"
        "<h2 id=\"choose\">1. Choose the right build</h2>"
        "<p>The archives differ only in the instruction set compiled into the engine. A build compiled for AVX2 "
        "will not start on a CPU without AVX2; the safe <code>x86-64</code> build runs on any x86-64 machine.</p>"
        + table(
            ["Build", "Requires", "Recommended when"],
            [
                ["<code>x86-64</code>", "SSE2", "You do not know the CPU, or you run a VM"],
                ["<code>sse4.2</code>", "SSE4.2 + POPCNT", "The CPU is from 2009 or later"],
                ["<code>avx2</code>", "AVX2 + BMI2 + POPCNT", "The CPU is Haswell (2013) / Excavator (2015) or later"],
            ],
        )
        + "<p>On Windows, <code>System Information</code> shows the processor model. On Linux, "
        "<code>lscpu</code> lists the flags. If <code>avx2</code> appears, the AVX2 archive is safe. If you are "
        "unsure, start with the safe build; you can always switch later.</p>"
        "<h2 id=\"extract\">2. Extract the archive</h2>"
        "<p>Unzip the Windows archive anywhere you can write, for example <code>C:\\Engines\\Fafnir</code>. Do not "
        "run the engine from inside the zip preview. On Linux, extract the tarball and keep the binary in a stable "
        "directory.</p>"
        + code_block(
            """tar -xzf Fafnir-x86-64-avx2-linux-x64.tar.gz
chmod +x Fafnir
./Fafnir""",
            "bash",
            "Linux extract",
        )
        + callout(
            "The binary is self-contained with respect to the network. Do not download or configure a separate "
            "<code>.nnue</code> file for the published archives.",
            "success",
            "No sidecar files",
            "✓",
        )
        + "<h2 id=\"gui\">3. Add the engine to a GUI</h2>"
        "<p>Any UCI-compatible GUI works. The exact menu names differ, but the flow is the same: add a new engine, "
        "point it at the binary, and let the GUI perform the UCI handshake. Common choices are Arena, Cute Chess, "
        "En Croissant and BanksiaGUI.</p>"
        '<ol class="steps">'
        "<li><h3>Open the engine manager</h3><p>Look for “Manage engines”, “Install engine” or a similar entry in the GUI settings.</p></li>"
        "<li><h3>Select the Fafnir binary</h3><p>Choose <code>Fafnir.exe</code> on Windows, or the extracted <code>Fafnir</code> binary on Linux. Do not select a source file or the archive.</p></li>"
        "<li><h3>Choose the UCI protocol</h3><p>If the GUI asks, select UCI (not WinBoard or a generic protocol). Fafnir does not implement CECP.</p></li>"
        "<li><h3>Confirm the identity</h3><p>The GUI should report <code>Fafnir 2</code> and list five options. If it reports another name, you selected a different binary.</p></li>"
        "</ol>"
        "<p>If the GUI offers a hash or thread setting, leave the defaults for the first run: 16 MB and one thread. "
        "You can raise Hash later without restarting the engine.</p>"
        "<h2 id=\"first\">4. Run the first UCI session</h2>"
        "<p>You can verify the engine without a GUI. Start it in a terminal and type the handshake manually. "
        "The engine reads one command per line and writes its replies to standard output.</p>"
        + code_block(
            """$ ./Fafnir
uci
id name Fafnir 2
id author Anthony
option name Hash type spin default 16 min 1 max 1024
option name Threads type spin default 1 min 1 max 128
option name MultiPV type spin default 1 min 1 max 256
option name Move Overhead type spin default 50 min 0 max 5000
option name Aggressivity type spin default 0 min 0 max 100
uciok
isready
readyok
position startpos
go depth 12
info depth 12 seldepth 20 score cp 31 nodes 48210 nps 402000 time 120 pv e2e4 e7e5 g1f3 b8c6
bestmove e2e4 ponder e7e5
quit""",
            "text",
            "Minimal session",
        )
        + callout(
            "The <code>id</code> and <code>option</code> lines are printed in response to <code>uci</code>. "
            "<code>uciok</code> closes the handshake. A GUI that never receives <code>uciok</code> will show the "
            "engine as unavailable.",
            "info",
            "Reading the handshake",
        )
        + "<h2 id=\"verify\">5. Verify the build</h2>"
        "<p>Three checks catch most installation problems:</p>"
        "<ul>"
        "<li>The engine prints <code>id name Fafnir 2</code> and exactly five options.</li>"
        "<li><code>isready</code> produces <code>readyok</code>.</li>"
        "<li>A short search produces <code>info</code> lines and a <code>bestmove</code>.</li>"
        "</ul>"
        "<p>If the process exits immediately on Windows, you probably selected an AVX2 archive on a CPU without "
        "AVX2. If it exits on Linux with <code>Illegal instruction</code>, the same applies. Switch to a lower ISA "
        "build.</p>"
        "<h2 id=\"next\">Next steps</h2>"
        "<ul>"
        '<li><a href="docs-uci.html">UCI reference</a> — every command, option and output field.</li>'
        '<li><a href="docs-build.html">Build from source</a> — compile for your exact CPU.</li>'
        '<li><a href="docs-troubleshooting.html">Troubleshooting</a> — if the GUI or engine misbehaves.</li>'
        '<li><a href="benchmarks.html">Benchmarks</a> — what the instruction sets change in practice.</li>'
        "</ul>"
        "</div>"
    )
    _doc(
        "docs-getting-started.html",
        "Getting started",
        "Download, extract and connect Fafnir to a UCI chess GUI, verify the engine identity and run the first search.",
        [("choose", "1. Choose a build", False), ("extract", "2. Extract", False), ("gui", "3. Add a GUI", False), ("first", "4. First session", False), ("verify", "5. Verify", False), ("next", "Next steps", False)],
        body,
    )


def build() -> None:
    body = (
        '<div class="prose">'
        "<p>Fafnir builds with a C++17 toolchain and GNU Make. The root <code>Makefile</code> handles native "
        "Windows and Linux builds; <code>Makefile.wasm</code> handles Emscripten. CI validates g++ on Linux and "
        "Windows; other compilers are accepted through <code>CXX=</code> but are not covered by CI.</p>"
        "<h2 id=\"requirements\">Requirements</h2>"
        + table(
            ["Tool", "Minimum", "Notes"],
            [
                ["C++ compiler", "C++17", "g++ is CI-validated. Clang and MSVC are accepted via <code>CXX=</code>."],
                ["Make", "GNU Make", "MSYS2 provides it on Windows."],
                ["Shell", "cmd.exe, MSYS2 sh, bash", "The Makefile is portable across them."],
                ["Emscripten", "emcc in <code>PATH</code>", "Only required for <code>Makefile.wasm</code>."],
                ["Python", "3.9+", "Only required for the EPD test runner."],
            ],
            cls="table--kv",
        )
        + callout(
            "The default target is <code>make</code>, which produces a binary safe on any x86-64 CPU (SSE2 only). "
            "It will not crash because of an unsupported instruction set, regardless of how the build machine is configured.",
            "success",
            "Safe default",
            "✓",
        )
        + "<h2 id=\"quick\">Quick start</h2>"
        + code_block(
            """git clone https://github.com/kagravashkelia-bit/Fafnir.git
cd Fafnir
make -j8
./bin/Fafnir""",
            "bash",
            "Clone and build",
        )
        + code_block(
            """make clean
make ARCH=native -j8
make ARCH=x86-64-avx2 -j8""",
            "bash",
            "Tuned and explicit builds",
        )
        + "<h2 id=\"targets\">Make targets</h2>"
        + table(
            ["Target", "Result"],
            [
                ["<code>make</code>", "Default <code>ARCH</code> (x86-64 safe), output <code>bin/Fafnir</code> or <code>bin/Fafnir.exe</code>"],
                ["<code>make ARCH=native</code>", "Tuned for the build machine (<code>-march=native</code>)"],
                ["<code>make all-arches</code>", "Builds every ISA in <code>bin/Fafnir-&lt;arch&gt;</code>"],
                ["<code>make clean</code>", "Removes <code>build/</code> and <code>bin/</code>"],
                ["<code>make rebuild</code>", "Clean followed by the default build"],
                ["<code>make run</code>", "Builds and starts the engine"],
                ["<code>make help</code>", "Prints the architecture matrix"],
            ],
        )
        + "<p><code>all-arches</code> builds <code>x86-64</code>, <code>x86-64-ssse3</code>, "
        "<code>x86-64-sse4.2</code>, <code>x86-64-avx2</code> and <code>x86-64-avx512</code>. Each architecture "
        "compiles into its own <code>build/&lt;ARCH&gt;/</code> directory, so flags are never mixed between binaries. "
        "The unsuffixed <code>bin/Fafnir</code> always reflects the last architecture built.</p>"
        "<h2 id=\"isa\">Architecture matrix</h2>"
        + table(
            ["ARCH", "Flags", "Baseline"],
            [
                ["<code>x86-64</code>", "<code>-msse2</code>", "Any x86-64"],
                ["<code>x86-64-ssse3</code>", "<code>-mssse3</code>", "Core 2 / K10 and later"],
                ["<code>x86-64-sse4.2</code>", "<code>-msse4.2 -mpopcnt</code>", "Nehalem / Bulldozer and later"],
                ["<code>x86-64-avx2</code>", "<code>-mavx2 -mbmi -mbmi2 -mpopcnt -mlzcnt</code>", "Haswell / Excavator and later"],
                ["<code>x86-64-avx512</code>", "<code>-mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vnni</code>", "Skylake-X / Ice Lake and later"],
                ["<code>native</code>", "<code>-march=native</code>", "This build machine only"],
            ],
        )
        + "<p>Choosing a target above your CPU's capability produces a binary that crashes with an illegal "
        "instruction. If you distribute a build, use the lowest ISA that covers your audience, or publish "
        "separate archives like the official release.</p>"
        "<h2 id=\"windows\">Windows</h2>"
        "<p>The CI workflow uses MSYS2 with g++. From an MSYS2 shell in the repository root:</p>"
        + code_block(
            """pacman -S --needed base-devel mingw-w64-x86_64-gcc make
make ARCH=x86-64-avx2 -j8
.\\bin\\Fafnir.exe""",
            "bash",
            "MSYS2 build",
        )
        + callout(
            "From <code>cmd.exe</code>, the Makefile uses the Windows <code>mkdir</code> and path conventions. "
            "It is idempotent: an existing directory no longer aborts the build. Do not mix cmd.exe and MSYS2 "
            "temporary files in the same build tree.",
            "info",
            "Shell portability",
        )
        + "<h2 id=\"linux\">Linux</h2>"
        + code_block(
            """sudo apt install build-essential
make ARCH=x86-64-sse4.2 -j"$(nproc)"
./bin/Fafnir""",
            "bash",
            "Debian / Ubuntu",
        )
        + "<p>The binary is produced directly; there is no install step. Copy it to a stable path and point your "
        "GUI at that path.</p>"
        "<h2 id=\"compiler\">Using another compiler</h2>"
        + code_block(
            """make CXX=clang++ ARCH=x86-64-avx2 -j8
make CXX=g++ ARCH=native -j8""",
            "bash",
            "Compiler override",
        )
        + callout(
            "Clang and MSVC are not validated by CI. They are accepted by the Makefile, but a successful build "
            "on your machine is not the same as the project's tested configuration. g++ 15.2.0 is the reference "
            "compiler for the published benchmarks.",
            "warning",
            "CI coverage is narrower than the Makefile",
        )
        + "<h2 id=\"wasm\">WebAssembly</h2>"
        + code_block(
            """make -f Makefile.wasm""",
            "bash",
            "Emscripten build",
        )
        + "<p>Requires <code>emcc</code> in <code>PATH</code>. Output goes to <code>build-wasm/Fafnir.js</code> and "
        "<code>build-wasm/Fafnir.wasm</code>. Network embedding is disabled with <code>NNUE_EMBEDDING_OFF</code>, "
        "so the WASM artifact does not carry the 94 MB network inside the binary. See "
        '<a href="docs-wasm.html">WebAssembly</a> for the C API and current status.</p>'
        "<h2 id=\"verify\">Verify the build</h2>"
        + code_block(
            """printf 'uci\\nisready\\nquit\\n' | ./bin/Fafnir | grep -E 'id name|uciok|readyok'""",
            "bash",
            "Smoke test",
        )
        + "<p>Expected output includes <code>id name Fafnir 2</code>, <code>uciok</code> and <code>readyok</code>. "
        "The same sequence is used by the <code>build-native.yml</code> CI workflow.</p>"
        "<h2 id=\"trouble\">Build troubleshooting</h2>"
        + table(
            ["Symptom", "Likely cause", "Fix"],
            [
                ["<code>make: command not found</code>", "Make is not installed or not in PATH", "Install build-essential / MSYS2 make"],
                ["<code>g++: not found</code>", "Compiler missing", "Install g++ and reopen the shell"],
                ["<code>mkdir</code> aborts", "Old build tree or shell mismatch", "Run <code>make clean</code>, then rebuild"],
                ["Illegal instruction at runtime", "ISA mismatch", "Rebuild for a lower <code>ARCH</code>"],
                ["<code>emcc</code> not found", "Emscripten not activated", "Source <code>emsdk_env</code> or add emcc to PATH"],
                ["Linker out of memory", "Too many parallel jobs", "Lower <code>-j</code>"],
            ],
            cls="table--kv",
        )
        + "</div>"
    )
    _doc(
        "docs-build.html",
        "Build from source",
        "Build Fafnir on Windows, Linux or Emscripten: requirements, make targets, ISA flags, compiler overrides and verification.",
        [("requirements", "Requirements", False), ("quick", "Quick start", False), ("targets", "Make targets", False), ("isa", "Architecture matrix", False), ("windows", "Windows", False), ("linux", "Linux", False), ("compiler", "Other compilers", False), ("wasm", "WebAssembly", False), ("verify", "Verify", False), ("trouble", "Troubleshooting", False)],
        body,
    )


def uci_reference() -> None:
    body = (
        '<div class="prose">'
        "<p>Fafnir implements the Universal Chess Interface. Commands are read line by line from standard input; "
        "replies are written to standard output. The engine does not use standard error for protocol output.</p>"
        "<h2 id=\"identity\">Identity and handshake</h2>"
        "<p><code>uci</code> prints the engine identity and the complete option list, then <code>uciok</code>. "
        "This is the only supported discovery mechanism. The current identity is:</p>"
        + code_block(
            """id name Fafnir 2
id author Anthony""",
            "text",
            "Identity strings",
        )
        + table(
            ["Command", "Behaviour"],
            [
                ["<code>uci</code>", "Prints <code>id</code>, every option and <code>uciok</code>"],
                ["<code>isready</code>", "Prints <code>readyok</code> when the engine can accept commands"],
                ["<code>ucinewgame</code>", "Resets the search state for a new game"],
                ["<code>position</code>", "Sets the board from <code>startpos</code> or <code>fen</code>, optionally followed by <code>moves</code>"],
                ["<code>go</code>", "Starts a search with the supplied limits"],
                ["<code>stop</code>", "Stops the current search as soon as possible"],
                ["<code>setoption</code>", "Changes a declared option"],
                ["<code>quit</code>", "Terminates the engine"],
            ],
        )
        + "<h2 id=\"position\">Position setup</h2>"
        "<p>Two forms are supported. The move list is applied after the base position, in UCI coordinate notation.</p>"
        + code_block(
            """position startpos
position startpos moves e2e4 e7e5 g1f3
position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
position fen 8/8/8/8/8/8/8/K6k w - - 0 1 moves a1a2""",
            "text",
            "position",
        )
        + callout(
            "The engine expects a complete, legal FEN. EPD test positions often omit the halfmove and fullmove "
            "counters; the Python test runner adds neutral counters before sending them. If you build a FEN by "
            "hand, include all six fields.",
            "warning",
            "FEN completeness",
        )
        + "<h2 id=\"go\">Search limits</h2>"
        "<p><code>go</code> accepts the following parameters. They can be combined; the engine stops at the first "
        "limit that is reached.</p>"
        + table(
            ["Parameter", "Meaning"],
            [
                ["<code>depth &lt;n&gt;</code>", "Stop after completing iteration <code>n</code>"],
                ["<code>movetime &lt;ms&gt;</code>", "Search for approximately <code>ms</code> milliseconds"],
                ["<code>wtime &lt;ms&gt;</code>", "White's remaining clock"],
                ["<code>btime &lt;ms&gt;</code>", "Black's remaining clock"],
                ["<code>winc &lt;ms&gt;</code>", "White's increment"],
                ["<code>binc &lt;ms&gt;</code>", "Black's increment"],
                ["<code>movestogo &lt;n&gt;</code>", "Moves remaining before the next time control"],
                ["<code>ponder</code>", "Start in ponder mode"],
                ["<code>infinite</code>", "Search until <code>stop</code>"],
            ],
        )
        + code_block(
            """go depth 18
go movetime 3000
go wtime 300000 btime 300000 winc 2000 binc 2000
go wtime 60000 btime 60000 movestogo 20
go infinite""",
            "text",
            "go examples",
        )
        + "<h2 id=\"options\">Options</h2>"
        "<p>The engine declares exactly five options. The ranges and defaults below are the values printed by "
        "<code>uci</code>, not approximations.</p>"
        + table(
            ["Option", "Type", "Default", "Min", "Max", "Implementation"],
            [
                ["<code>Hash</code>", "spin", "16", "1", "1024", "Resizable transposition table, applied immediately"],
                ["<code>Threads</code>", "spin", "1", "1", "128", "Lazy SMP helper threads"],
                ["<code>MultiPV</code>", "spin", "1", "1", "256", "Root search by variation"],
                ["<code>Move Overhead</code>", "spin", "50", "0", "5000", "Margin subtracted from the clock"],
                ["<code>Aggressivity</code>", "spin", "0", "0", "100", "Evaluation optimism during search"],
            ],
        )
        + code_block(
            """setoption name Hash value 128
setoption name Threads value 4
setoption name MultiPV value 3
setoption name Move Overhead value 100
setoption name Aggressivity value 25""",
            "text",
            "setoption",
        )
        + callout(
            "An unrecognised option is not silently ignored. Fafnir answers "
            "<code>info string unknown UCI option</code>, so a typo is visible in the GUI log. Only the five "
            "options above are valid in 2.0.",
            "danger",
            "Unknown options are rejected",
        )
        + "<h2 id=\"info\">Output during search</h2>"
        "<p>While searching, the engine emits <code>info</code> lines. The fields below are the ones Fafnir "
        "produces; a GUI must not assume that optional fields such as <code>hashfull</code> or <code>tbhits</code> "
        "are present.</p>"
        + table(
            ["Field", "Meaning"],
            [
                ["<code>depth</code>", "Completed search depth"],
                ["<code>seldepth</code>", "Maximum selective depth reached"],
                ["<code>score cp &lt;n&gt;</code>", "Score in centipawns from the side to move"],
                ["<code>score mate &lt;n&gt;</code>", "Mate distance in moves"],
                ["<code>nodes</code>", "Nodes searched in this iteration"],
                ["<code>nps</code>", "Nodes per second"],
                ["<code>time</code>", "Elapsed search time in milliseconds"],
                ["<code>pv</code>", "Principal variation in UCI coordinate notation"],
            ],
        )
        + code_block(
            """info depth 20 seldepth 34 score cp 35 nodes 553530 nps 464370 time 1192 pv e2e4 e7e5 g1f3 b8c6
bestmove e2e4 ponder e7e5""",
            "text",
            "info and bestmove",
        )
        + "<p><code>bestmove</code> is always the final line of a search. The <code>ponder</code> field is a hint "
        "for the GUI; it is not part of the move that must be played.</p>"
        "<h2 id=\"multi\">MultiPV output</h2>"
        "<p>With <code>MultiPV</code> greater than one, the engine reports one <code>info</code> line per root "
        "variation. The <code>multipv</code> field identifies the rank. The final <code>bestmove</code> still "
        "corresponds to the first variation.</p>"
        + code_block(
            """setoption name MultiPV value 3
go depth 12
info depth 12 seldepth 18 multipv 1 score cp 34 ... pv e2e4 ...
info depth 12 seldepth 18 multipv 2 score cp 28 ... pv d2d4 ...
info depth 12 seldepth 18 multipv 3 score cp 21 ... pv g1f3 ...
bestmove e2e4 ponder e7e5""",
            "text",
            "MultiPV",
        )
        + "<h2 id=\"errors\">Error behaviour</h2>"
        + table(
            ["Input", "Response"],
            [
                ["Unknown option", "<code>info string unknown UCI option</code>"],
                ["Unknown command", "No protocol reply; the command is ignored"],
                ["Unknown <code>go</code> parameter", "Ignored; the search uses the recognised limits"],
                ["Empty position", "The previous position remains in effect"],
                ["<code>stop</code> with no search", "No reply"],
            ],
        )
        + callout(
            "Fafnir does not invent a <code>bestmove 0000</code> response for a normal search. "
            "<code>0000</code> is used by the WASM API when no move can be produced.",
            "info",
            "Protocol honesty",
        )
        + "<h2 id=\"session\">Complete session</h2>"
        + code_block(
            """uci
isready
ucinewgame
setoption name Hash value 64
position startpos moves e2e4 c7c5
go movetime 1000
# ... info lines ...
bestmove g1f3 ponder d7d6
quit""",
            "text",
            "Session flow",
        )
        + "<h2 id=\"gui\">GUI integration notes</h2>"
        "<ul>"
        "<li>Wait for <code>uciok</code> before sending <code>isready</code>.</li>"
        "<li>Send <code>ucinewgame</code> between games if the GUI supports it; it clears stale search state.</li>"
        "<li>Set <code>Move Overhead</code> to a value that covers your GUI's latency. The default is 50 ms.</li>"
        "<li>Do not assume a tablebase or book option exists; it does not in 2.0.</li>"
        "<li>Log the raw engine output when reporting a protocol bug. The exact <code>info</code> sequence matters.</li>"
        "</ul>"
        "</div>"
    )
    _doc(
        "docs-uci.html",
        "UCI reference",
        "The complete Fafnir UCI contract: handshake, position, go limits, five options, info fields, MultiPV and error behaviour.",
        [("identity", "Identity", False), ("position", "Position", False), ("go", "Search limits", False), ("options", "Options", False), ("info", "Search output", False), ("multi", "MultiPV", False), ("errors", "Errors", False), ("session", "Complete session", False), ("gui", "GUI notes", False)],
        body,
    )


def search_internals() -> None:
    stages = table(
        ["Stage", "Contents", "Ordering"],
        [
            ["TT move", "Hash move from the transposition table", "Exact match"],
            ["Good captures", "Captures that pass SEE", "MVV-LVA, then SEE"],
            ["Quiets", "All remaining quiet moves", "Main / continuation history"],
            ["Bad captures", "Captures that lose material by SEE", "MVV-LVA"],
            ["Bad quiets", "Deferred quiet moves", "History"],
            ["Evasions", "Legal replies to check", "TT, captures, history"],
            ["ProbCut", "Tactical subset at reduced depth", "SEE and capture gain"],
            ["Qsearch", "Captures and promotions at the horizon", "MVV-LVA / SEE"],
        ],
    )
    body = (
        '<div class="prose">'
        "<p>The search lives in <code>src/engine/search.hpp</code>, with the move picker in "
        "<code>movepicker.cpp</code> and the thread controller in <code>SearchController.cpp</code>. The "
        "implementation follows the Stockfish development style; the constants and code are part of this "
        "repository, not a dropped-in binary.</p>"
        "<h2 id=\"pipeline\">Search pipeline</h2>"
        "<ol>"
        "<li><strong>Root setup.</strong> The controller copies the position, clears per-search state, and starts the requested number of threads.</li>"
        "<li><strong>Iterative deepening.</strong> Depth increases one ply at a time until a limit is reached.</li>"
        "<li><strong>Move generation.</strong> The MovePicker yields moves in stages, cheapest and most promising first.</li>"
        "<li><strong>Pruning and reduction.</strong> Futility, null move, ProbCut, LMR and singular extensions decide how much work each node deserves.</li>"
        "<li><strong>Evaluation.</strong> The NNUE probe returns a quantized score for the leaf.</li>"
        "<li><strong>Backup.</strong> Scores and principal variations are stored in the stack and the transposition table.</li>"
        "</ol>"
        "<h2 id=\"tt\">Transposition table</h2>"
        "<p>The table is divided into clusters of three entries. Each entry stores a key fragment, a move, a "
        "score, the search depth and generation metadata. Replacement is age-based: an entry from an older "
        "search is evicted before a current-generation entry of similar depth.</p>"
        "<p><code>Hash</code> is exposed as a UCI spin option from 1 to 1024 MB, default 16. The implementation "
        "resizes the table immediately, so a GUI can change it between games without restarting the process.</p>"
        "<h2 id=\"history\">History heuristics</h2>"
        "<p>Fafnir maintains the full family used by the reference search style:</p>"
        + table(
            ["History", "Purpose"],
            [
                ["Main history", "Quiet-move success by side, from, to and piece"],
                ["Low-ply history", "Extra weight for quiet moves near the root"],
                ["Capture history", "Capture success by piece and target"],
                ["Continuation history (2×2)", "Success conditioned on the previous two moves"],
                ["Pawn history", "Quiet-move success keyed by pawn structure"],
                ["Correction history", "Corrects static evaluation bias by pawn / minor / non-pawn / continuation context"],
            ],
        )
        + callout(
            "Correction history is not a tie-breaker. It adjusts the static evaluation before search uses it, "
            "which is why the four tables (pawn, minor, non-pawn, continuation) are updated separately.",
            "info",
            "Correction history",
        )
        + "<h2 id=\"movepicker\">MovePicker stages</h2>"
        + stages
        + "<p>The picker is stateful: each call to the next-move function advances through the stages and "
        "returns the next candidate without regenerating the full move list.</p>"
        "<h2 id=\"pruning\">Pruning and reductions</h2>"
        + table(
            ["Technique", "Role"],
            [
                ["Razoring", "Drop clearly hopeless nodes before full search"],
                ["Futility pruning", "Skip quiet moves when the static score is far below alpha"],
                ["Null-move pruning", "Search a pass position to prove a beta cutoff, with verification"],
                ["IIR", "Reduce the depth when no TT move is available"],
                ["ProbCut", "Probe a reduced-depth tactical search for a likely cutoff"],
                ["Singular extension", "Extend a move when it appears to be the only good reply"],
                ["Multicut", "Cut when several moves fail high in a reduced search"],
                ["Negative extensions", "Reduce moves that repeatedly fail to improve"],
                ["LMR", "Reduce late quiet moves, with shallow re-search when they beat alpha"],
            ],
        )
        + "<p>LMR at non-PV nodes uses both quiet history and futility to decide the reduction. A reduced move that "
        "returns a score above alpha is re-searched at full depth before it can change the principal variation.</p>"
        "<h2 id=\"draws\">Draw and repetition detection</h2>"
        "<p>SEE is computed with pinned pieces taken into account, so a capture that looks winning but exposes "
        "the king is not treated as a good capture. Cuckoo keys detect imminent repetition without walking the "
        "entire history. Repetition and the fifty-move rule are checked in the search stack before a node is "
        "searched.</p>"
        "<h2 id=\"time\">Time management</h2>"
        "<p>The controller translates UCI limits into a soft and a hard deadline. <code>movetime</code> sets both "
        "to the same budget; clock-based searches derive an allocation from <code>wtime</code>/<code>btime</code>, "
        "increments and <code>movestogo</code>. <code>Move Overhead</code> is subtracted as a safety margin. "
        "<code>ponder</code> starts a search that can be stopped by the GUI.</p>"
        "<h2 id=\"aggressivity\">Aggressivity</h2>"
        "<p><code>Aggressivity</code> ranges from 0 to 100 and modulates the optimism applied to evaluation "
        "during search. At 0 the engine uses its default evaluation; higher values bias the search toward "
        "positions with more apparent activity. It is a search-time adjustment, not a separate evaluation "
        "function, and it does not change the network.</p>"
        "<h2 id=\"limits\">Structural limits</h2>"
        + table(
            ["Constant", "Value", "Meaning"],
            [
                ["<code>MAX_PLY</code>", "246", "Maximum search stack depth"],
                ["<code>Hash</code>", "1–1024 MB", "Transposition table size"],
                ["<code>Threads</code>", "1–128", "Lazy SMP helper threads"],
                ["<code>MultiPV</code>", "1–256", "Root variations"],
            ],
            cls="table--kv",
        )
        + "<h2 id=\"map\">Code map</h2>"
        + table(
            ["File", "Responsibility"],
            [
                ["<code>src/engine/search.hpp</code>", "Search, pruning, reductions, history updates"],
                ["<code>src/engine/movepicker.cpp</code>", "Staged move generation and ordering"],
                ["<code>src/engine/SearchController.cpp</code>", "Threads, time limits, search lifecycle"],
                ["<code>src/engine/eval/evaluate.hpp</code>", "Evaluation entry point"],
                ["<code>src/bridges/nnue_probe.cpp</code>", "Bridge to the vendored SFNNv17 network"],
            ],
        )
        + callout(
            "Changing a pruning constant or a history weight changes playing strength, not just speed. Run the "
            '<a href="docs-testing.html">EPD suite</a> before and after the change and keep both results.',
            "warning",
            "Tuning is a regression risk",
        )
        + "</div>"
    )
    _doc(
        "docs-search.html",
        "Search internals",
        "How Fafnir searches: transposition table, six history families, MovePicker stages, pruning, LMR, time management and Aggressivity.",
        [("pipeline", "Pipeline", False), ("tt", "Transposition table", False), ("history", "History", False), ("movepicker", "MovePicker", False), ("pruning", "Pruning", False), ("draws", "Draw detection", False), ("time", "Time management", False), ("aggressivity", "Aggressivity", False), ("limits", "Limits", False), ("map", "Code map", False)],
        body,
    )


def nnue() -> None:
    body = (
        '<div class="prose">'
        "<p>Fafnir uses the Stockfish development SFNNv17 evaluation architecture with a single-output network "
        "and no PSQT. The network is loaded at link time with <code>incbin</code>; the native engine needs no "
        "external evaluation file.</p>"
        "<h2 id=\"facts\">Network facts</h2>"
        + table(
            ["Property", "Value"],
            [
                ["Architecture", "SFNNv17, single output, no PSQT"],
                ["Upstream snapshot", "Stockfish development commit <code>49ea5ded38315cff8e67f4a677a9e7811612fbf6</code>"],
                ["Default network", "<code>assets/nnue/nn-252f33942263.nnue</code>"],
                ["Size", "98,668,604 bytes"],
                ["SHA-256", "<code>252f33942263bc8b8f740ba8aec3fed5a159ff148113c47a55c18c33d6627ab3</code>"],
                ["Feature sets", "<code>half_ka_v2_hm</code>, <code>full_threats</code>, <code>pp_3wide</code>"],
                ["Loading", "<code>incbin</code> at compile time"],
                ["Normalization", "<code>NORMALIZE_TO_PAWN_VALUE = 356</code>"],
            ],
            cls="table--kv",
        )
        + "<h2 id=\"features\">Feature sets</h2>"
        "<p>The probe compiles only the subset of the vendored evaluator that the SFNNv17 network requires. "
        "Three feature families are active:</p>"
        "<ul>"
        "<li><strong><code>half_ka_v2_hm</code></strong> — half-king, attacker and piece features with horizontal mirroring.</li>"
        "<li><strong><code>full_threats</code></strong> — threats against pieces and pawns, including discovered and overloaded cases.</li>"
        "<li><strong><code>pp_3wide</code></strong> — a three-wide pawn-pattern feature set.</li>"
        "</ul>"
        "<p>Other Stockfish feature sets present in the vendored tree are not compiled into Fafnir. Keeping the "
        "compiled surface small reduces build time and makes the binary easier to audit.</p>"
        "<h2 id=\"embedding\">Embedding</h2>"
        "<p>The network sits at <code>assets/nnue/nn-252f33942263.nnue</code> and is included by the native build "
        "with <code>incbin</code>. The file is a normal Git blob (not an LFS pointer), so a clone contains the "
        "actual bytes and CI embeds the real network without a smudge step.</p>"
        + code_block(
            """# native build: network is inside the binary
./bin/Fafnir
# no external .nnue file is required

# WASM build: embedding disabled
make -f Makefile.wasm
# compiled with NNUE_EMBEDDING_OFF""",
            "bash",
            "Embedding behaviour",
        )
        + callout(
            "The WASM build deliberately disables embedding. A browser artifact carrying a 94 MB network would "
            "be impractical; the web edition plan uses a separately served network or a reduced artifact. "
            "The public web edition is still in preparation.",
            "warning",
            "WASM is different",
        )
        + "<h2 id=\"eval\">Evaluation and units</h2>"
        "<p>The probe returns a quantized score that the search consumes directly. For display, the value is "
        "normalized to pawn units using <code>NORMALIZE_TO_PAWN_VALUE = 356</code>, matching the convention of "
        "Stockfish 17.x. UCI <code>score cp</code> values are centipawns from the side to move.</p>"
        + code_block(
            """info depth 20 seldepth 34 score cp 35 nodes 553530 nps 464370 time 1192 pv e2e4 ...""",
            "text",
            "Score in context",
        )
        + "<p>A positive <code>cp</code> score favours the side to move. The GUI is responsible for rendering it "
        "from White's perspective if that is its convention.</p>"
        "<h2 id=\"replace\">Replacing the network</h2>"
        "<p>Replacing the default network is a source-level change, not a runtime option. The file name is "
        "referenced by <code>EvalFileDefaultName</code> in the vendored <code>evaluate.h</code> (the single local "
        "patch to the snapshot), and the binary embeds whatever file that name resolves to at build time.</p>"
        + callout(
            "Do not replace the network with an incompatible architecture. SFNNv17 expects the feature sets and "
            "layer layout of the same generation. A mismatched file will fail to load or produce meaningless "
            "scores. Rebuild and re-run the EPD suite after any change.",
            "danger",
            "Architecture compatibility",
        )
        + "<h2 id=\"provenance\">Provenance and license</h2>"
        "<p>The network is the default network of the vendored SFNNv17 snapshot and is redistributed under the "
        "upstream license. The engine as a whole is GPLv3 because it links GPLv3 Stockfish code. See the "
        '<a href="license.html">license page</a> and <code>third_party/VENDORED.md</code> for the complete '
        "breakdown.</p>"
        "<h2 id=\"verify\">Verify the network</h2>"
        + code_block(
            """# Linux
sha256sum assets/nnue/nn-252f33942263.nnue
# expected:
252f33942263bc8b8f740ba8aec3fed5a159ff148113c47a55c18c33d6627ab3

# Windows PowerShell
Get-FileHash .\\assets\\nnue\\nn-252f33942263.nnue -Algorithm SHA256""",
            "bash",
            "Checksum",
        )
        + "<h2 id=\"map\">Code map</h2>"
        + table(
            ["File", "Responsibility"],
            [
                ["<code>src/bridges/nnue_probe.cpp</code>", "Initializes the network and exposes evaluation to the engine"],
                ["<code>src/bridges/nnue_probe.h</code>", "Probe interface"],
                ["<code>third_party/stockfish/sfdev/src/nnue/</code>", "Vendored SFNNv17 implementation"],
                ["<code>third_party/stockfish/sfdev/src/evaluate.h</code>", "Local patch: network path"],
                ["<code>assets/nnue/nn-252f33942263.nnue</code>", "Embedded network bytes"],
            ],
        )
        + "</div>"
    )
    _doc(
        "docs-nnue.html",
        "NNUE evaluation",
        "Fafnir's SFNNv17 network: feature sets, embedding with incbin, quantization, hash verification, provenance and replacement rules.",
        [("facts", "Network facts", False), ("features", "Feature sets", False), ("embedding", "Embedding", False), ("eval", "Evaluation units", False), ("replace", "Replacing", False), ("provenance", "Provenance", False), ("verify", "Verify", False), ("map", "Code map", False)],
        body,
    )


def testing() -> None:
    body = (
        '<div class="prose">'
        "<p>The regression suite is a 100-position EPD test set with a deliberately strict rule: a position is a "
        "hit only when the engine's UCI <code>bestmove</code> matches one of the EPD <code>bm</code> moves. "
        "There is no partial credit and no “near miss” category.</p>"
        "<h2 id=\"suite\">The suite</h2>"
        + table(
            ["Source", "Positions", "Focus"],
            [
                ["Bratko-Kopec", "24", "Tactical calculation and positional tactics"],
                ["Kaufman", "19", "Opening and strategic accuracy (strict bm positions)"],
                ["Nolot", "11", "Sharp tactical positions"],
                ["LCT-II", "14 positional + 12 combination", "Long-term positional and tactical tests"],
                ["Rybka Forum", "20", "Endgame and conversion tests"],
            ],
            cls="table--kv",
        )
        + "<p>The runner converts EPD SAN best moves to UCI internally, so the test file stays readable. It requires "
        "Python 3.9+ and no third-party packages.</p>"
        "<h2 id=\"run\">Running the suite</h2>"
        + code_block(
            """python tests/tests.py bin/Fafnir.exe --depth 18
python tests/tests.py bin/Fafnir --movetime 1000
python tests/tests.py bin/Fafnir --depth 20 --hash 128 --threads 1
python tests/tests.py bin/Fafnir --depth 18 --category FINALES_RYBKA""",
            "bash",
            "Commands",
        )
        + table(
            ["Flag", "Meaning"],
            [
                ["<code>--depth &lt;n&gt;</code>", "Search each position to a fixed depth"],
                ["<code>--movetime &lt;ms&gt;</code>", "Use a fixed time per position instead of depth"],
                ["<code>--hash &lt;mb&gt;</code>", "Set Hash before the run"],
                ["<code>--threads &lt;n&gt;</code>", "Set Threads before the run"],
                ["<code>--category &lt;name&gt;</code>", "Restrict to one EPD category"],
            ],
        )
        + "<p>Pick either depth or movetime. A depth run is more reproducible; a movetime run reflects tournament "
        "conditions but depends on machine load.</p>"
        "<h2 id=\"interpret\">Interpreting the result</h2>"
        + table(
            ["Result", "Meaning"],
            [
                ["HIT", "The engine's bestmove is one of the EPD best moves"],
                ["MISS", "The engine chose a different legal move"],
                ["ERROR", "The engine failed to produce a valid bestmove"],
                ["Timeout", "No bestmove before the runner's deadline"],
            ],
        )
        + callout(
            "A single run is not a rating. The suite is a regression gate: compare the same binary, depth and "
            "Hash across changes. A one-position difference can be noise; a pattern across a category is a signal.",
            "info",
            "How to read the score",
        )
        + "<h2 id=\"records\">Control games</h2>"
        "<p><code>tests/records/</code> contains PGN archives of control games, including draws against Stockfish "
        "and Reckless 0.9.0. These are historical records, not a claim of current strength. They are useful for "
        "replaying a position or checking how a configuration behaved in a real game.</p>"
        + code_block(
            """tests/records/
  FlashBit 1.3_vs_Stockfish_2026.10.17.pgn
  FlashBit 1.3_vs_Reckless 0.9.0_2026.10.21.pgn
  lichess_pgn_2026.10.24_Fafnir_vs_Reckless_0.9.0.4Z8a5nTu.pgn""",
            "text",
            "Archived records",
        )
        + "<h2 id=\"ci\">CI smoke test</h2>"
        "<p><code>build-native.yml</code> does not run the full EPD suite. It performs a UCI smoke test after "
        "building each native archive, which catches a binary that cannot start or answer the handshake. The "
        "EPD suite is run locally when changing search or evaluation behaviour.</p>"
        + code_block(
            """uci
isready
quit""",
            "text",
            "CI smoke sequence",
        )
        + "<h2 id=\"add\">Adding a position</h2>"
        "<ol>"
        "<li>Add a <code>TestCase</code> entry in <code>tests/tests.py</code> with a name, category, FEN, best moves and source.</li>"
        "<li>Include all six FEN fields if possible; the runner can add neutral counters, but explicit is better.</li>"
        "<li>Verify the position is legal and that the best move is expressed in SAN.</li>"
        "<li>Run the suite before and after the change you are testing.</li>"
        "</ol>"
        "<h2 id=\"limits\">Limitations</h2>"
        "<ul>"
        "<li>Best-move matching ignores evaluation quality: a move that is objectively equal but not listed counts as a miss.</li>"
        "<li>The suite is small compared with a full game-pair match; it detects regressions, not absolute strength.</li>"
        "<li>Time-based runs vary with hardware and background load.</li>"
        "<li>There is no built-in <code>bench</code> command; use the documented UCI sequence for speed measurements.</li>"
        "</ul>"
        "</div>"
    )
    _doc(
        "docs-testing.html",
        "Testing",
        "Fafnir's 100-position EPD regression suite: categories, strict best-move rule, commands, interpretation, control games and CI smoke tests.",
        [("suite", "The suite", False), ("run", "Running", False), ("interpret", "Interpreting", False), ("records", "Control games", False), ("ci", "CI smoke test", False), ("add", "Adding positions", False), ("limits", "Limitations", False)],
        body,
    )


def architecture() -> None:
    body = (
        '<div class="prose">'
        "<p>The repository is intentionally small at the top level. Most of the code is either the in-house "
        "engine or a vendored dependency with a documented boundary. This page maps the responsibilities.</p>"
        "<h2 id=\"tree\">Repository map</h2>"
        + code_block(
            """Fafnir/
├─ src/
│  ├─ ui/            main.cpp · uci.cpp · uci.hpp · wasm_api.cpp · fafnir_wasm_api.h
│  ├─ engine/        search.hpp · movepicker.cpp · SearchController.cpp
│  │  └─ eval/       evaluate.hpp · pst.hpp
│  ├─ bridges/       nnue_probe.cpp · nnue_probe.h
│  └─ core/          types.hpp
├─ assets/
│  ├─ branding/      Fafnir-logo.png
│  └─ nnue/          nn-252f33942263.nnue
├─ third_party/
│  ├─ chesslib/      chesslib.hpp (MIT)
│  ├─ stockfish/     sfdev snapshot + legacy probe (not compiled)
│  └─ incbin/        incbin.h (Unlicense)
├─ tests/            tests.py · records/
├─ web/              this site
├─ Makefile          native multi-architecture builds
├─ Makefile.wasm     Emscripten build
└─ .github/          workflows""",
            "text",
            "Tree",
        )
        + "<h2 id=\"layers\">Layers</h2>"
        + table(
            ["Layer", "Files", "Responsibility"],
            [
                ["Protocol", "<code>src/ui/uci.cpp</code>, <code>uci.hpp</code>", "Parse UCI, own the board, translate options and search limits"],
                ["Entry point", "<code>src/ui/main.cpp</code>", "Construct the UCI engine and run the console loop"],
                ["Search", "<code>src/engine/search.hpp</code>", "Iterative deepening, pruning, reductions, history, PV"],
                ["Ordering", "<code>src/engine/movepicker.cpp</code>", "Staged move generation and ordering"],
                ["Control", "<code>src/engine/SearchController.cpp</code>", "Threads, time limits, lifecycle, stop"],
                ["Evaluation", "<code>src/engine/eval/</code>", "Evaluation entry point and piece-square tables"],
                ["NNUE bridge", "<code>src/bridges/nnue_probe.cpp</code>", "Initialize and call the vendored SFNNv17 network"],
                ["WASM API", "<code>src/ui/wasm_api.cpp</code>", "Five C functions exposed to JavaScript"],
                ["Core types", "<code>src/core/types.hpp</code>", "Shared enums and small types"],
            ],
        )
        + "<h2 id=\"flow\">Data flow</h2>"
        "<ol>"
        "<li><code>main.cpp</code> starts <code>UciEngine::main_loop()</code>.</li>"
        "<li><code>uci.cpp</code> parses a command and updates the board or options.</li>"
        "<li>A <code>go</code> command calls <code>SearchController</code>, which launches the requested threads.</li>"
        "<li><code>search.hpp</code> walks the tree, using <code>MovePicker</code> and the NNUE probe.</li>"
        "<li>Info lines and the final <code>bestmove</code> are written back to standard output.</li>"
        "<li><code>stop</code> or a time limit asks the controller to finish the current iteration.</li>"
        "</ol>"
        "<h2 id=\"threads\">Threading model</h2>"
        "<p><code>Threads</code> controls Lazy SMP helper threads. The main thread owns the search stack and "
        "the reported principal variation; helpers search the same root with different move ordering and share "
        "the transposition table. The controller owns the stop flag and the time checks, so a GUI <code>stop</code> "
        "does not need to interrupt a helper directly.</p>"
        + callout(
            "The shared transposition table is the only large cross-thread structure. Hash resizing must happen "
            "between searches, which is why the UCI layer applies <code>setoption name Hash</code> while the engine "
            "is idle.",
            "info",
            "Concurrency boundary",
        )
        + "<h2 id=\"vendored\">Vendored code boundary</h2>"
        + table(
            ["Dependency", "License", "Compiled?", "Notes"],
            [
                ["ChessLib 0.9.4", "MIT", "Yes", "Board, move generation, FEN/UCI helpers"],
                ["Stockfish SFNNv17 snapshot", "GPLv3", "Subset", "Only the NNUE code required by the probe"],
                ["Legacy Stockfish probe", "GPLv3", "No", "Not referenced by any build"],
                ["incbin", "Unlicense", "Yes", "Embeds the network in native builds"],
                ["NNUE network", "Upstream license", "Data", "98,668,604 bytes, SHA-256 published"],
            ],
        )
        + "<p>The only local patch to the vendored snapshot changes <code>EvalFileDefaultName</code> in "
        "<code>evaluate.h</code> to point at <code>assets/nnue/nn-252f33942263.nnue</code>. When the snapshot is "
        "renewed, that patch must be reapplied.</p>"
        "<h2 id=\"build\">Build graph</h2>"
        "<p>Each architecture compiles into <code>build/&lt;ARCH&gt;/</code>. The final link combines the UI, "
        "engine, probe and the required vendored Stockfish sources. There is no runtime plugin loading: the "
        "network and all engine code are in the binary.</p>"
        + code_block(
            """make ARCH=x86-64-avx2 -j8
# build/x86-64-avx2/src/...      object files
# build/x86-64-avx2/third_party/ vendored objects
# bin/Fafnir                     final binary""",
            "bash",
            "Build layout",
        )
        + "<h2 id=\"decisions\">Design decisions</h2>"
        "<ul>"
        "<li><strong>Safe default.</strong> <code>make</code> uses SSE2 so a distributable binary does not crash on older CPUs.</li>"
        "<li><strong>No runtime network path.</strong> Embedding removes a class of deployment errors.</li>"
        "<li><strong>No fake options.</strong> The protocol surface is exactly the implemented surface.</li>"
        "<li><strong>Remove unbuildable code.</strong> The JNI bridge was deleted rather than shipped broken.</li>"
        "<li><strong>One source of truth for the site.</strong> The website is generated from the same repository that ships the engine.</li>"
        "</ul>"
        "<h2 id=\"reading\">Where to start reading</h2>"
        "<p>For the protocol, read <code>src/ui/uci.cpp</code>. For move choice, read "
        "<code>src/engine/search.hpp</code> alongside <code>movepicker.cpp</code>. For evaluation, read "
        "<code>src/bridges/nnue_probe.cpp</code>, then the vendored <code>nnue/</code> directory. The build "
        "files are short enough to read end to end.</p>"
        "</div>"
    )
    _doc(
        "docs-architecture.html",
        "Architecture",
        "Fafnir repository map, layer responsibilities, search data flow, Lazy SMP threading, vendored code boundaries and build graph.",
        [("tree", "Repository map", False), ("layers", "Layers", False), ("flow", "Data flow", False), ("threads", "Threading", False), ("vendored", "Vendored code", False), ("build", "Build graph", False), ("decisions", "Design decisions", False), ("reading", "Reading order", False)],
        body,
    )


def wasm() -> None:
    body = (
        '<div class="prose">'
        + callout(
            "The public WebAssembly edition is <strong>in preparation</strong>. CI builds an artifact named "
            "<code>Fafnir-WASM-Build</code> on every push, and the C API below is implemented, but there is no "
            "supported browser release yet. Treat this page as an API reference for review and testing.",
            "warning",
            "Status: in preparation",
            "!",
        )
        + "<h2 id=\"build\">Building</h2>"
        "<p>The WASM build uses Emscripten and a separate makefile. It does not require the native toolchain.</p>"
        + code_block(
            """# requires emcc in PATH
make -f Makefile.wasm

# output
build-wasm/Fafnir.js
build-wasm/Fafnir.wasm""",
            "bash",
            "Emscripten build",
        )
        + "<p>Network embedding is disabled for WASM with <code>NNUE_EMBEDDING_OFF</code>. The artifact therefore "
        "does not contain the 94 MB network inside the wasm binary. The public edition will need a deliberate "
        "strategy for the network (separate download, reduced artifact, or a server-side search) that is not "
        "finalized yet.</p>"
        "<h2 id=\"api\">C API</h2>"
        "<p>The header <code>src/ui/fafnir_wasm_api.h</code> declares five functions. They are compiled only "
        "under <code>__EMSCRIPTEN__</code> and exported with <code>EMSCRIPTEN_KEEPALIVE</code>.</p>"
        + code_block(
            """#ifdef __EMSCRIPTEN__
#ifdef __cplusplus
extern "C" {
#endif

int fafnir_init(void);
int fafnir_set_fen(const char* fen);
const char* fafnir_go_movetime(int milliseconds);
void fafnir_stop(void);
int fafnir_eval(void);

#ifdef __cplusplus
}
#endif
#endif""",
            "c",
            "fafnir_wasm_api.h",
        )
        + table(
            ["Function", "Returns", "Behaviour"],
            [
                ["<code>fafnir_init</code>", "1 on success, 0 on error", "Initializes the NNUE probe once; repeated calls are safe"],
                ["<code>fafnir_set_fen</code>", "1 on success, 0 on error", "Stops any search, sets the board from a FEN, resets history"],
                ["<code>fafnir_go_movetime</code>", "UCI move string, or <code>0000</code>", "Synchronous search for at least 1 ms; returns the best move"],
                ["<code>fafnir_stop</code>", "void", "Requests the current search to stop"],
                ["<code>fafnir_eval</code>", "integer, 0 on error", "Returns the evaluation of the current board"],
            ],
        )
        + "<h2 id=\"js\">Calling from JavaScript</h2>"
        "<p>With the default Emscripten module, the functions are available through <code>ccall</code> or the "
        "exported function table. Strings returned by <code>fafnir_go_movetime</code> are pointers into the "
        "WASM heap; copy them with <code>UTF8ToString</code> before the next call.</p>"
        + code_block(
            """const Fafnir = await createFafnirModule();

// initialize once
if (!Fafnir.ccall('fafnir_init', 'number', [], [])) {
  throw new Error('Fafnir failed to initialize');
}

// set a position
const fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
const ok = Fafnir.ccall('fafnir_set_fen', 'number', ['string'], [fen]);
if (!ok) throw new Error('Invalid FEN');

// search synchronously
const ptr = Fafnir.ccall('fafnir_go_movetime', 'string', ['number'], [1000]);
const bestmove = Fafnir.UTF8ToString(ptr);

console.log(bestmove); // e2e4

// raw evaluation
const score = Fafnir.ccall('fafnir_eval', 'number', [], []);
console.log(score);""",
            "javascript",
            "Browser usage",
        )
        + callout(
            "The search call is synchronous. A long <code>movetime</code> blocks the JavaScript thread. The public "
            "web edition will need a worker or a cooperative yield loop; do not ship a long synchronous search on "
            "the main thread.",
            "warning",
            "Blocking call",
        )
        + "<h2 id=\"ci\">CI artifact</h2>"
        "<p>The <code>build-wasm.yml</code> workflow runs on every push to <code>main</code>/<code>master</code>. "
        "It builds the WASM target and uploads the result as the <code>Fafnir-WASM-Build</code> artifact. The "
        "workflow previously failed because its file selection pulled in the JNI bridge and the legacy probe; "
        "both are now excluded or removed, and CI no longer mutates the vendored <code>incbin.h</code>.</p>"
        + table(
            ["Workflow", "Trigger", "Output"],
            [
                ["<code>build-wasm.yml</code>", "Push / PR to main or master", "<code>Fafnir-WASM-Build</code> artifact"],
                ["<code>build-native.yml</code>", "Push / PR touching engine, assets or Makefile", "Native archives + UCI smoke test"],
                ["<code>deploy-pages.yml</code>", "Changes under <code>web/</code>", "This website on GitHub Pages"],
            ],
        )
        + "<h2 id=\"limits\">Current limitations</h2>"
        "<ul>"
        "<li>No published browser build or hosted UI.</li>"
        "<li>No threading: the C API drives one synchronous search at a time.</li>"
        "<li>No network embedding in the WASM artifact.</li>"
        "<li>No UCI console in the browser; the C API is the only interface.</li>"
        "<li>The API returns a move string, not a full analysis object; a UI needs a separate info channel for depth, score and PV.</li>"
        "</ul>"
        "<h2 id=\"next\">What ships next</h2>"
        "<p>The roadmap lists the public WASM edition as <em>in preparation</em>, without a date. The API is "
        "documented first so it can be reviewed and tested before a UI is built on top of it. See the "
        '<a href="roadmap.html">roadmap</a> for the current state.</p>'
        "</div>"
    )
    _doc(
        "docs-wasm.html",
        "WebAssembly",
        "Fafnir WebAssembly status, Emscripten build, the five-function C API, JavaScript usage, CI artifact and current limitations.",
        [("build", "Building", False), ("api", "C API", False), ("js", "JavaScript", False), ("ci", "CI artifact", False), ("limits", "Limitations", False), ("next", "What ships next", False)],
        body,
    )


def troubleshooting() -> None:
    body = (
        '<div class="prose">'
        "<p>Most failures fall into four groups: the binary does not match the CPU, the GUI never completes the "
        "UCI handshake, the WASM toolchain is missing, or a search is slower than expected. Start with the "
        "symptom table, then use the diagnostic commands at the end.</p>"
        "<h2 id=\"triage\">Quick triage</h2>"
        + table(
            ["Symptom", "Most likely cause", "First action"],
            [
                ["Process exits immediately on Windows", "AVX2/SSE4.2 build on an older CPU", "Use the safe <code>x86-64</code> archive"],
                ["<code>Illegal instruction</code> on Linux", "ISA mismatch", "Rebuild or download a lower ISA build"],
                ["GUI shows engine as unavailable", "Handshake not completed", "Run the binary manually and send <code>uci</code>"],
                ["No analysis appears", "Wrong engine path or protocol", "Verify <code>id name Fafnir 2</code> in the GUI log"],
                ["<code>unknown UCI option</code>", "Typo or unsupported option", "Compare with the five declared options"],
                ["<code>emcc: not found</code>", "Emscripten not in PATH", "Activate emsdk and retry"],
                ["Lower NPS than expected", "Power profile, ISA, or Hash/Threads", "Compare with the benchmark protocol"],
            ],
        )
        + "<h2 id=\"isa\">Illegal instruction</h2>"
        "<p>This is not a corrupt download. A binary compiled for AVX2 uses instructions that a CPU without AVX2 "
        "cannot execute. The safe <code>x86-64</code> archive runs on any x86-64 CPU. On Linux, confirm the flags:</p>"
        + code_block(
            """lscpu | grep -o -E 'sse2|ssse3|sse4_2|avx2|avx512f' | sort -u""",
            "bash",
            "Check CPU flags",
        )
        + "<p>If <code>avx2</code> is absent, do not use an AVX2 build. If <code>sse4_2</code> is absent, use the "
        "safe build. On Windows, the processor model in System Information is enough to choose between the three "
        "published archives.</p>"
        "<h2 id=\"linux\">Linux: permission or not found</h2>"
        + code_block(
            """chmod +x Fafnir
./Fafnir
# if the shell cannot find it:
./Fafnir-x86-64-avx2""",
            "bash",
            "Run the binary",
        )
        + "<p>Extracting a tarball does not always preserve the executable bit. The archive may contain a "
        "version-suffixed binary; list the directory after extraction.</p>"
        "<h2 id=\"gui\">The GUI does not see the engine</h2>"
        "<ol>"
        "<li>Run the binary in a terminal and send <code>uci</code>. If there is no <code>uciok</code>, the problem is the binary, not the GUI.</li>"
        "<li>Check that the GUI is configured for UCI, not WinBoard/CECP.</li>"
        "<li>Point the GUI at the extracted binary, not the archive or a source file.</li>"
        "<li>On Windows, avoid a path with unusual permissions or a network drive for the first test.</li>"
        "<li>Look at the GUI's engine log; it usually shows the raw handshake and the exact failure.</li>"
        "</ol>"
        + callout(
            "Fafnir prints a banner (<code>Fafnir 2</code>) when it starts, before the UCI handshake. A GUI that "
            "expects absolute silence until <code>uci</code> may log it as noise; it is not an error.",
            "info",
            "Startup banner",
        )
        + "<h2 id=\"options\">Unknown option messages</h2>"
        "<p>Fafnir declares five options: <code>Hash</code>, <code>Threads</code>, <code>MultiPV</code>, "
        "<code>Move Overhead</code> and <code>Aggressivity</code>. If the GUI tries to set anything else — a "
        "tablebase path, a book, a Syzygy option — the engine answers:</p>"
        + code_block("""info string unknown UCI option""", "text", "Response")
        + "<p>Remove the unsupported setting from the GUI profile, or accept the message. It does not stop the "
        "search. Do not add options to the GUI profile in the hope that they will work.</p>"
        "<h2 id=\"network\">The engine cannot find a network</h2>"
        "<p>It does not look for one. The native build embeds the SFNNv17 network with <code>incbin</code>. If a "
        "guide tells you to download a <code>.nnue</code> file for the published archives, that guide is for a "
        "different build. The WASM build is the exception: it disables embedding with <code>NNUE_EMBEDDING_OFF</code>.</p>"
        "<h2 id=\"wasm\">WASM build failures</h2>"
        + table(
            ["Error", "Cause", "Fix"],
            [
                ["<code>emcc: command not found</code>", "Emscripten not activated", "Source <code>emsdk_env.sh</code> or <code>emsdk_env.bat</code>"],
                ["JNI or <code>jni.h</code> errors", "Old workflow file selection", "Use the current <code>Makefile.wasm</code>; the JNI bridge is removed"],
                ["Legacy probe compile errors", "Old source list", "Do not add <code>third_party/stockfish/probe</code> to the build"],
                ["<code>incbin.h</code> modified locally", "An old CI step mutated it", "Restore the vendored file and rebuild"],
                ["Out of memory", "Emscripten and a large network", "Embedding is already off; limit parallel jobs"],
            ],
            cls="table--kv",
        )
        + "<h2 id=\"ci\">CI failures</h2>"
        + table(
            ["Job", "Symptom", "Likely fix"],
            [
                ["<code>build-native.yml</code>", "Windows mkdir aborts", "Pull the current Makefile; the bug is fixed in 2.0"],
                ["<code>build-native.yml</code>", "UCI smoke test fails", "Run the same sequence locally and compare the identity"],
                ["<code>build-wasm.yml</code>", "JNI / probe errors", "The workflow must exclude the removed bridge and legacy probe"],
                ["<code>deploy-pages.yml</code>", "Pages deployment fails", "Check the Pages environment and the <code>web/</code> artifact"],
            ],
            cls="table--kv",
        )
        + "<h2 id=\"performance\">Lower performance than expected</h2>"
        "<ul>"
        "<li><strong>Safe build.</strong> <code>x86-64</code> is SSE2 only. An AVX2 build is roughly twice as fast on this project's benchmark.</li>"
        "<li><strong>Power profile.</strong> A laptop on battery or a balanced profile can lose a large fraction of NPS.</li>"
        "<li><strong>Threads.</strong> More threads do not scale linearly and can reduce NPS per thread. Compare with one thread first.</li>"
        "<li><strong>Hash.</strong> A tiny hash causes re-search; a huge hash can hurt cache locality. Start at 16–128 MB.</li>"
        "<li><strong>Background load.</strong> Browsers, antivirus and other engines distort a benchmark.</li>"
        "</ul>"
        "<h2 id=\"diagnostics\">Diagnostic commands</h2>"
        + code_block(
            """# identity, options, readiness, one short search
printf 'uci\\nisready\\nposition startpos\\ngo depth 10\\nquit\\n' | ./bin/Fafnir

# verify the embedded network file in the source tree
sha256sum assets/nnue/nn-252f33942263.nnue

# list the native binaries produced by all-arches
ls -l bin/""",
            "bash",
            "Collect evidence",
        )
        + "<h2 id=\"help\">Getting help</h2>"
        "<p>Open an issue with the operating system, CPU model, archive name, the exact command or GUI action, and "
        "the raw engine output. For protocol issues, include the complete handshake and the line that failed. "
        "Do not paste only “it does not work”; the logs are what make a report reproducible.</p>"
        "<div class=\"btn-row mt-md\">" + btn("Open an issue", REPO + "/issues", "primary", icon="github")
        + btn("UCI reference", "docs-uci.html", "secondary") + "</div>"
        "</div>"
    )
    _doc(
        "docs-troubleshooting.html",
        "Troubleshooting",
        "Fix Fafnir startup, illegal-instruction, GUI handshake, unknown option, NNUE, WASM and performance problems with diagnostic commands.",
        [("triage", "Quick triage", False), ("isa", "Illegal instruction", False), ("linux", "Linux permissions", False), ("gui", "GUI detection", False), ("options", "Unknown options", False), ("network", "Network file", False), ("wasm", "WASM failures", False), ("ci", "CI failures", False), ("performance", "Performance", False), ("diagnostics", "Diagnostics", False), ("help", "Getting help", False)],
        body,
    )
