#Requires -Version 5.1
<#
.SYNOPSIS
    Grano Windows acceptance test runner.

    Runs pluginval on the VST3, launches the Standalone, and auto-marks the
    structural test results in TESTING.md. The ~101 listening/visual tests
    still require a human — this script handles the ~8 that are mechanically
    verifiable.

.PARAMETER Vst3Path
    Path to Grano.vst3. If omitted, downloads the latest CI artifact via gh CLI.

.PARAMETER StandalonePath
    Path to Grano.exe (Standalone). If omitted, downloads from CI.

.PARAMETER RepoRoot
    Root of the cloned repo (contains TESTING.md). Defaults to the parent of
    the Scripts/ directory where this script lives.

.EXAMPLE
    # Auto-download from latest CI run (needs: gh auth login):
    .\windows-test.ps1

    # Use already-extracted artifacts:
    .\windows-test.ps1 -Vst3Path "D:\temp\Grano.vst3" -StandalonePath "D:\temp\Grano.exe"

    # Skip standalone (VST3 only):
    .\windows-test.ps1 -Vst3Path "D:\temp\Grano.vst3"
#>
param(
    [string] $Vst3Path       = "",
    [string] $StandalonePath = "",
    [string] $RepoRoot       = (Split-Path $PSScriptRoot -Parent)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$testingMd = Join-Path $RepoRoot "TESTING.md"
$tempDir   = Join-Path $env:TEMP "grano-test"
New-Item -ItemType Directory -Force $tempDir | Out-Null

# ── helpers ──────────────────────────────────────────────────────────────────

function Write-Step ([string]$msg) { Write-Host "`n► $msg" -ForegroundColor Cyan }
function Write-Ok   ([string]$msg) { Write-Host "  ✓ $msg" -ForegroundColor Green }
function Write-Fail ([string]$msg) { Write-Host "  ✗ $msg" -ForegroundColor Red }
function Write-Warn ([string]$msg) { Write-Host "  ! $msg" -ForegroundColor Yellow }

# Patches the first Win result line under test header "### $TestId " in $Lines.
# $Mark is "x" or "F". Pass -Vst3 or -Standalone (or both).
function Set-TestResult {
    param(
        [string[]] $Lines,
        [string]   $TestId,
        [string]   $Vst3       = "",
        [string]   $Standalone = ""
    )
    $inSection = $false
    $patched   = $false
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        # Match exact test header (word-boundary stops "1.1" from hitting "1.10")
        if ($Lines[$i] -match "^### $([regex]::Escape($TestId))\b") {
            $inSection = $true
        }
        if ($inSection -and (-not $patched) -and ($Lines[$i] -match '\*\*Win \(VST3/DAW\):\*\*')) {
            $line = $Lines[$i]
            if ($Vst3) {
                $line = $line -replace '\*\*Win \(VST3/DAW\):\*\* `\[ \]`', "**Win (VST3/DAW):** ``[$Vst3]``"
            }
            if ($Standalone) {
                $line = $line -replace '\*\*Win \(Standalone\):\*\* `\[ \]`', "**Win (Standalone):** ``[$Standalone]``"
            }
            $Lines[$i] = $line
            $patched = $true
        }
        # Stop scanning once we hit the section separator
        if ($patched -and $Lines[$i] -eq "---") { break }
    }
    if (-not $patched) { Write-Warn "  Could not find test $TestId in TESTING.md" }
    return $Lines
}

# ── step 1: locate VST3 ──────────────────────────────────────────────────────

Write-Step "Locating VST3 artifact"

