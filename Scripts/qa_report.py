#!/usr/bin/env python3
"""Turn automated acceptance results into a report and a manual checklist.

Inputs (all optional except --out):
  --junit FILE      Catch2 JUnit XML from `GranoTests "[qa]" -r junit`
  --metrics FILE    metrics.txt written by the [qa] tests (GRANO_QA_DIR)
  --tool NAME=RESULT[:IDS[:COLUMNS]]  result of an external check, e.g.
                    pluginval-vst3=pass:1.1,15.2:vst3   auval=fail:1.1:au
                    standalone=pass:1.1,1.3:sa
  --platform        macos | windows (which TESTING.md columns to fill)
  --testing FILE    TESTING.md (default: ../TESTING.md next to this script)
  --mark            write the results into TESTING.md
  --out DIR         where report.md and manual-checklist.md go

Marking rules:
  * [auto] engine tests that pass mark every column of the platform with x;
    a failure marks F. They are format-independent (same processor code).
  * [pre] tests are never marked: they only prove the engine side; the
    report lists them as "pre-checked, still listen/look".
  * External tools mark only the columns they name.
"""
import argparse
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

COLUMNS = {
    "macos":   {"vst3": "macOS (VST3)", "au": "macOS (AU)", "sa": "macOS (Standalone)"},
    "windows": {"vst3": "Win (VST3/DAW)", "sa": "Win (Standalone)"},
}
METRIC_FIELDS = {  # TESTING.md "Observed" line filled from metrics.txt
    ("16.1", "cpu_pct"): ("Observed CPU", "{:.1f}%"),
    ("16.2", "cpu_pct"): ("Observed CPU", "{:.1f}%"),
    ("16.4", "plugin_mb"): ("Observed RAM", "{:.0f}MB"),
}


def expand_ids(spec):
    """'3.1-3.6' -> ['3.1', ..., '3.6']; '13.5-13.6' -> ['13.5','13.6']; '2.6' -> ['2.6']."""
    out = []
    for part in spec.split(","):
        part = part.strip()
        m = re.fullmatch(r"(\d+)\.(\d+)-(\d+)\.(\d+)", part)
        if m and m.group(1) == m.group(3):
            sec = m.group(1)
            out += [f"{sec}.{i}" for i in range(int(m.group(2)), int(m.group(4)) + 1)]
        elif part:
            out.append(part)
    return out


def parse_junit(path):
    """-> list of dicts {ids, title, kind, ok, message}"""
    results = []
    root = ET.parse(path).getroot()
    for tc in root.iter("testcase"):
        name = tc.get("name", "")
        m = re.match(r"QA (\S+) (.*)", name)
        if not m:
            continue
        failure = tc.find("failure")
        error = tc.find("error")
        bad = failure if failure is not None else error
        msg = ""
        if bad is not None:
            msg = (bad.get("message") or "") + "\n" + (bad.text or "")
        results.append({
            "ids": expand_ids(m.group(1)),
            "title": m.group(2),
            "ok": bad is None,
            "message": msg.strip(),
            "kind": None,  # filled from tags below
            "name": name,
        })
    return results


def tags_from_source(src):
    """Map test name -> 'auto' | 'pre' from Tests/test_qa.cpp."""
    kinds = {}
    for m in re.finditer(r'TEST_CASE\("(QA [^"]+)",\s*"([^"]+)"\)', src):
        kinds[m.group(1)] = "auto" if "[auto]" in m.group(2) else "pre"
    return kinds


def parse_metrics(path):
    metrics = {}
    for line in Path(path).read_text().splitlines():
        m = re.match(r"(\S+) (\w+)=(\S+)", line.strip())
        if m:
            metrics[(m.group(1), m.group(2))] = float(m.group(3))
    return metrics


def parse_tool(spec):
    name, rest = spec.split("=", 1)
    parts = rest.split(":")
    result = parts[0]
    ids = expand_ids(parts[1]) if len(parts) > 1 and parts[1] else []
    cols = parts[2].split(",") if len(parts) > 2 and parts[2] else []
    return {"name": name, "result": result, "ids": ids, "cols": cols}


