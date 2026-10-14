# Session log: S07-D — Assembly → C++: data codecs

- **Date:** 2026-10-14
- **Session brief:** [S07](../sessions/S07-asm-to-cpp.md) (package S07-D in [assembly-inventory.md](../codebase/assembly-inventory.md))
- **Branch / PR:** `worktree-agent-a3908ecc48cebfd75` (based on `claude/cool-bohr-9vg7ac` at `66de4c5`) / not pushed
- **Status at end:** Done (in review)

## Goal
Replace the data-codec assembly with C++ that gives the same bytes: adopt `CODE/LCW.CPP` for `LCW_Uncompress`
(`WIN32LIB/IFF/LCWUNCMP.ASM`), port the LCW compressor (`CODE/LCWCOMP.ASM`), `Apply_XOR_Delta*`
(`WIN32LIB/WSA/XORDELTA.ASM`), `Mem_Copy` (`MEM/MEM_COPY.ASM`), `Reverse_*` (`MISC/REVERSE.ASM`), and keep only
`RandNumb` from `MISC/RANDOM.ASM`. Unit-test everything without game assets.

## What was done
1. `git merge --ff-only claude/cool-bohr-9vg7ac` (head `66de4c5`, `tests/ra_test.h` present).
2. Read every asm file of the package and the existing C++ (`CODE/LCW.CPP`, the unbuilt duplicate `CODE/LCWUNCMP.CPP`)
   and compared them. Findings:
   - The asm `LCW_Uncompress` is **bounded**: it clips every command to the `length` argument and stops when the output is
     full. `LCW.CPP` ignores `length`. Library callers decode in place (WSA, VQA), so the bound matters for them.
   - `LCW.CPP`'s long run (`0xFE`) filled with aligned 8-byte stores: it could write up to 4 bytes past the end of the run,
     and for a run shorter than the 1-4 bytes it wrote before its first aligned store the count underflowed (huge write). It also cast a
     pointer to `unsigned`, which does not compile for 64-bit. The asm wrote runs exactly.
   - The asm compressor reads up to 64 bytes past its input (the run check looks 64 bytes ahead) and misbehaves for inputs
     of 0 or 1 bytes. Worked through the asm: the over-read can never change the output (any run found that near the end is
     shorter than 65), so the port just skips the check there.
   - `RandNumb` is only written (`CODE/INIT.CPP`); `Random`/`Get_Random_Mask` are never called by the game.
     `WIN32LIB/MISC/IRANDOM.CPP` (`IRandom`, also never called) references `Get_Random_Mask`.
3. **LCW decoder:** rewrote the body of `CODE/LCW.CPP` as one `Decode(src, dst, length, bounded)` with two entry points:
   `LCW_Uncomp` (unbounded, as before) and `extern "C" LCW_Uncompress` (bounded, like the asm). Runs are a plain byte fill.
   Deleted `WIN32LIB/IFF/LCWUNCMP.ASM` and the unbuilt duplicate `CODE/LCWUNCMP.CPP`, so there is exactly one LCW decoder.
4. **Ports:** `CODE/LCWCOMP.CPP` (`LCW_Comp`, a line-by-line port of the asm's control flow, including its tie rule and the
   "last byte is never in a run" quirk), `WIN32LIB/WSA/XORDELTA.CPP`, `WIN32LIB/MEM/MEM_COPY.CPP` (`memmove` plus the asm's
   NULL/same-pointer checks), `WIN32LIB/MISC/REVERSE.CPP`, `WIN32LIB/MISC/RANDOM.CPP` (`long RandNumb = 0x12349876`).
   Deleted the five `.ASM` files; updated `cmake/RASources.cmake` (5 lines added, 6 removed).
5. **Asm oracle for the tests:** transcribed the original asm of `LCW_Comp`, `LCW_Uncompress`, both XOR-delta functions,
   `Random` and the `Reverse_*` functions into GNU assembler (Intel syntax) in `tests/s07d_asm_oracle.cpp`, instruction for
   instruction (TASM `PROC/ARG/LOCAL/USES` expanded into explicit frames). It builds only on 32-bit x86 GCC/Clang/ELF, i.e.
   the `linux-gcc-x86` CI job; elsewhere it is empty and those tests are skipped. See [ADR-0019](../decisions/0019-one-lcw-decoder-and-asm-oracle-tests.md).
6. **Tests** (`tests/tests.cmake`, `# --- S07-D ---`): three executables, 27 tests on x86 (21 without the oracle):
   hand-built LCW streams for every command, hand-derived compressor outputs, round trips over generated inputs (1 byte to
   16 KB, four kinds of data), the bound of `LCW_Uncompress`, the game decoder against a verbatim copy of the 1995
   `LCW_Uncomp`, XOR-delta vectors (short/long forms, viewport row wrapping, copy vs XOR) and a round trip through a small
   encoder written from the format, `Mem_Copy` against `memmove` for all overlaps, `Reverse_*` values, `RandNumb`'s initial
   value, and the old `Random()` sequence (golden values taken from the original instructions). With the oracle:
   800 compressor comparisons, about 4,700 decoder comparisons, 1,100 XOR-delta comparisons, 100,000 `Random` steps.
