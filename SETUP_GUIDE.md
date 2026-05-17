# Setup Guide

Step-by-step setup for developing Grano. Run these once on each development machine (your Windows desktop and your MacBook M4).

This guide assumes you have a working terminal, git, and a code editor (VS Code recommended).

---

## 1. Prerequisites

### Both platforms

- **Git** ≥ 2.40
- **Node.js** ≥ 18 (for PAUL and Caveman installers)
- **Python** ≥ 3.10 (for Graphify; install via `uv` or system package manager)
- **CMake** ≥ 3.22
- **Claude Code** CLI installed and authenticated. Verify with `claude --version`.

### Windows-specific (desktop)

- **Visual Studio 2022** with the "Desktop development with C++" workload, including the Windows 10/11 SDK.
- **Git Bash** or **WSL2 Ubuntu** for running shell installers.

### macOS-specific (MacBook M4)

- **Xcode 15+** with command-line tools: `xcode-select --install`.
- **Homebrew**.

### Linux-specific (optional secondary target)

- `build-essential`, `libasound2-dev`, `libjack-jackd2-dev`, `libcurl4-openssl-dev`, `libfreetype-dev`, `libx11-dev`, `libxcomposite-dev`, `libxcursor-dev`, `libxinerama-dev`, `libxrandr-dev`, `libgtk-3-dev`, `libwebkit2gtk-4.1-dev`, `pkg-config`.

---

## 2. Install JUCE 8

Pick one approach.

### Approach A: JUCE as a CMake submodule (recommended for this project)

From your `~/dev` or wherever you keep code:

```bash
git clone --branch master --depth 1 https://github.com/juce-framework/JUCE.git ~/JUCE
```

You will reference `~/JUCE` from the project's `CMakeLists.txt`. No global install needed.

### Approach B: System-installed JUCE

Only do this if you want JUCE shared across many projects.

```bash
# macOS
brew install --cask juce

# Linux: download from juce.com or build from source
```

The project's CMake setup defaults to Approach A. If you go with B, you'll change one line in the top-level `CMakeLists.txt` (documented in `BUILD.md`).

---

## 3. Install PAUL

PAUL is the structured-development loop for Claude Code. It will manage the planning, execution, and reconciliation of every unit of work.

```bash
npx paul-framework
```

The installer asks where to install. Choose **local to project** so that the `.claude/commands/paul/` folder lives inside the Grano repo and travels with it. (You can always run `npx paul-framework --global` later if you want PAUL available in other projects too.)

Verify:

```bash
claude
# Inside Claude Code:
/paul:help
```

If you see the command list, PAUL is ready.

---

## 4. Install Caveman

Caveman compresses Claude's output by ~65% with no loss of technical accuracy. Across a long JUCE project this saves significant tokens and accelerates every reply.

```bash
# macOS / Linux / WSL / Git Bash on Windows
curl -fsSL https://raw.githubusercontent.com/JuliusBrussee/caveman/main/install.sh | bash

# Windows PowerShell
irm https://raw.githubusercontent.com/JuliusBrussee/caveman/main/install.ps1 | iex
```

After install:

```bash
claude
# Inside Claude Code:
/caveman full
```

Set caveman to `full` mode at the start of each session, or use `/caveman ultra` for extreme density during heavy iteration.

---

## 5. Install Graphify

Graphify builds a queryable knowledge graph of the codebase. The hook makes Claude consult the graph before reading any file, which becomes critical once the JUCE codebase has 20+ files.

### Install

```bash
# Recommended:
uv tool install graphifyy && graphify install

# Or with pipx:
pipx install graphifyy && graphify install

# Or plain pip:
pip install graphifyy && graphify install
```

> Note: the PyPI package name is `graphifyy` (double-y). Other `graphify*` packages are not affiliated.

### Configure for Claude Code integration

```bash
cd /path/to/grano  # the project root after you clone/init it
graphify claude install
graphify hook install  # auto-rebuild on git commit
```

Verify Claude has access:

```bash
claude
# Inside Claude Code:
/graphify .
```

