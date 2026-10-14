# Session log: S02 — Audit: support libraries & assembly

- **Date:** 2026-10-14
- **Session brief:** [S02](../sessions/S02-audit-libraries.md)
- **Branch / PR:** `worktree-agent-a5997e14974eed088` / no PR yet (the orchestrator merges Wave 1)
- **Status at end:** Done (in review)

## Goal
Work out which parts of `WIN32LIB/` and `WINVQ/` the Win32 game actually uses, and which of the
assembly files really need porting, so that S05 (strip dead platforms), S07 (asm to C++) and S09 (audio)
can be planned from the documentation alone.

## What was done
1. The worktree branch started at `0dc09bb`, before the wiki existed. Fast-forwarded it (no branch switch)
   to `0ebf96c` so the brief and templates were present.
2. Read `CODE/MAKEFILE` for the Win32 configuration: object list (with the `!ifdef WIN32` additions) and the
   `win95.lnk` link line. Found that besides `win32lib.lib` and `vqa32wp.lib` the game also links
   `vqm32wp.lib`, `keyboard.lib` (again), `ipx\wwipx32.lib`, `mpgdll.lib` and DirectShow libraries.
3. Wrote `members.py` to extract every sub-library's `OBJECTS` macro and map each member to its source
   (`WIN32LIB`: 18 sub-libraries, 90 members, 42 asm; `WINVQ`: 11 + 32 members, 17 asm).
4. Wrote `asmsyms.py` to list each asm file's exported symbols (following `INCLUDE`d `.INC` files, because
   most `GLOBAL` declarations live in `DRAWBUFF.INC` and similar) and grep for the C-visible name.
   First version missed every DRAWBUFF export because of that; fixed by reading includes.
5. Traced every symbol by hand. A raw grep hit is often wrong: same-named C++ methods
   (`GraphicViewPortClass::Fill_Rect`, `MonoClass::Text_Print`), calls inside `#ifndef WIN32` branches,
   disabled prototypes (`#if (0)` around the facing functions in `MISC.H`), macros (`Size_Of_Region`),
   and game re-definitions (`Calculate_CRC`, `SetPalette`, `Desired_Facing*`). Wrote `ifctx.py` to print
   the `#if` stack around a call site. Followed the indirect paths through `GBUFFER.H` inline wrappers,
   `MOUSE.CPP`, `SOUNDINT.CPP`, `WSA.CPP`, `ICONSET.CPP` and the VQA drawer configuration.
6. Checked the other directories: `WWFLAT32/` and `VQ/` are only referenced from the DOS branch of the
   makefile; `IPX/` is **not** DOS-only (it produces the Win95 thunk DLL and `wwipx32.lib` that the Win32
   build links); `LAUNCH/` is a DOS stub; `LAUNCHER/` is a separate MSVC patcher/launcher program;
   `TOOLS/` holds prebuilt asset tools.
7. Located HMI SOS and DirectSound use (only the SOS ADPCM codec is used on Win32; the SOS driver calls
   are compiled out).
8. Wrote [`docs/codebase/libraries.md`](../codebase/libraries.md) and
   [`docs/codebase/assembly-inventory.md`](../codebase/assembly-inventory.md); ticked the brief.

Dead ends worth knowing:
- `grep` for `Text_Print`, `Fill_Rect` or `Mono_*` in `CODE/` finds many hits that are all C++ methods
  or C++ re-implementations; none reaches the asm.
- `CODE/2KEYFBUF.ASM` has two `END` directives; everything after line 3929 is a dead copy of the DOS file
  and includes a `wwlib.i` that doesn't exist. Don't port the tail.

## Decisions
No ADR written (read-only session). Suggested ADRs for the orchestrator:
- **"Don't port unreferenced or shadowed asm; adopt the existing C++ ports."** Records that the 31 unneeded
  asm files in the link are deleted rather than ported, that `CODE/LCW.CPP` replaces `IFF/LCWUNCMP.ASM`
  and `CODE/ADPCM.CPP` replaces `AUDIO/SOSCODEC.ASM`, and that the asm versions of `Desired_Facing*`,
  `Random` and `Calculate_CRC` must never replace the C++ ones used by the simulation.
