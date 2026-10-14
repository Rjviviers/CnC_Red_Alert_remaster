#!/usr/bin/env python3
"""Compile-progress report: try to compile every translation unit on its own.

This is the scoreboard for porting the Watcom-era code to modern compilers
(session S06). It reads compile_commands.json from a CMake build directory,
runs each compile command independently (in parallel, never stopping on
errors), and writes:

  <build>/compile-progress.md     human-readable report (totals, top errors, per file)
  <build>/compile-progress.json   the same data, machine-readable
  <build>/compile-progress-logs/  full compiler output for every file

One command from the repository root (configures ./build if needed):

  python3 scripts/compile_progress.py

Exit status: 0 when the report was produced (even if every file fails to
compile), 2 when the build system itself is broken (configure failed, no
compile_commands.json, compiler could not be started).

If the environment variable GITHUB_STEP_SUMMARY is set (GitHub Actions), a
short summary is appended to that file.
"""

import argparse
import collections
import concurrent.futures
import datetime
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# GCC/Clang:  file:12:3: error: msg      file:1:10: fatal error: x.h: No such file or directory
# MSVC:       file(12): error C2065: msg  file(1): fatal error C1083: Cannot open include file: 'x.h': ...
ERROR_RE = re.compile(r"(?:\bfatal error\b|\berror\b)(?:\s+C\d+)?\s*:", re.I)
MISSING_GCC_RE = re.compile(r"fatal error:\s*(.+?):\s*No such file or directory")
MISSING_MSVC_RE = re.compile(r"C1083:\s*Cannot open include file:\s*'([^']+)'")
CXX_KEYWORDS = set("""alignas alignof asm auto bool break case catch char class const constexpr
const_cast continue decltype default delete do double dynamic_cast else enum explicit export
extern false float for friend goto if inline int long mutable namespace new noexcept nullptr
operator private protected public register reinterpret_cast return short signed sizeof static
static_assert static_cast struct switch template this throw true try typedef typeid typename
union unsigned using virtual void volatile wchar_t while""".split())
QUOTED_RE = re.compile(r"(‘[^’]*’|'[^']*'|\"[^\"]*\"|`[^']*')")


def fail(msg):
    print("compile_progress: ERROR: " + msg, file=sys.stderr)
    sys.exit(2)


def run_configure(build_dir, cmake_args):
    cmd = ["cmake", "-S", ROOT, "-B", build_dir]
    if not os.path.exists(os.path.join(build_dir, "CMakeCache.txt")) and shutil.which("ninja"):
        cmd += ["-G", "Ninja"]
    cmd += cmake_args
    print("+ " + " ".join(cmd), flush=True)
    if subprocess.call(cmd) != 0:
        fail("CMake configure failed")


def read_cache(build_dir):
    cache = {}
    try:
        with open(os.path.join(build_dir, "CMakeCache.txt"), encoding="utf-8", errors="replace") as f:
            for line in f:
                m = re.match(r"([A-Za-z0-9_]+):[A-Z]+=(.*)$", line.strip())
                if m:
                    cache[m.group(1)] = m.group(2)
    except OSError:
        pass
    return cache


def compiler_info(build_dir):
    """Return 'GNU 13.3.0' etc. from CMakeFiles/<ver>/CMakeCXXCompiler.cmake."""
    cmf = os.path.join(build_dir, "CMakeFiles")
    try:
        for entry in sorted(os.listdir(cmf)):
            path = os.path.join(cmf, entry, "CMakeCXXCompiler.cmake")
            if os.path.isfile(path):
                text = open(path, encoding="utf-8", errors="replace").read()
                cid = re.search(r'set\(CMAKE_CXX_COMPILER_ID "([^"]*)"\)', text)
                ver = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]*)"\)', text)
                return "%s %s" % (cid.group(1) if cid else "?", ver.group(1) if ver else "?")
    except OSError:
        pass
    return "unknown"


def asm_counts():
    """Count TASM sources listed (but not compiled) in cmake/RASources.cmake."""
    counts = {}
    names = {"GAME": "redalert", "WIN32LIB": "win32lib", "VQA32": "vqa32wp", "VQM32": "vqm32wp"}
    try:
        text = open(os.path.join(ROOT, "cmake", "RASources.cmake"), encoding="utf-8").read()
    except OSError:
        return counts
    for m in re.finditer(r"set\(RA_(\w+)_ASM_SOURCES\n(.*?)\)", text, re.S):
        counts[names.get(m.group(1), m.group(1))] = len(m.group(2).split())
    return counts


def target_of(entry):
    out = entry.get("output", "")
    if not out:
        # Older CMake: derive from the -o / /Fo argument.
        m = re.search(r"(?:-o\s+|/Fo)(\S+)", entry.get("command", ""))
        out = m.group(1) if m else ""
    m = re.search(r"CMakeFiles[\\/]([^\\/]+)\.dir[\\/]", out)
    return (m.group(1) if m else "unknown"), out


