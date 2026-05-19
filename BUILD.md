# BUILD.md

Build instructions for the Grano plugin. Covers CMake configuration, platform-specific dependencies, Debug and Release builds, plugin validation, and packaging.

For first-time developer setup (installing JUCE, PAUL, Caveman, Graphify), read `SETUP_GUIDE.md` first.

---

## Quick reference

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --config Debug

# Test
ctest --test-dir build --output-on-failure

# Validate (after pluginval is installed)
pluginval --strictness-level 5 build/Grano_artefacts/Debug/VST3/Grano.vst3
```

The artifacts land in `build/Grano_artefacts/<Config>/<Format>/`.

---

## Platform-specific setup

### Windows

**Required**:
- Visual Studio 2022 with the "Desktop development with C++" workload.
- Windows 10 SDK or Windows 11 SDK (installed via Visual Studio Installer).
- CMake ≥ 3.22 (Visual Studio bundles a compatible version; `cmake --version` to verify).
- Git for Windows.

**Recommended**:
- Git Bash for running shell-style commands.
- `pluginval.exe` from https://github.com/Tracktion/pluginval/releases.

**Build**:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

For Release:

```powershell
cmake --build build --config Release
```

Artifacts will be in `build\Grano_artefacts\Debug\` or `Release\`.

To install the VST3 system-wide for testing:

```powershell
# Run as admin
copy /Y build\Grano_artefacts\Debug\VST3\Grano.vst3 "C:\Program Files\Common Files\VST3\"
```

### macOS (Apple Silicon and Intel)

**Required**:
- Xcode 15 or later.
- Xcode Command Line Tools: `xcode-select --install`.
- CMake ≥ 3.22: `brew install cmake`.
- Git: included with Xcode CLT.

**Recommended**:
- `pluginval` for Mac from https://github.com/Tracktion/pluginval/releases.
- `auval` is included with macOS for AU validation.

**Build**:

```bash
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

`CMAKE_OSX_DEPLOYMENT_TARGET=11.0` is set automatically by `CMakeLists.txt`. Override with `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` if you need a newer minimum.

For a universal binary (Apple Silicon + Intel):

```bash
cmake -B build -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artifacts in `build/Grano_artefacts/<Config>/`. The AU bundle lives under `AU/Grano.component/`. To install locally for testing:

```bash
cp -R build/Grano_artefacts/Debug/VST3/Grano.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/Grano_artefacts/Debug/AU/Grano.component ~/Library/Audio/Plug-Ins/Components/
```

Validate AU:

```bash
auval -v aumu Gran Bidu
```

### Linux (Ubuntu 22.04+)

**Required system packages**:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  libasound2-dev \
  libjack-jackd2-dev \
  libcurl4-openssl-dev \
  libfreetype-dev \
  libx11-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxinerama-dev \
  libxrandr-dev \
  libgtk-3-dev \
  libwebkit2gtk-4.1-dev \
  libfontconfig1-dev \
  libgl1-mesa-dev
```

**Build**:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Linux produces VST3 and Standalone only (no AU). Install for testing:

```bash
mkdir -p ~/.vst3
cp -R build/Grano_artefacts/Debug/VST3/Grano.vst3 ~/.vst3/
```

Linux is the **secondary target**. CI builds Linux best-effort (failures do not block PRs). If you hit a Linux-only bug, file it as a deferred issue via `/paul:consider-issues` unless it's a portability regression introduced by recent changes.

---

## CMake configuration options

The top-level `CMakeLists.txt` exposes these options. Override with `-D<NAME>=<VALUE>`:

| Option | Default | Purpose |
|--------|---------|---------|
| `JUCE_DIR` | `~/JUCE` | Path to the JUCE 8 source tree (CMake `add_subdirectory`). |
| `CMAKE_BUILD_TYPE` | `Debug` | `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`. |
| `GRANO_BUILD_TESTS` | `ON` | Build the Catch2 test executable. Disable for release CI to skip test deps. |
| `GRANO_ENABLE_TSAN` | `OFF` | Build with ThreadSanitizer. Linux and macOS only. Slow. CI enables for one matrix entry. |
| `GRANO_ENABLE_ASAN` | `OFF` | Build with AddressSanitizer. Similar matrix. |
| `GRANO_USE_MP3` | `ON` | Enable `JUCE_USE_MP3AUDIOFORMAT`. Set OFF for distributions where MP3 patent licensing is a concern. |
| `CMAKE_OSX_ARCHITECTURES` | (host) | macOS only. Set `"arm64;x86_64"` for a universal binary. |
| `CMAKE_OSX_DEPLOYMENT_TARGET` | `11.0` | macOS only. Set in `CMakeLists.txt`; override here if you need a newer minimum. |