- **Movies**: ADR 0005 (proposed) should be decided before S07 plans the movie-only package (3 asm files,
  ~1,500 lines) and before S05 deletes `WINVQ/`. Note the 2x interpolation in `CODE/WINASM.ASM` is also used
  by the map-selection and score screens, not only movies.

Small decisions:
- Counted `CODE/IPXPROT.ASM` and the `IPXREAL.ASM` blob it includes as "in the link, dead".
- Treated "pulled" as "referenced by compiled code", using the documented `wlink` order rules; no link
  map exists to confirm it.

## Files changed
| File / folder | Change |
|---------------|--------|
| `docs/codebase/libraries.md` | New: link line, library member lists, sub-library purposes and callers, game overrides, other directories, audio (HMI SOS / DirectSound), delete candidates |
| `docs/codebase/assembly-inventory.md` | New: summary counts, S07 work packages, one row per linked asm file, delete list, calling-convention traps, simulation relevance, inline `#pragma aux` list, method |
| `docs/sessions/S02-audit-libraries.md` | Ticked tasks, status `In review` |
| `docs/logs/2026-10-14-S02-audit-libraries.md` | This log |

## Verification
No code to build. The audit itself was checked like this (run from the repository root):

```
$ find . -path ./.git -prune -o -type f -iname '*.asm' -print | cut -d/ -f2 | sort | uniq -c
     12 CODE
     17 IPX
      1 LAUNCH
      1 TOOLS
     19 VQ
    166 WIN32LIB
     23 WINVQ
    118 WWFLAT32
$ python3 -I run_all.py . <outdir>
65 linked asm files; 236 exported symbols
```

`run_all.py` writes `members.tsv` (133 members), `linked_asm.txt` (65 files) and `syms.tsv`
(236 exported symbols with reference counts). Every row of the inventory was then checked by hand with
`grep -n` and `ifctx.py`; each claim in the docs cites the `FILE:line` it rests on.

Count cross-check: 322 files to delete + 25 to port + 7 to stub + 3 movie-only = 357 (all asm files).

`WIN32LIB/SRCDEBUG/`: 60 files byte-identical to a library source (`cmp`), 33 with no counterpart.

## Determinism impact
None (documentation only). Findings relevant to determinism:
- No linked `.ASM` file is on the simulation path.
- Simulation-critical inline asm exists in `CODE/JSHELL.H` (`Fixed_To_Cardinal`, `Cardinal_To_Fixed`) and
  `CODE/COORD.CPP` (`calcx`/`calcy` used by `Move_Point`). Their C++ replacements must reproduce the exact
  32-bit / 16-bit semantics, including `Fixed_To_Cardinal`'s saturation. These belong to S01's
  classification and S07's porting.

## Problems & open questions
- **Missing sources:** `CODE/MAKEFILE` lists `PALETTE.OBJ` and `PASSEDIT.OBJ`, but `CODE/PALETTE.CPP`,
  `CODE/PASSEDIT.CPP` and the game's `PALETTE.H` (which defines `PaletteClass`) are not in the repository.
  Needs an owner (S01 to confirm, S06/S10 to rewrite).
- `mpgdll.lib` is on the link line but nothing in the repo builds it (`WIN32LIB/MOVIE` builds `mpeg.lib`).
- Unverified without a toolchain: `wlink` library-order resolution for duplicate symbols
  (`LCW_Uncompress` in two libraries, `sosCODECInitStream` twice in the same library).
- Unverified whether 320-pixel-wide mode (and so `ModeX_Blit`) can occur in the Win32 game.
- Unverified whether `Load_Icon_Set` (and so `REVERSE.ASM`) runs at all.
- Seen in passing, for S14: the Win32 branch of the screen shake in `CODE/CONQUER.CPP` (around line 5360)
  calls `Sim_Random_Pick`. If the shake can be triggered for one player only, that consumes the simulation
  RNG on one machine. Not checked further.
