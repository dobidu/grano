#!/usr/bin/env bash
# Grano macOS acceptance runner — automates what TESTING.md allows and writes a
# manual checklist for the rest.
#
#   Scripts/macos-test.sh [options]
#
#   --tag TAG        release to test (default: latest release on GitHub)
#   --local          test the plugins from build-macos/ instead of a release zip
#   --no-install     do not copy plugins into ~/Library (use what is installed)
#   --skip-build     reuse build-macos/ (engine checks need GranoTests built)
#   --soak-min N     length of the offline soak test 16.8 (default 30)
#   --mark           write results into TESTING.md (macOS columns)
#   -h | --help
#
# Steps:
#   1. environment summary (macOS, CPU, RAM)
#   2. download + unzip the release, install VST3 / AU / Standalone into
#      ~/Library/Audio/Plug-Ins and ~/Applications (existing copies backed up),
#      clear quarantine, refresh the AU cache
#   3. auval on the AU                        → TESTING 1.1 (AU)
#   4. pluginval --strictness-level 10 on VST3 and AU
#                                              → 1.1, 15.1–15.3 (VST3 / AU)
#   5. Standalone launch watchdog              → 1.1, 1.3 (Standalone)
#   6. build GranoTests (Release), run the unit suite and the [qa] engine
#      checks: formats, extremes, state, CPU, memory, soak
#   7. editor screenshots (fonts / layout reference for section 17)
#   8. report.md + manual-checklist.md in qa-results/macos-<date>/
#
# Needs: Xcode command line tools, cmake, python3; gh (optional, for downloads).
set -uo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
TAG=""; LOCAL=0; INSTALL=1; BUILD=1; SOAK=30; MARK=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --tag) TAG="$2"; shift 2 ;;
    --local) LOCAL=1; shift ;;
    --no-install) INSTALL=0; shift ;;
    --skip-build) BUILD=0; shift ;;
    --soak-min) SOAK="$2"; shift 2 ;;
    --mark) MARK=1; shift ;;
    -h|--help) sed -n '2,30p' "$0"; exit 0 ;;
    *) echo "unknown option: $1" >&2; exit 2 ;;
  esac
done

OUT="$REPO/qa-results/macos-$(date +%Y%m%d-%H%M)"
CACHE="$HOME/Library/Caches/grano-qa"
mkdir -p "$OUT" "$CACHE"
LOG="$OUT/run.log"
exec > >(tee -a "$LOG") 2>&1

c_step() { printf '\n\033[1;36m▶ %s\033[0m\n' "$*"; }
c_ok()   { printf '  \033[32m✓ %s\033[0m\n' "$*"; }
c_bad()  { printf '  \033[31m✗ %s\033[0m\n' "$*"; }
c_warn() { printf '  \033[33m! %s\033[0m\n' "$*"; }

TOOLS=()   # --tool arguments for qa_report.py

# ── 1. environment ───────────────────────────────────────────────────────────
c_step "Environment"
{
  echo "date:     $(date '+%Y-%m-%d %H:%M')"
  echo "macOS:    $(sw_vers -productVersion) ($(sw_vers -buildVersion))"
  echo "arch:     $(uname -m)"
  echo "cpu:      $(sysctl -n machdep.cpu.brand_string 2>/dev/null)"
  echo "cores:    $(sysctl -n hw.ncpu)"
  echo "ram:      $(( $(sysctl -n hw.memsize) / 1073741824 )) GB"
  echo "commit:   $(git -C "$REPO" rev-parse --short HEAD 2>/dev/null)"
} | tee "$OUT/env.txt"

for t in cmake python3 xcrun; do
  command -v "$t" >/dev/null || { c_bad "missing: $t"; [[ $t == xcrun ]] && echo "  run: xcode-select --install"; exit 1; }
done

# ── 2. get + install the plugins ─────────────────────────────────────────────
VST3=""; COMP=""; APP=""
if [[ $LOCAL == 1 ]]; then
  c_step "Using local build (build-macos)"
  ART="$REPO/build-macos/Grano_artefacts/Release"
  VST3="$ART/VST3/Grano.vst3"; COMP="$ART/AU/Grano.component"; APP="$ART/Standalone/Grano.app"