7. **Mutation check** of the oracle tests: changing the compressor's tie rule (`>=` to `>`), removing the run quirk, removing
   one clip in the bounded decoder, or changing the viewport skip wrap (`>=` to `>`) each makes an oracle test fail.
8. Docs: new [file-formats.md](../codebase/file-formats.md) (LCW and XOR delta), inventory rows marked *Ported (S07-D)*,
   brief sub-items ticked, ADR-0019 (Proposed).

## Decisions
- [ADR-0019](../decisions/0019-one-lcw-decoder-and-asm-oracle-tests.md) (Proposed): one LCW decoder with a bounded and an
  unbounded entry point; asm ports are checked against a test-only transcription of the original asm.
  **Not added to `docs/decisions/README.md`** (index files are the integrator's); please add it.
- `RandNumb` is defined with **C++ linkage** to match its only declaration (`extern long RandNumb;`, `CODE/INIT.CPP:108`).
  With GCC/Clang that is the same symbol as C linkage; with MSVC it is not, and no asm needs the C name any more.
- `Random()` and `Get_Random_Mask()` are not provided (ADR-0010). The old sequence is pinned in `tests/s07d_misc_test.cpp`
  for the record only.
- `Largest_Mem_Block` is not provided: it was inside `IF 0` in `MEM_COPY.ASM`, so it never existed; its only caller,
  `CODE/ALLOC.CPP`, is not built.
- Degenerate inputs the asm could not handle are defined in the port and documented: `LCW_Comp` of 0 or 1 bytes, XOR-delta
  counts of 0 (the asm looped 2^32 times), viewport `width <= 0` (the asm looped forever).
- New `.CPP` files declare their own `extern "C"` prototypes and define `__cdecl` as empty on non-MSVC compilers when it is
  not already defined, instead of including the original headers (which S06a is still fixing). No header was changed:
  no edits in `WIN32LIB/INCLUDE` or `CODE/*.H`.

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/LCW.CPP` | Decoder rewritten as one `Decode()`; `LCW_Uncomp` unchanged in behaviour; new `extern "C" LCW_Uncompress` (bounded) |
| `CODE/LCWCOMP.CPP` | New: C++ port of `CODE/LCWCOMP.ASM` (`LCW_Comp`) |
| `CODE/LCWCOMP.ASM`, `CODE/LCWUNCMP.CPP` | Deleted (the `.CPP` was an unbuilt duplicate of the decoder) |
| `WIN32LIB/IFF/LCWUNCMP.ASM` | Deleted (replaced by `CODE/LCW.CPP`) |
| `WIN32LIB/WSA/XORDELTA.CPP` (was `.ASM`) | New port |
| `WIN32LIB/MEM/MEM_COPY.CPP` (was `.ASM`) | New port (`memmove`) |
| `WIN32LIB/MISC/REVERSE.CPP` (was `.ASM`) | New port |
| `WIN32LIB/MISC/RANDOM.CPP` (was `.ASM`) | Only `long RandNumb` |
| `cmake/RASources.cmake` | Added the 5 new `.CPP`s next to related entries; removed 6 `.ASM` entries |
| `tests/s07d_lcw_test.cpp`, `tests/s07d_xordelta_test.cpp`, `tests/s07d_misc_test.cpp` | New tests |
| `tests/s07d_asm_oracle.cpp`, `tests/s07d_asm_oracle.h` | Test-only transcription of the original asm |
| `tests/tests.cmake` | Three `ra_add_test` calls under `# --- S07-D ---` |
| `docs/codebase/file-formats.md` | New page: LCW and XOR-delta formats, both decoders, the compressor's rules, where they are used |
| `docs/codebase/assembly-inventory.md` | Rows 3, 29, 32, 41, 42, 47 marked *Ported (S07-D)*; S07-D package row; note in section 4 |
| `docs/decisions/0019-one-lcw-decoder-and-asm-oracle-tests.md` | New ADR (Proposed) |
| `docs/sessions/S07-asm-to-cpp.md` | S07-D sub-items ticked under each task |
| `docs/logs/2026-10-14-S07D-data-codecs.md` | This log |

## Verification
GCC 13.3.0, Ubuntu 24.04, CMake 3.28, Ninja.

```sh
cmake -S . -B build -G Ninja                       # RA_TARGET_ARCH=x86 (default): -m32, asm oracle active
cmake --build build --target ra_tests
ctest --test-dir build --output-on-failure
# 100% tests passed, 0 tests failed out of 4  (smoke_test + 3 S07-D tests)
#   s07d_lcw_test: 14 test(s), 0 failed   s07d_xordelta_test: 6 test(s), 0 failed   s07d_misc_test: 7 test(s), 0 failed

cmake -S . -B build64 -G Ninja -DRA_TARGET_ARCH=native   # 64-bit, oracle skipped
cmake --build build64 --target s07d_lcw_test s07d_xordelta_test s07d_misc_test
ctest --test-dir build64 -R s07d                    # 3/3 passed (12 + 4 + 5 tests)
```

The new files also compile as part of their real targets with the game's flags (no errors or warnings):
`ninja -C build CMakeFiles/redalert.dir/CODE/LCW.CPP.o CMakeFiles/redalert.dir/CODE/LCWCOMP.CPP.o
CMakeFiles/win32lib.dir/WIN32LIB/WSA/XORDELTA.CPP.o CMakeFiles/win32lib.dir/WIN32LIB/MEM/MEM_COPY.CPP.o
CMakeFiles/win32lib.dir/WIN32LIB/MISC/REVERSE.CPP.o CMakeFiles/win32lib.dir/WIN32LIB/MISC/RANDOM.CPP.o`.
`nm` on `LCW.CPP.o` shows `LCW_Uncompress` (C name) and `LCW_Uncomp(void const*, void*, unsigned long)`.
MSVC was not available here; the `windows-msvc-x86` CI job will build the tests without the oracle.

## Determinism impact
**None for the simulation**, with the reasoning and evidence:

- The multiplayer map (`[MapPack]`, `[OverlayPack]`) is decoded by `LCW_Uncomp` through `LCWStraw`. Its output is unchanged
  for every stream the 1995 code decoded correctly. The rewrite differs from the old code in only two cases:
  (a) the old run fill wrote up to 4 extra bytes **after** the run; those positions are either written again by the next
  commands before anything reads them (copies only read bytes already written) or lie past the decoded length; and
  (b) a run shorter than the 1-4 bytes the old code wrote one at a time before its first aligned store, where its count
  underflowed and it wrote gigabytes (a crash).
  `lcw_game_decoder_matches_1995_code` compares the new decoder with a verbatim copy of the old one on compressor-made
  streams at all four output alignments: identical return values and bytes.
  One theoretical old behaviour is gone: `LCWStraw` decodes in place, so the old stray bytes could in principle have
  overwritten compressed input not yet read. That needs a run ending within 4 bytes of the unread input, which the
  8 KB blocks' safety margin (`BlockSize/128+1` = 65 bytes) makes very unlikely; if it ever happened, the original
  game would have decoded a corrupt map anyway.
- `LCW_Comp` (writes map packs and anything else sent through `LCWPipe`) gives byte-identical output to the asm for every
  input of 2+ bytes (oracle test), so scenario files written by the port match the original's.
- `RandNumb` is written by `CODE/INIT.CPP` and read by nothing. The game's random generators (`CODE/RANDOM.CPP`,
  `CODE/RAND.CPP`) are untouched. `Mem_Copy` is `memmove`, which is what the asm did.