- Licensing: prebuilt HMI SOS libraries (`WINVQ/LIB/SOS*.LIB`, `WWFLAT32/.../SOSDW1PS.LIB`) are committed.
  They are unused by the Win32 build; S05 should delete them.

## Next steps
- S05: use the delete lists in `libraries.md` section 6 and `assembly-inventory.md` section 2.
- S07: plan around work packages S07-A … S07-E; decide S07-F after ADR 0005.
- S09: replace the DirectSound layer inside `WIN32LIB/AUDIO`; port the three decoders (S07-E or S09).
- S06/S08: do not change the layout or packing of `GraphicViewPortClass` or `WWMouseClass` until S07-A
  and S07-C have ported the asm that reads them.

## Scripts

All four scripts are plain Python 3 with no dependencies. Save them in a scratch directory and run
`python3 -I run_all.py <repo_root> <out_dir>`; `ifctx.py` is used by hand.

<details><summary><code>run_all.py</code></summary>

```python
# Usage: python3 -I run_all.py <repo_root> <out_dir>
# Runs members.py and asmsyms.py over the Win32 link set and writes TSVs.
import os, subprocess, sys
root, out = sys.argv[1], sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
W32 = 'AUDIO DIPTHONG DRAWBUFF FONT IFF KEYBOARD MEM MISC MONO PALETTE SHAPE TILE TIMER WW_WIN WSA PROFILE PLAYCD WINCOMM'.split()
def run(args):
    return subprocess.run([sys.executable, '-I'] + args, capture_output=True, text=True, check=True).stdout
w32 = run([os.path.join(here, 'members.py'), root, 'WIN32LIB'] + W32)
vq = run([os.path.join(here, 'members.py'), root, 'WINVQ', 'VQA32', 'VQM32'])
open(os.path.join(out, 'members.tsv'), 'w').write(w32 + vq)
asm = []
for line in (w32 + vq).splitlines():
    d, o, src = line.split('\t')
    for s in src.split(','):
        if s.upper().endswith('.ASM'):
            asm.append(os.path.join(root, d, s))
# CODE asm objects listed in CODE/MAKEFILE OBJECTS for WIN32 (checked by hand)
code_linked = ['2KEYFBUF.ASM', 'CPUID.ASM', 'WINASM.ASM', '2TXTPRNT.ASM', 'LCWCOMP.ASM', 'IPXPROT.ASM']
code_unlinked = ['2SUPPORT.ASM', 'SUPPORT.ASM', 'COORDA.ASM', 'KEYFBUFF.ASM', 'TXTPRNT.ASM', 'IPXREAL.ASM']
asm += [os.path.join(root, 'CODE', f) for f in code_linked]
open(os.path.join(out, 'linked_asm.txt'), 'w').write('\n'.join(os.path.relpath(a, root) for a in asm) + '\n')
allasm = asm + [os.path.join(root, 'CODE', f) for f in code_unlinked]
syms = run([os.path.join(here, 'asmsyms.py'), root] + allasm)
open(os.path.join(out, 'syms.tsv'), 'w').write(syms)
print(len(asm), 'linked asm files;', len(syms.splitlines()), 'exported symbols')
```

</details>

<details><summary><code>members.py</code></summary>