else
  c_step "Downloading release ${TAG:-latest}"
  if [[ -z "$TAG" ]]; then
    TAG=$(gh release list -R dobidu/grano --limit 1 --json tagName -q '.[0].tagName' 2>/dev/null \
          || curl -fsSL https://api.github.com/repos/dobidu/grano/releases | python3 -c 'import json,sys;print(json.load(sys.stdin)[0]["tag_name"])')
  fi
  ZIP="$CACHE/Grano-$TAG-macOS.zip"
  if [[ ! -f "$ZIP" ]]; then
    curl -fL -o "$ZIP" "https://github.com/dobidu/grano/releases/download/$TAG/Grano-$TAG-macOS.zip" \
      || { c_bad "download failed for $TAG"; exit 1; }
  fi
  rm -rf "$OUT/release"; mkdir -p "$OUT/release"
  ditto -x -k "$ZIP" "$OUT/release"
  VST3=$(find "$OUT/release" -name Grano.vst3 -maxdepth 4 | head -1)
  COMP=$(find "$OUT/release" -name Grano.component -maxdepth 4 | head -1)
  APP=$(find "$OUT/release" -name Grano.app -maxdepth 4 | head -1)
  echo "release:  $TAG" >> "$OUT/env.txt"
  c_ok "release $TAG unpacked"
fi
[[ -d "$VST3" ]] && c_ok "VST3: $VST3" || c_warn "VST3 not found"
[[ -d "$COMP" ]] && c_ok "AU:   $COMP" || c_warn "AU not found"
[[ -d "$APP"  ]] && c_ok "App:  $APP"  || c_warn "Standalone not found"

PI="$HOME/Library/Audio/Plug-Ins"
if [[ $INSTALL == 1 ]]; then
  c_step "Installing (backups in $OUT/backup)"
  mkdir -p "$PI/VST3" "$PI/Components" "$HOME/Applications" "$OUT/backup"
  install_one() {  # src dst
    [[ -d "$1" ]] || return 0
    [[ -e "$2" ]] && mv "$2" "$OUT/backup/" && c_warn "backed up existing $(basename "$2")"
    ditto "$1" "$2" && xattr -dr com.apple.quarantine "$2" 2>/dev/null
    c_ok "installed $2"
  }
  install_one "$VST3" "$PI/VST3/Grano.vst3"
  install_one "$COMP" "$PI/Components/Grano.component"
  install_one "$APP"  "$HOME/Applications/Grano.app"
  killall -9 AudioComponentRegistrar 2>/dev/null || true   # refresh AU cache
  sleep 2
fi
I_VST3="$PI/VST3/Grano.vst3"; I_COMP="$PI/Components/Grano.component"; I_APP="$HOME/Applications/Grano.app"
[[ $INSTALL == 0 ]] && { I_VST3="$VST3"; I_COMP="$COMP"; I_APP="$APP"; }

# ── 3. auval ─────────────────────────────────────────────────────────────────
c_step "auval"
AUID=$(auval -a 2>/dev/null | awk '/Bidu/ && /Gran/ {print $1, $2, $3; exit}')
if [[ -z "$AUID" ]]; then
  c_bad "Grano AU not registered (auval -a). Log out/in or reboot, then rerun with --no-install."
  TOOLS+=(--tool "auval=fail:1.1:au")
else
  if auval -strict -v $AUID > "$OUT/auval.log" 2>&1; then
    c_ok "auval PASS ($AUID)"; TOOLS+=(--tool "auval=pass:1.1:au")
  else
    c_bad "auval FAIL — $OUT/auval.log"; tail -15 "$OUT/auval.log"; TOOLS+=(--tool "auval=fail:1.1:au")
  fi
fi

# ── 4. pluginval ─────────────────────────────────────────────────────────────
c_step "pluginval (strictness 10)"
PV="$CACHE/pluginval.app/Contents/MacOS/pluginval"
if [[ ! -x "$PV" ]]; then
  curl -fsSL -o "$CACHE/pluginval.zip" \
    https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip \
    && ditto -x -k "$CACHE/pluginval.zip" "$CACHE" && chmod +x "$PV"
fi
run_pv() {  # name path column
  [[ -d "$2" ]] || { c_warn "$1: plugin missing, skipped"; TOOLS+=(--tool "pluginval-$1=skipped"); return; }
  if "$PV" --strictness-level 10 --validate-in-process --timeout-ms 120000 "$2" > "$OUT/pluginval-$1.log" 2>&1; then
    c_ok "pluginval $1 PASS"; TOOLS+=(--tool "pluginval-$1=pass:1.1,15.1-15.3:$3")
  else
    c_bad "pluginval $1 FAIL — $OUT/pluginval-$1.log"; tail -20 "$OUT/pluginval-$1.log"
    TOOLS+=(--tool "pluginval-$1=fail:1.1:$3")
  fi
}
if [[ -x "$PV" ]]; then
  run_pv vst3 "$I_VST3" vst3
  run_pv au   "$I_COMP" au
else
  c_bad "pluginval unavailable"
fi

# ── 5. Standalone ────────────────────────────────────────────────────────────
c_step "Standalone launch (10 s watchdog)"
if [[ -d "$I_APP" ]]; then
  "$I_APP/Contents/MacOS/Grano" > "$OUT/standalone.log" 2>&1 &
  SA_PID=$!
  sleep 10
  if kill -0 "$SA_PID" 2>/dev/null; then
    c_ok "Standalone alive after 10 s"
    screencapture -x "$OUT/standalone-screen.png" 2>/dev/null || true
    kill "$SA_PID" 2>/dev/null; wait "$SA_PID" 2>/dev/null
    TOOLS+=(--tool "standalone=pass:1.1,1.3:sa")
  else
    wait "$SA_PID"; c_bad "Standalone exited early (code $?) — $OUT/standalone.log"
    TOOLS+=(--tool "standalone=fail:1.1,1.3:sa")
  fi
else
  c_warn "Standalone app missing, skipped"
fi

# ── 6. build + engine checks ─────────────────────────────────────────────────
BUILD_DIR="$REPO/build-macos"
if [[ $BUILD == 1 ]]; then
  c_step "Building GranoTests (Release) — first build takes several minutes"
  GEN=(); command -v ninja >/dev/null && GEN=(-G Ninja)
  cmake -S "$REPO" -B "$BUILD_DIR" ${GEN[@]+"${GEN[@]}"} -DCMAKE_BUILD_TYPE=Release > "$OUT/cmake-configure.log" 2>&1 \
    || { c_bad "configure failed — $OUT/cmake-configure.log"; exit 1; }
  TARGETS=(--target GranoTests)
  [[ $LOCAL == 1 ]] && TARGETS=()
  cmake --build "$BUILD_DIR" --config Release -j "$(sysctl -n hw.ncpu)" ${TARGETS[@]+"${TARGETS[@]}"} > "$OUT/cmake-build.log" 2>&1 \
    || { c_bad "build failed — $OUT/cmake-build.log"; tail -30 "$OUT/cmake-build.log"; exit 1; }
  c_ok "built"
fi
GT=$(find "$BUILD_DIR" -name GranoTests -type f -perm -u+x | head -1)
[[ -x "$GT" ]] || { c_bad "GranoTests not found in $BUILD_DIR (run without --skip-build)"; exit 1; }

c_step "Unit tests (ctest)"
if ctest --test-dir "$BUILD_DIR" -C Release -j "$(sysctl -n hw.ncpu)" > "$OUT/ctest.log" 2>&1; then
  c_ok "$(grep -E 'tests passed' "$OUT/ctest.log")"
  TOOLS+=(--tool "unit-tests=pass")
else
  c_bad "$(grep -E 'tests passed|tests failed' "$OUT/ctest.log")"; TOOLS+=(--tool "unit-tests=fail")
fi

c_step "Engine acceptance checks [qa] (soak ${SOAK} min offline)"
rm -f "$OUT/metrics.txt"
GRANO_QA_DIR="$OUT" GRANO_QA_SOAK_MIN="$SOAK" "$GT" "[qa]" -r junit -o "$OUT/qa.xml" > "$OUT/qa.log" 2>&1
c_ok "done ($(grep -c '<testcase' "$OUT/qa.xml" 2>/dev/null) checks)"

# ── 7. screenshots ───────────────────────────────────────────────────────────
c_step "Editor screenshots (section 17 reference)"
mkdir -p "$OUT/shots"
GRANO_SHOT_DIR="$OUT/shots" "$GT" "[screenshot]" > "$OUT/shots.log" 2>&1 \
  && c_ok "$(ls "$OUT/shots" | wc -l | tr -d ' ') images in $OUT/shots" || c_warn "screenshot run failed"

# ── 8. report ────────────────────────────────────────────────────────────────
c_step "Report"
MARKARG=(); [[ $MARK == 1 ]] && MARKARG=(--mark)
python3 "$REPO/Scripts/qa_report.py" --platform macos --out "$OUT" --env "$OUT/env.txt" \
  --junit "$OUT/qa.xml" --metrics "$OUT/metrics.txt" ${TOOLS[@]+"${TOOLS[@]}"} ${MARKARG[@]+"${MARKARG[@]}"}
RC=$?
echo
echo "Results:   $OUT/report.md"
echo "Checklist: $OUT/manual-checklist.md   ← do these by hand in a DAW"
open "$OUT/report.md" 2>/dev/null || true
exit $RC