def testing_sections(text):
    """-> {id: (start, end)} line ranges of '### <id> ...' blocks."""
    lines = text.split("\n")
    heads = [(i, re.match(r"### (\d+\.\d+)\b", l)) for i, l in enumerate(lines)]
    heads = [(i, m.group(1)) for i, m in heads if m]
    sections = {}
    for k, (i, tid) in enumerate(heads):
        end = len(lines)
        for j in range(i + 1, len(lines)):
            if lines[j].startswith("## ") or lines[j].startswith("### ") or lines[j] == "---":
                end = j
                break
        sections[tid] = (i, end)
    return lines, sections


def mark(lines, sections, tid, label, value):
    if tid not in sections:
        return False
    s, e = sections[tid]
    pat = re.compile(r"(\*\*" + re.escape(label) + r":\*\* `\[)[ xF]?(\]`)")
    for i in range(s, e):
        if pat.search(lines[i]):
            lines[i] = pat.sub(lambda m: m.group(1) + value + m.group(2), lines[i], count=1)
            return True
    return False


def fill_observed(lines, sections, tid, field, text):
    if tid not in sections:
        return
    s, e = sections[tid]
    for i in range(s, e):
        if lines[i].startswith(f"**{field}:**"):
            lines[i] = re.sub(r"\*\*" + field + r":\*\* [^*]*", f"**{field}:** {text} (auto)  ", lines[i], count=1)
            return


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--junit")
    ap.add_argument("--metrics")
    ap.add_argument("--tool", action="append", default=[])
    ap.add_argument("--platform", default="macos", choices=COLUMNS.keys())
    here = Path(__file__).resolve().parent
    ap.add_argument("--testing", default=str(here.parent / "TESTING.md"))
    ap.add_argument("--qa-source", default=str(here.parent / "Tests" / "test_qa.cpp"))
    ap.add_argument("--mark", action="store_true")
    ap.add_argument("--out", required=True)
    ap.add_argument("--env", help="environment summary file to embed")
    a = ap.parse_args()

    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    cols = COLUMNS[a.platform]

    engine = parse_junit(a.junit) if a.junit and Path(a.junit).exists() else []
    kinds = tags_from_source(Path(a.qa_source).read_text()) if Path(a.qa_source).exists() else {}
    for r in engine:
        r["kind"] = kinds.get(r["name"], "pre")
    metrics = parse_metrics(a.metrics) if a.metrics and Path(a.metrics).exists() else {}
    tools = [parse_tool(t) for t in a.tool]

    text = Path(a.testing).read_text(encoding="utf-8")
    lines, sections = testing_sections(text)

    # Per TESTING id: status per column.  auto_status[id][col] = 'x' | 'F'
    status = {}

    def put(tid, col, value):
        cur = status.setdefault(tid, {})
        if cur.get(col) == "F":
            return  # a failure is sticky
        cur[col] = value

    pre_ok, pre_bad = {}, {}
    for r in engine:
        for tid in r["ids"]:
            if r["kind"] == "auto":
                for c in cols:
                    put(tid, c, "x" if r["ok"] else "F")
            else:
                (pre_ok if r["ok"] else pre_bad).setdefault(tid, []).append(r)
    for t in tools:
        v = {"pass": "x", "fail": "F"}.get(t["result"])
        if v is None:
            continue
        for tid in t["ids"]:
            for c in t["cols"]:
                if c in cols:
                    put(tid, c, v)

    # ── report.md ────────────────────────────────────────────────────────────
    rep = [f"# Grano acceptance — automated results ({a.platform})", ""]
    if a.env and Path(a.env).exists():
        rep += ["```", Path(a.env).read_text().strip(), "```", ""]
    rep += ["## External checks", "", "| Check | Result | TESTING ids | Columns |", "|---|---|---|---|"]
    for t in tools:
        icon = {"pass": "✅ pass", "fail": "❌ FAIL"}.get(t["result"], "⏭ " + t["result"])
        rep.append(f"| {t['name']} | {icon} | {', '.join(t['ids']) or '—'} | {', '.join(t['cols']) or '—'} |")
    rep += ["", "## Engine checks (headless, real processor)", "",
            "`auto` = covers the TESTING.md expectation; `pre` = engine side only, a person must still listen/look.", "",
            "| TESTING | Check | Kind | Result |", "|---|---|---|---|"]
    for r in engine:
        rep.append(f"| {', '.join(r['ids'])} | {r['title']} | {r['kind']} | {'✅' if r['ok'] else '❌ FAIL'} |")
    if metrics:
        rep += ["", "## Measurements", "", "| TESTING | Metric | Value |", "|---|---|---|"]
        for (tid, key), v in sorted(metrics.items(), key=lambda kv: [int(x) for x in kv[0][0].split(".")]):
            rep.append(f"| {tid} | {key} | {v:.2f} |")
        rep += ["", "CPU % is processing time / audio time on one core, measured offline "
                    "(comparable to a DAW's per-plugin meter, not to whole-system load)."]
    failures = [r for r in engine if not r["ok"]] + [t for t in tools if t["result"] == "fail"]
    if failures:
        rep += ["", "## Failures", ""]
        for r in engine:
            if not r["ok"]:
                rep += [f"### {r['name']}", "", "```", r["message"][:2000], "```", ""]
        for t in tools:
            if t["result"] == "fail":
                rep += [f"- **{t['name']}** failed — see its log in this folder.", ""]
    (out / "report.md").write_text("\n".join(rep) + "\n", encoding="utf-8")

    # ── manual-checklist.md: every TESTING test not fully covered ────────────
    fully = {tid for tid, c in status.items() if all(c.get(col) == "x" for col in cols)}
    man = [f"# Grano — manual checklist ({a.platform})", "",
           "Tests below still need a person. Automated results are in report.md; "
           "items marked *pre-checked* already passed an engine-side check, so you only need to listen/look.", ""]
    for tid, (s, e) in sections.items():
        if tid in fully:
            continue
        block = lines[s:e]
        note = []
        if tid in pre_ok:
            note.append("> *pre-checked (engine OK):* " + "; ".join(r["title"] for r in pre_ok[tid]))
        if tid in pre_bad:
            note.append("> **engine check FAILED:** " + "; ".join(r["title"] for r in pre_bad[tid]) + " — see report.md")
        partial = status.get(tid, {})
        if partial:
            note.append("> *already marked:* " + ", ".join(f"{cols[c]} [{v}]" for c, v in partial.items()))
        man += [block[0]] + note + block[1:] + ["", "---", ""]
    (out / "manual-checklist.md").write_text("\n".join(man) + "\n", encoding="utf-8")

    # ── TESTING.md marks ─────────────────────────────────────────────────────
    marked = 0
    if a.mark:
        for tid, c in status.items():
            for col, v in c.items():
                marked += mark(lines, sections, tid, cols[col], v)
        for (tid, key), (field, fmt) in METRIC_FIELDS.items():
            if (tid, key) in metrics:
                fill_observed(lines, sections, tid, field, fmt.format(metrics[(tid, key)]))
        if ("16.8", "rss_start_mb") in metrics and ("16.8", "rss_end_mb") in metrics:
            s, e = sections.get("16.8", (0, 0))
            for i in range(s, e):
                if lines[i].startswith("**RAM at start:**"):
                    lines[i] = (f"**RAM at start:** {metrics[('16.8', 'rss_start_mb')]:.0f}MB  "
                                f"**RAM at end:** {metrics[('16.8', 'rss_end_mb')]:.0f}MB (auto, "
                                f"{metrics.get(('16.8', 'minutes'), 0):.0f} min offline)")
        Path(a.testing).write_text("\n".join(lines), encoding="utf-8")

    n_ok = sum(r["ok"] for r in engine)
    print(f"engine checks: {n_ok}/{len(engine)} passed; external: "
          + ", ".join(f"{t['name']}={t['result']}" for t in tools))
    print(f"fully automated TESTING ids: {len(fully)}; manual checklist: {len(sections) - len(fully)} tests")
    if a.mark:
        print(f"TESTING.md: {marked} result boxes written")
    print(f"report: {out / 'report.md'}\nchecklist: {out / 'manual-checklist.md'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