```python
# Usage: python3 -I members.py <repo_root> <TOPDIR> <SUBDIR>...
# Prints lib member list: dir, obj, source file(s) found in that directory.
import os, re, sys
root, top, subs = sys.argv[1], sys.argv[2], sys.argv[3:]

def objs_from_makefile(path):
    txt = open(path, 'rb').read().decode('latin1').replace('\r', '')
    out, inobj = [], False
    for l in txt.split('\n'):
        s = l.split('#')[0]
        if re.match(r'^\s*OBJECTS\s*\+?=', s):
            inobj = True
            s = s.split('=', 1)[1]
        if inobj:
            out += re.findall(r'([\w$]+\.obj)', s, re.I)
            if not s.rstrip().endswith('&'):
                inobj = False
    return out

def find_src(d, obj):
    base = obj[:-4].lower()
    return [f for f in sorted(os.listdir(d))
            if os.path.splitext(f)[0].lower() == base
            and os.path.splitext(f)[1].lower() in ('.cpp', '.asm', '.c')]

for sub in subs:
    d = os.path.join(root, top, sub)
    for o in objs_from_makefile(os.path.join(d, 'MAKEFILE')):
        print(f"{top}/{sub}\t{o}\t{','.join(find_src(d, o)) or 'MISSING'}")
```

</details>

<details><summary><code>asmsyms.py</code></summary>