Wait for it to build. You should see `graphify-out/graph.html`, `GRAPH_REPORT.md`, and `graph.json` appear in the project root. Open `graphify-out/graph.html` in your browser to explore the graph.

---

## 6. Bootstrap the Grano project

Unzip this bootstrap pack into an empty directory and initialize git + PAUL.

```bash
mkdir grano
cd grano
unzip /path/to/grano-project-bootstrap.zip -d .

git init
git add .
git commit -m "chore: bootstrap project structure"
```

Now initialize PAUL inside the project:

```bash
claude
# Inside Claude Code:
/paul:init
```

PAUL will read `.paul/PROJECT.md`, `.paul/ROADMAP.md`, and `.paul/SPECIAL-FLOWS.md` and confirm the loop is ready. It will create or update `.paul/STATE.md` to track the current phase.

---

## 7. Compress CLAUDE.md (one-time saving)

The repo includes a `CLAUDE.md` that gives Claude Code persistent project context. Compress it now so every session starts with ~40-60% less input cost:

```bash
claude
# Inside Claude Code:
/caveman-compress CLAUDE.md
```

The file is rewritten in place, code blocks and paths preserved byte-for-byte. The compression is one-shot — commit the result.

```bash
git add CLAUDE.md
git commit -m "chore: compress CLAUDE.md via caveman"
```

---

## 8. Build your first plan

The Foundation phase (F0) plan is already written at `.paul/phases/00-foundation/00-01-PLAN.md`. Execute it:

```bash
claude
# Inside Claude Code:
/paul:apply .paul/phases/00-foundation/00-01-PLAN.md
```

PAUL will load the plan, check that required skills (caveman, graphify, frontend-design) are loaded per `SPECIAL-FLOWS.md`, and then execute task by task. Each task has a `verify` step Claude will run.

When the plan completes, close the loop:

```bash
/paul:unify .paul/phases/00-foundation/00-01-PLAN.md
```

This creates `00-01-SUMMARY.md`, updates `STATE.md`, and logs decisions. Do not skip this — every PAUL plan must close.

---

## 9. Daily workflow

Once F0 is done, the rest of development follows the same loop:

```
/paul:progress         # see where you are
/paul:plan 01-engine   # create the next plan (F1: granular engine)
                       # (review it carefully before approving)
/paul:apply            # execute it
/paul:verify           # manual acceptance testing
/paul:unify            # close the loop
```

Helpful subcommands:

- `/paul:resume` — pick up after a break, get one clear next action.
- `/paul:handoff` — produce a comprehensive handoff document (useful when switching from Windows desktop to MacBook M4 mid-flow).
- `/caveman-stats` — see how many tokens Caveman has saved.
- `/graphify query "what connects GranularEngine to the audio thread?"` — query the codebase graph.
- `/caveman-commit` — generate a Conventional Commit message for the current diff.

---

## 10. Building the plugin

After F0 completes, you can build:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

See `BUILD.md` for platform-specific details, Release builds, code signing, and plugin validation with `pluginval` and `auval`.

---

## Troubleshooting

**PAUL commands not appearing in Claude Code**: restart Claude Code to reload slash commands.

**Caveman not auto-activating**: run `/caveman full` once per session, or follow the `--with-init` flag in the Caveman installer to make it always-on.

**Graphify graph stale**: run `/graphify . --update` to re-extract only changed files.

**JUCE not found by CMake**: edit `CMakeLists.txt` and set `JUCE_DIR` to your JUCE clone path.

**Audio thread crashes / xruns during development**: in F1 you'll add proper lock-free queues. Until then, expect some instability — that's why F1 exists.

---

## Sync between machines

Keep development synchronized via git. The `.paul/` folder is committed, including STATE.md, so picking up on the other machine reads the same state.

```bash
# Before switching machines:
git add . && git commit -m "wip: …" && git push

# On the other machine:
git pull
claude
/paul:resume
```

Graphify's `graphify-out/manifest.json` is gitignored (it's mtime-based and breaks across clones), but `graph.json` and `GRAPH_REPORT.md` are committed so the second machine has the graph immediately. Run `/graphify . --update` after pulling to refresh.