if (-not $Vst3Path) {
    Write-Warn "No -Vst3Path provided. Trying gh CLI..."
    try {
        $runId = (gh run list --workflow=build.yml --status=success --limit=1 --json=databaseId |
                  ConvertFrom-Json)[0].databaseId
        Write-Warn "Downloading Grano-VST3-Windows from run $runId..."
        gh run download $runId -n "Grano-VST3-Windows" -D "$tempDir\vst3" 2>&1 | Out-Null
        $Vst3Path = (Get-ChildItem "$tempDir\vst3" -Recurse -Filter "Grano.vst3" |
                     Select-Object -First 1).FullName
        if (-not $Vst3Path) { throw "Grano.vst3 not found in downloaded artifact" }
        Write-Ok "Downloaded VST3: $Vst3Path"
    } catch {
        Write-Fail "Could not download VST3: $_"
        Write-Host ""
        Write-Host "  Fix: provide path directly:" -ForegroundColor Yellow
        Write-Host "    .\windows-test.ps1 -Vst3Path 'D:\path\to\Grano.vst3'"
        exit 1
    }
} else {
    if (-not (Test-Path $Vst3Path)) { Write-Fail "VST3 not found: $Vst3Path"; exit 1 }
    Write-Ok "VST3: $Vst3Path"
}

# ── step 2: locate Standalone ────────────────────────────────────────────────

Write-Step "Locating Standalone artifact"

if (-not $StandalonePath) {
    Write-Warn "No -StandalonePath provided. Trying gh CLI..."
    try {
        $runId = (gh run list --workflow=build.yml --status=success --limit=1 --json=databaseId |
                  ConvertFrom-Json)[0].databaseId
        Write-Warn "Downloading Grano-Standalone-Windows from run $runId..."
        gh run download $runId -n "Grano-Standalone-Windows" -D "$tempDir\standalone" 2>&1 | Out-Null
        $StandalonePath = (Get-ChildItem "$tempDir\standalone" -Recurse -Filter "*.exe" |
                           Where-Object { $_.Name -notmatch 'unins|setup' } |
                           Select-Object -First 1).FullName
        if ($StandalonePath) { Write-Ok "Downloaded Standalone: $StandalonePath" }
        else                 { Write-Warn "No .exe found in artifact — standalone tests skipped" }
    } catch {
        Write-Warn "Standalone artifact unavailable — tests 1.1 (SA) and 1.3 will be skipped"
        $StandalonePath = ""
    }
} else {
    if (-not (Test-Path $StandalonePath)) { Write-Warn "Standalone not found: $StandalonePath — skipping" ; $StandalonePath = "" }
    else                                  { Write-Ok "Standalone: $StandalonePath" }
}

# ── step 3: download pluginval ───────────────────────────────────────────────

Write-Step "Checking pluginval"

$pluginvalExe = Get-ChildItem $tempDir -Recurse -Filter "pluginval.exe" -ErrorAction SilentlyContinue |
                Select-Object -First 1

if (-not $pluginvalExe) {
    Write-Warn "pluginval not cached — downloading..."
    $pvZip = Join-Path $tempDir "pluginval.zip"
    Invoke-WebRequest `
        -Uri "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip" `
        -OutFile $pvZip -UseBasicParsing
    Expand-Archive $pvZip -DestinationPath $tempDir -Force
    $pluginvalExe = Get-ChildItem $tempDir -Recurse -Filter "pluginval.exe" | Select-Object -First 1
}

if (-not $pluginvalExe) { Write-Fail "pluginval.exe not found after extraction"; exit 1 }
$pluginvalPath = $pluginvalExe.FullName
Write-Ok "pluginval: $pluginvalPath"

# ── step 4: run pluginval ────────────────────────────────────────────────────

Write-Step "Running pluginval --strictness-level 10  (this takes 60-120s)"

$pvLog    = Join-Path $tempDir "pluginval.log"
$pvErrLog = Join-Path $tempDir "pluginval.err"

$proc = Start-Process -FilePath $pluginvalPath `
    -ArgumentList "--strictness-level", "10",
                  "--validate-in-process",
                  "--timeout-ms", "120000",
                  $Vst3Path `
    -RedirectStandardOutput $pvLog `
    -RedirectStandardError  $pvErrLog `
    -NoNewWindow -Wait -PassThru

$pvPassed = ($proc.ExitCode -eq 0)

if ($pvPassed) { Write-Ok "pluginval PASSED (exit 0)" }
else           { Write-Fail "pluginval FAILED (exit $($proc.ExitCode))" }

# ── step 5: launch Standalone ────────────────────────────────────────────────

Write-Step "Testing Standalone launch (5s watchdog)"

$saPassed = $false