def first_error(output):
    for line in output.splitlines():
        if ERROR_RE.search(line):
            return line.strip()
    for line in output.splitlines():
        if line.strip():
            return line.strip()
    return "(no output)"


def categorize(err_line):
    m = MISSING_GCC_RE.search(err_line) or MISSING_MSVC_RE.search(err_line)
    if m:
        return "missing header: %s" % m.group(1).strip().replace("\\", "/").lower()
    m = ERROR_RE.search(err_line)
    msg = err_line[m.end():] if m else err_line
    code = re.search(r"\b(C\d{4})\b", err_line)
    # Hide names and numbers so similar errors group together, but keep C++
    # keywords: "expected identifier before 'false'" is a useful category.
    def _quoted(m):
        inner = m.group(0)[1:-1]
        return "'%s'" % inner if inner in CXX_KEYWORDS else "'…'"
    msg = QUOTED_RE.sub(_quoted, msg).strip()
    msg = re.sub(r"\d+", "N", msg)
    return ("%s: %s" % (code.group(1), msg)) if code else msg


def compile_one(entry, source_root, log_dir):
    target, out = target_of(entry)
    directory = entry["directory"]
    if out:
        os.makedirs(os.path.dirname(os.path.join(directory, out)), exist_ok=True)
    if "arguments" in entry:
        cmd = entry["arguments"]
    elif os.name == "nt":
        cmd = entry["command"]
    else:
        cmd = shlex.split(entry["command"])
    try:
        proc = subprocess.run(cmd, cwd=directory, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=600)
        output = proc.stdout.decode("utf-8", errors="replace")
        ok = proc.returncode == 0
    except subprocess.TimeoutExpired:
        output, ok = "error: compile timed out after 600 s", False
    except OSError as exc:
        return {"infra_error": "could not start compiler: %s" % exc}

    rel = os.path.relpath(entry["file"], source_root).replace("\\", "/")
    log_path = os.path.join(log_dir, target, rel.replace("/", "__") + ".log")
    os.makedirs(os.path.dirname(log_path), exist_ok=True)
    with open(log_path, "w", encoding="utf-8") as f:
        f.write(output)

    result = {"file": rel, "target": target, "ok": ok,
              "log": os.path.relpath(log_path, os.path.dirname(log_dir)).replace("\\", "/")}
    if not ok:
        err = first_error(output)
        err = err.replace(source_root.replace("\\", "/") + "/", "").replace(source_root + os.sep, "")
        result["first_error"] = err
        result["category"] = categorize(err)
        result["error_count"] = sum(1 for l in output.splitlines() if ERROR_RE.search(l))
    return result


def md_escape(text):
    return text.replace("|", "\\|").replace("\n", " ")


def pct(a, b):
    return "%.1f%%" % (100.0 * a / b) if b else "n/a"


def build_report(results, meta):
    by_target = collections.OrderedDict()
    for name in ("redalert", "win32lib", "vqa32wp", "vqm32wp"):
        by_target[name] = []
    for r in results:
        by_target.setdefault(r["target"], []).append(r)
    asm = meta["asm_not_compiled"]

    totals_rows = []
    all_ok = all_n = all_asm = 0
    for name, rs in by_target.items():
        ok = sum(1 for r in rs if r["ok"])
        n = len(rs)
        a = asm.get(name, 0)
        all_ok, all_n, all_asm = all_ok + ok, all_n + n, all_asm + a
        totals_rows.append((name, ok, n - ok, n, pct(ok, n), a))
    totals_rows.append(("**All**", all_ok, all_n - all_ok, all_n, pct(all_ok, all_n), all_asm))

    cats = collections.Counter(r["category"] for r in results if not r["ok"])
    top = cats.most_common(20)

    totals_md = ["| Target | Compiles | Fails | C++ files | Progress | ASM files (not compiled) |",
                 "|--------|---------:|------:|----------:|---------:|-------------------------:|"]
    for row in totals_rows:
        totals_md.append("| %s | %d | %d | %d | %s | %d |" % row)
    top_md = ["| Files | First error (normalised) |", "|------:|---------------------------|"]
    for cat, count in top:
        top_md.append("| %d | `%s` |" % (count, md_escape(cat).replace("`", "'")))

    header = [
        "# Compile progress report",
        "",
        "- **Generated:** %s" % meta["generated"],
        "- **Compiler:** %s · **Arch:** %s (pointer size %s) · **C++ standard:** %s · **Host:** %s"
        % (meta["compiler"], meta["arch"], meta["pointer_size"], meta["cxx_standard"], meta["host"]),
        "- Each C++ translation unit is compiled on its own; a file counts as compiling when the "
        "compiler exits with status 0. Assembly (TASM) files are listed but not compiled (session S07).",
        "",
    ]
    summary = header + ["## Totals", ""] + totals_md + [""] + \
        ["## Most common first errors", "",
         "The first error of each failing file, with names and numbers normalised.", ""] + top_md + [""]

    full = list(summary)
    full += ["## Per-file results", ""]
    for name, rs in by_target.items():
        if not rs:
            continue
        full += ["### %s" % name, "", "| File | Result | Errors | First error |",
                 "|------|--------|-------:|-------------|"]
        for r in sorted(rs, key=lambda r: r["file"].upper()):
            if r["ok"]:
                full.append("| `%s` | pass | 0 | |" % r["file"])
            else:
                full.append("| `%s` | FAIL | %d | `%s` |" % (
                    r["file"], r["error_count"], md_escape(r["first_error"]).replace("`", "'")))
        full.append("")

    data = {"meta": meta,
            "totals": [{"target": t.strip("*").lower() if t.startswith("**") else t,
                        "compiles": ok, "fails": f, "total": n, "asm_not_compiled": a}
                       for (t, ok, f, n, _, a) in totals_rows],
            "top_errors": [{"category": c, "files": n} for c, n in top],
            "files": sorted(results, key=lambda r: (r["target"], r["file"]))}
    return "\n".join(full), "\n".join(summary), data, (all_ok, all_n)