- Shapes, WSA animations and VQA (via `LCW_Uncompress` and the XOR-delta functions) are presentation only.

## Problems & open questions
- `WIN32LIB/MISC/IRANDOM.CPP` (`IRandom`, never called) still references `Get_Random_Mask`, which no longer exists. A static
  link only pulls that object if something calls `IRandom`, so it does not matter today; S10 (link) may prefer to delete
  `IRANDOM.CPP` from the source list. Not done here because it is not an asm file of this package.
- `WIN32LIB/INCLUDE/MISC.H` still declares `Random`/`Get_Random_Mask`, and `WIN32LIB/INCLUDE/IFF.H` declares `LCW_Compress`
  (whose asm was deleted in S05). Declarations without definitions are harmless unless something calls them; S06a may
  tidy them.
- The asm `Reverse_*`, `Mem_Copy` etc. were `__cdecl` C functions; the ports keep C linkage. Once S06a gives the build a
  common `__cdecl` definition, the local fallback `#define __cdecl` in the new files can go.

## Next steps
- Integrator: add ADR-0019 to `docs/decisions/README.md`, and this log to `docs/logs/README.md`.
- S10: the library now expects the game executable to provide `LCW_Uncompress` (from `CODE/LCW.CPP`); a stand-alone link of
  `win32lib`/`vqa32wp` without the game objects needs `CODE/LCW.CPP` too.
- Other S07 packages can reuse the oracle technique from `tests/s07d_asm_oracle.cpp` (ADR-0019).

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-14, after S07-C. Resolved conflicts in `cmake/RASources.cmake` (both
  packages removed different `.ASM` entries from the same lists; all removals kept), `assembly-inventory.md` (kept the
  "Ported" version of each row) and the S07 brief (kept both packages' sub-items).
- On the merged tree (GCC `-m32`): `ctest` reports 8/8 test executables passing; compile progress is 21 of 295.
- Handoff: `WIN32LIB/MISC/IRANDOM.CPP` still references the deleted `Get_Random_Mask`. It's noted for S10 (drop the file if nothing calls `IRandom`).