```python
# Usage: python3 -I asmsyms.py <repo_root> <file.asm>...
# For each asm file: list exported procs/data with their TASM language spec,
# the C-visible name, and the first reference outside asm files.
#
# Output (TSV): asm_file  kind  asm_name  lang  c_name  n_code_refs  first_code_ref  n_lib_refs  first_lib_ref
#   n_code_refs   = number of lines in CODE/*.CPP|*.H that mention c_name as a word
#   n_lib_refs    = same, in WIN32LIB/ and WINVQ/ C/C++ sources and headers (excl. OLD/ test dirs)
import os, re, sys

root = sys.argv[1]
files = sys.argv[2:]

def read(p):
    return open(p, 'rb').read().decode('latin1').replace('\r', '')

def strip_comment(l):
    return l.split(';')[0]

decl_re = re.compile(r'^\s*(global|public|externdef)\s+(.*)$', re.I)
proc_ideal = re.compile(r'^\s*proc\s+(\w+)\s*(\w+)?', re.I)
proc_masm = re.compile(r'^\s*(\w+)\s+proc\b\s*(\w+)?', re.I)
label_re = re.compile(r'^\s*label\s+(\w+)', re.I)
data_re = re.compile(r'^\s*(\w+)\s+(db|dw|dd|dq|df|dt|byte|word|dword)\b', re.I)
colon_label = re.compile(r'^\s*(\w+)::?\s*')
LANGS = {'c', 'pascal', 'syscall', 'stdcall', 'nolanguage', 'basic', 'fortran', 'prolog', 'cpp'}

inc_re = re.compile(r'^\s*include\s+"?([^"\s]+)"?', re.I)
def with_includes(path, depth=0):
    # Returns the file text plus the text of any INCLUDEd .INC/.I files (globals often live there).
    txt = read(path)
    if depth > 3:
        return txt
    d = os.path.dirname(path)
    extra = []
    for l in txt.split('\n'):
        m = inc_re.match(strip_comment(l))
        if not m:
            continue
        name = m.group(1).replace('\\', '/').split('/')[-1]
        for cand_dir in (d, os.path.join(root, 'WIN32LIB', 'INCLUDE'), os.path.join(root, 'WINVQ', 'INCLUDE', 'VQM32'), os.path.join(root, 'CODE')):
            hit = [f for f in os.listdir(cand_dir) if f.lower() == name.lower()] if os.path.isdir(cand_dir) else []
            if hit:
                extra.append(with_includes(os.path.join(cand_dir, hit[0]), depth + 1))
                break
    return txt + '\n' + '\n'.join(extra)

def analyse(path):
    txt = with_includes(path)
    own = read(path)
    globals_, defined, proclang = {}, set(), {}
    # GLOBAL/PUBLIC declarations come from the file and its INCLUDEs;
    # definitions (PROC, labels, data) only from the file itself.
    for l in txt.split('\n'):
        m = decl_re.match(strip_comment(l))
        if m:
            for part in m.group(2).split(','):
                toks = part.strip().split()
                if not toks:
                    continue
                lang = ''
                if toks[0].lower() in LANGS and len(toks) > 1:
                    lang = toks[0].upper()
                    toks = toks[1:]
                nm = ' '.join(toks)
                name, _, typ = nm.partition(':')
                name = name.strip()
                if re.match(r'^\w+$', name):
                    globals_.setdefault(name, (lang, typ.strip().lower()))
    for l in own.split('\n'):
        s = strip_comment(l)
        if decl_re.match(s):
            continue
        m = proc_ideal.match(s)
        if m and m.group(1).lower() != 'near':
            defined.add(m.group(1)); proclang[m.group(1)] = (m.group(2) or '').upper(); continue
        m = proc_masm.match(s)
        if m:
            defined.add(m.group(1)); proclang[m.group(1)] = (m.group(2) or '').upper(); continue
        for r in (label_re, data_re, colon_label):
            m = r.match(s)
            if m:
                defined.add(m.group(1))
    out = []
    for name, (lang, typ) in globals_.items():
        if name not in defined:
            continue  # extern
        kind = 'proc' if (name in proclang or typ in ('near', 'far', 'proc')) else 'data'
        plang = proclang.get(name, '')
        lang = lang or (plang if plang in ('C', 'PASCAL', 'SYSCALL', 'NOLANGUAGE', 'STDCALL') else '')
        if lang == 'C':
            cname, conv = name, 'cdecl'
        elif name.endswith('_') and not name.startswith('_'):
            cname, conv = name[:-1], 'watcom-reg'
        elif name.startswith('_') and not name.startswith('__'):
            cname, conv = name[1:], 'cdecl(manual _)'
        else:
            cname, conv = name, (lang.lower() or 'none')
        out.append((kind, name, conv, cname))
    return out

def src_files(sub, exts=('.cpp', '.h', '.c')):
    for dp, dn, fn in os.walk(os.path.join(root, sub)):
        dn[:] = [d for d in dn if d.upper() not in ('OLD', 'OLDTEST', 'TEST', 'OLDMEM', 'MSVC', 'EXAMPLE', 'SRCDEBUG', '.GIT')]
        for f in fn:
            if os.path.splitext(f)[1].lower() in exts:
                yield os.path.join(dp, f)

cache = {}
def lines_of(p):
    if p not in cache:
        cache[p] = read(p).split('\n')
    return cache[p]

code_files = sorted(src_files('CODE'))
lib_files = sorted(list(src_files('WIN32LIB')) + list(src_files('WINVQ')))

def refs(cname, files):
    r = re.compile(r'\b' + re.escape(cname) + r'\b')
    hits = []
    for p in files:
        for i, l in enumerate(lines_of(p), 1):
            if r.search(l):
                hits.append(f"{os.path.relpath(p, root)}:{i}")
    return hits

for f in files:
    rel = os.path.relpath(f, root)
    for kind, name, conv, cname in sorted(analyse(f)):
        c = refs(cname, code_files)
        lb = refs(cname, lib_files)
        print('\t'.join([rel, kind, name, conv, cname, str(len(c)), c[0] if c else '-', str(len(lb)), lb[0] if lb else '-']))
```

</details>

<details><summary><code>ifctx.py</code></summary>

```python
# Usage: python3 -I ifctx.py <file> <line>...
# Prints the stack of enclosing #if/#ifdef/#ifndef/#else conditions for each line,
# so a reader can see whether a call is compiled in a given configuration.
import re, sys
path, lines = sys.argv[1], [int(x) for x in sys.argv[2:]]
src = open(path, 'rb').read().decode('latin1').replace('\r', '').split('\n')
stack = []
want = set(lines)
for i, l in enumerate(src, 1):
    s = l.strip()
    if re.match(r'#\s*if', s):
        stack.append(s)
    elif re.match(r'#\s*el', s):
        if stack:
            stack[-1] = stack[-1] + '  -> ' + s
    elif re.match(r'#\s*endif', s):
        if stack:
            stack.pop()
    if i in want:
        print(f"{path}:{i}: {s[:80]}\n    under: {' | '.join(stack) or '(none)'}")
```

</details>