def main():
    ap = argparse.ArgumentParser(description="Compile every translation unit independently and report progress.")
    ap.add_argument("--build-dir", default=os.path.join(ROOT, "build"),
                    help="CMake build directory (default: ./build)")
    ap.add_argument("--no-configure", action="store_true",
                    help="do not (re-)run CMake configure first")
    ap.add_argument("--cmake-arg", action="append", default=[],
                    help="extra argument for the configure step (repeatable), e.g. --cmake-arg=-DRA_TARGET_ARCH=native")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--filter", default=None, help="only compile files whose path contains this text")
    ap.add_argument("--summary", default=os.environ.get("GITHUB_STEP_SUMMARY"),
                    help="append a short Markdown summary to this file (default: $GITHUB_STEP_SUMMARY)")
    args = ap.parse_args()

    build_dir = os.path.abspath(args.build_dir)
    if not args.no_configure:
        run_configure(build_dir, args.cmake_arg)

    ccj = os.path.join(build_dir, "compile_commands.json")
    if not os.path.isfile(ccj):
        fail("%s not found. Configure with a Ninja or Makefile generator "
             "(the Visual Studio generator does not write it)." % ccj)
    with open(ccj, encoding="utf-8") as f:
        entries = json.load(f)
    if args.filter:
        entries = [e for e in entries if args.filter.lower() in e["file"].lower()]
    if not entries:
        fail("compile_commands.json has no entries")

    cache = read_cache(build_dir)
    source_root = cache.get("CMAKE_HOME_DIRECTORY", ROOT)
    log_dir = os.path.join(build_dir, "compile-progress-logs")
    if os.path.isdir(log_dir):
        shutil.rmtree(log_dir)

    print("Compiling %d translation units with %d jobs..." % (len(entries), args.jobs), flush=True)
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = [pool.submit(compile_one, e, source_root, log_dir) for e in entries]
        for i, fut in enumerate(concurrent.futures.as_completed(futures), 1):
            r = fut.result()
            if "infra_error" in r:
                fail(r["infra_error"])
            results.append(r)
            print("[%d/%d] %s %s" % (i, len(entries), "ok  " if r["ok"] else "FAIL", r["file"]), flush=True)

    meta = {
        "generated": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
        "compiler": compiler_info(build_dir),
        "arch": cache.get("RA_TARGET_ARCH", "?"),
        "pointer_size": "?",
        "cxx_standard": cache.get("RA_CXX_STANDARD", "?"),
        "host": "%s %s" % (platform.system(), platform.machine()),
        "asm_not_compiled": asm_counts(),
    }
    try:
        for entry in os.listdir(os.path.join(build_dir, "CMakeFiles")):
            p = os.path.join(build_dir, "CMakeFiles", entry, "CMakeCXXCompiler.cmake")
            if os.path.isfile(p):
                m = re.search(r'set\(CMAKE_CXX_SIZEOF_DATA_PTR "(\d+)"\)', open(p).read())
                if m:
                    meta["pointer_size"] = m.group(1)
    except OSError:
        pass

    full_md, summary_md, data, (ok, n) = build_report(results, meta)
    md_path = os.path.join(build_dir, "compile-progress.md")
    with open(md_path, "w", encoding="utf-8") as f:
        f.write(full_md + "\n")
    with open(os.path.join(build_dir, "compile-progress.json"), "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as f:
            f.write(summary_md + "\n")

    print("\n%d of %d translation units compile (%s). Report: %s" % (ok, n, pct(ok, n), md_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