Example with options:

```bash
cmake -B build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DJUCE_DIR=/usr/local/JUCE \
  -DGRANO_ENABLE_TSAN=ON \
  -DGRANO_USE_MP3=OFF
```

---

## CMake structure

```
CMakeLists.txt                  Top-level configuration
├── Source/
│   └── (sources listed via target_sources, not glob)
└── Tests/
    └── CMakeLists.txt          Test target, Catch2 FetchContent
```

The top-level file:

1. Sets project info and C++20 standard.
2. Sets platform flags: `CMAKE_OSX_DEPLOYMENT_TARGET=11.0` on Apple (Big Sur minimum), `/utf-8` on MSVC (Windows source encoding).
3. Adds JUCE 8 as a subdirectory.
4. Calls `juce_add_plugin(Grano ...)` with formats VST3, AU (gated by `APPLE`), Standalone.
5. Lists sources via `target_sources(Grano PRIVATE ...)` — explicit, no globbing.
6. Sets compile definitions: `JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_MP3AUDIOFORMAT=${GRANO_USE_MP3}`.
7. Links JUCE modules: `juce_audio_utils`, `juce_audio_processors`, `juce_dsp`, `juce_gui_extra`, plus the recommended flags helpers.
8. If `GRANO_BUILD_TESTS`: `enable_testing()`, `add_subdirectory(Tests)`.
9. Sanitizer flags applied via `target_compile_options` and `target_link_options` per option.

The Tests CMakeLists fetches Catch2 v3 via `FetchContent` and creates `add_executable(GranoTests ...)`. `catch_discover_tests` registers them with CTest.

---

## Plugin validation

### pluginval (cross-platform, VST3 and AU)

Install from https://github.com/Tracktion/pluginval/releases.

```bash
pluginval \
  --strictness-level 5 \
  --validate-in-process \
  --timeout-ms 30000 \
  path/to/Grano.vst3
```

Strictness levels:

| Level | Use |
|-------|-----|
| 1-3 | Development sanity checks. |
| 5 | F0 acceptance criterion. CI gate. |
| 7 | F4 acceptance criterion. |
| 10 | F7 release gate. Full host simulation. |

CI runs strictness 10 on every push to `main` (raised from 5 in F7). Strictness 10 exercises full
host simulation; do not lower it without a documented reason.

### auval (macOS, AU only)

```bash
auval -v aumu Gran Bidu
```

The four-letter codes (`aumu`, `Gran`, `Bidu`) come from the AU bundle's manifest, set in `CMakeLists.txt`. `auval -a` lists all registered Audio Units; verify Grano appears.

`auval` exit code 0 = pass. Investigate any warnings before release.

> **macOS 14+ (Sonoma / Sequoia):** `auval` requires the AU component to be code-signed with an
> Apple Developer ID certificate. Unsigned builds will report a load failure (exit 2). The AU
> loads and runs correctly in compatible DAWs (Logic Pro, GarageBand, Reaper) regardless. Code
> signing is documented in the "Code signing and notarization" section below. auval CI validation
> is planned once Developer ID is in place.

### Manual DAW smoke tests

Before tagging a release, load Grano in at least three DAWs per platform:

| Platform | DAWs |
|----------|------|
| Windows | Reaper, Ableton Live, FL Studio |
| macOS | Logic Pro, Ableton Live, Reaper |
| Linux | Reaper, Bitwig Studio, Ardour |

Confirm:
- Plugin loads without crash.
- All parameters appear in the host's automation list.
- Preset save/load via the host works.
- CPU usage is within budget (`ARCHITECTURE.md`).
- No xruns at 256-sample buffer @ 48 kHz with default settings.

---

## Release builds

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

Release builds:
- Disable assertions (`NDEBUG` defined).
- Enable LTO via `juce::juce_recommended_lto_flags`.
- Strip debug symbols (or keep them in a separate `.dSYM` / `.pdb`).
- Apply compiler optimization (`-O3` on Clang/GCC, `/O2` on MSVC).

The CMake `juce_recommended_*` helpers handle most of this.

---

## Code signing and notarization (F7)

### macOS