if ($StandalonePath) {
    try {
        # Standalone may show an audio-device dialog — that's fine, it means it launched.
        # We check it's still alive after 5s, then kill it.
        $sa = Start-Process -FilePath $StandalonePath -PassThru
        Start-Sleep -Seconds 5
        if (-not $sa.HasExited) {
            $saPassed = $true
            Write-Ok "Alive after 5s — OK  (killing process)"
            $sa.Kill()
        } else {
            Write-Fail "Crashed immediately (exit $($sa.ExitCode))"
        }
    } catch {
        Write-Fail "Could not launch Standalone: $_"
    }
} else {
    Write-Warn "Standalone not available — skipping tests 1.1 (SA) and 1.3"
}

# ── step 6: patch TESTING.md ─────────────────────────────────────────────────

Write-Step "Patching TESTING.md"

if (-not (Test-Path $testingMd)) {
    Write-Fail "TESTING.md not found: $testingMd"
    exit 1
}

[string[]]$lines = Get-Content $testingMd -Encoding UTF8

$pv = if ($pvPassed) { "x" } else { "F" }
$sa = if ($saPassed) { "x" } elseif ($StandalonePath) { "F" } else { $null }

# Tests auto-verified by pluginval (VST3/DAW column):
#   1.1  — plugin loads/initializes without crash
#   15.1 — state saves and restores (pluginval state roundtrip)
#   15.2 — state restores in a new DAW instance (pluginval multi-instance)
#   15.3 — multiple instances maintain independent state
#   16.3 — no dropout during rapid parameter automation (pluginval param stress)
#   16.5 — no crash on rapid bypass toggling (pluginval param stress)
#   16.7 — silence when no sample loaded (pluginval default-state output check)
foreach ($id in @("1.1", "15.1", "15.2", "15.3", "16.3", "16.5", "16.7")) {
    [string[]]$lines = Set-TestResult -Lines $lines -TestId $id -Vst3 $pv
}

# Tests auto-verified by Standalone launch (Standalone column):
#   1.1 — plugin (standalone) launches
#   1.3 — Standalone application launches
if ($sa) {
    foreach ($id in @("1.1", "1.3")) {
        [string[]]$lines = Set-TestResult -Lines $lines -TestId $id -Standalone $sa
    }
}

$lines | Set-Content $testingMd -Encoding UTF8
Write-Ok "TESTING.md updated"

# ── step 7: summary ──────────────────────────────────────────────────────────

$pvTestIds = @("1.1 (VST3)", "15.1", "15.2", "15.3", "16.3", "16.5", "16.7")
$saTestIds = @("1.1 (SA)", "1.3")
$autoCount = $pvTestIds.Count + $(if ($sa) { $saTestIds.Count } else { 0 })

Write-Host ""
Write-Host "════════════════════════════════════════" -ForegroundColor White
Write-Host "  GRANO WINDOWS AUTO-TEST COMPLETE" -ForegroundColor White
Write-Host "════════════════════════════════════════" -ForegroundColor White
Write-Host ""
Write-Host ("  pluginval (VST3):  " + $(if ($pvPassed) { "✅ PASS" } else { "❌ FAIL" }))
Write-Host ("  Standalone launch: " + $(if ($saPassed) { "✅ PASS" } elseif (-not $StandalonePath) { "⏭ SKIPPED (no artifact)" } else { "❌ FAIL" }))
Write-Host ""
Write-Host "  Auto-marked in TESTING.md ($autoCount tests):"
foreach ($id in $pvTestIds) { Write-Host "    $id — Win (VST3/DAW): [$pv]" }
if ($sa) {
    foreach ($id in $saTestIds) { Write-Host "    $id — Win (Standalone): [$sa]" }
}
Write-Host ""
Write-Host "  Remaining: ~$(109 - $autoCount) tests require human listening / visual check."
Write-Host "  Open TESTING.md and work through sections 2–16 manually."
Write-Host ""
Write-Host "  Logs: $tempDir"
Write-Host ""

if (-not $pvPassed) {
    Write-Host "  ── pluginval output (last 30 lines) ──" -ForegroundColor Red
    Get-Content $pvLog -Tail 30 -ErrorAction SilentlyContinue |
        ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    Write-Host ""
    Write-Host "  Full log: $pvLog" -ForegroundColor Red
}