```bash
# Sign
codesign --force --sign "Developer ID Application: Bidu (TEAMID)" \
  --options runtime \
  --entitlements Resources/entitlements.plist \
  build/Grano_artefacts/Release/AU/Grano.component

codesign --force --sign "Developer ID Application: Bidu (TEAMID)" \
  --options runtime \
  --entitlements Resources/entitlements.plist \
  build/Grano_artefacts/Release/VST3/Grano.vst3

codesign --force --sign "Developer ID Application: Bidu (TEAMID)" \
  --options runtime \
  build/Grano_artefacts/Release/Standalone/Grano.app

# Notarize (Xcode 13+)
xcrun notarytool submit Grano.zip \
  --apple-id "your.apple.id@example.com" \
  --team-id "TEAMID" \
  --password "@keychain:AC_PASSWORD" \
  --wait

# Staple
xcrun stapler staple build/Grano_artefacts/Release/Standalone/Grano.app
xcrun stapler staple build/Grano_artefacts/Release/VST3/Grano.vst3
xcrun stapler staple build/Grano_artefacts/Release/AU/Grano.component
```

### Windows

For beta: self-signed certificate is acceptable, documented in the README's installation notes.

For 1.0 release: purchase a code-signing certificate (DigiCert, SSL.com, Sectigo). Sign with `signtool.exe`:

```powershell
signtool sign /fd SHA256 /a /tr http://timestamp.sectigo.com /td SHA256 ^
  build\Grano_artefacts\Release\VST3\Grano.vst3
```

### Linux

No code signing required. Distribute via tarball or AppImage.

---

## Installers (F7)

### macOS .pkg

Use `pkgbuild` and `productbuild`:

```bash
pkgbuild \
  --root build/Grano_artefacts/Release/ \
  --identifier br.com.bidu.grano \
  --version 1.0.0 \
  --install-location /Library/Audio/Plug-Ins \
  Grano-1.0.0.pkg
```

Sign the pkg with the Developer ID Installer cert, then notarize.

### Windows .exe (Inno Setup)

Create `Resources/Installer/grano-installer.iss` defining install paths. Build with Inno Setup compiler. Sign the resulting `.exe` with `signtool`.

### Linux .tar.gz

```bash
cd build/Grano_artefacts/Release
tar -czf grano-1.0.0-linux-x86_64.tar.gz VST3/ Standalone/
```

Document install instructions in the README.

---

## Continuous Integration

See `.github/workflows/ci.yml`. The workflow:

1. Runs on push to `main` and on pull_request to `main`.
2. Gating matrix: `windows-latest`, `macos-latest`.
3. Per OS: checkout, clone JUCE `8.0.12` (pinned via `JUCE_VERSION` env var), CMake configure with Release (macOS adds `-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`), build, run ctest, run pluginval at strictness 5, upload artifacts.
4. Separate `linux-best-effort` job on `ubuntu-22.04` with `continue-on-error: true`.
5. Separate `tsan` job on `ubuntu-22.04`: Debug build + `GRANO_ENABLE_TSAN=ON`, runs GranoTests under ThreadSanitizer.
6. On release tags (`v*.*.*`), an additional `release` job creates installers and uploads to the GitHub release.

CI cache: JUCE clone cached by version tag + OS. CMake build dir cached by config hash. Saves ~5 minutes per run. To upgrade JUCE, change `JUCE_VERSION` in `.github/workflows/ci.yml`.

---

## Troubleshooting

**`fatal error: 'juce_audio_utils/juce_audio_utils.h' file not found`**
- JUCE_DIR is wrong. Verify `~/JUCE` exists and is a JUCE 8 clone.

**`auval` reports "Authentication failure"**
- The AU was not signed, or the signature is broken. Re-sign and re-validate.

**pluginval crashes on a specific test**
- Reproduce with `--repeat 1 --random-seed <seed>`. Capture the seed. File the bug, then fix it.

**Linux build fails with "GTK 3 not found"**
- Install `libgtk-3-dev`. Some JUCE features require GTK.

**TSAN reports false positives near JUCE internals**
- `Tests/tsan_suppressions.txt` already suppresses known JUCE 8 false positives (`WaitableEvent::signal`, `SharedResourcePointer<TimerThread>`). Add new entries only after verifying the race is in JUCE internals — check `juce::` frames in the TSAN report and cross-reference the JUCE issue tracker.

**CMake reconfigures from scratch every build**
- Delete `build/CMakeCache.txt` and reconfigure. Sometimes IDE-generated cache fights with command-line CMake.

**Standalone shows white window on launch (Windows)**
- Likely a font loading issue. Confirm Inter and JetBrains Mono are embedded in `BinaryData`. Add a fallback to `juce::Font::getDefaultMonospacedFontName()`.
