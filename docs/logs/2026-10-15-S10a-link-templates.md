# Session log: S10a — Link: template instantiations

- **Date:** 2026-10-15
- **Session brief:** [S10](../sessions/S10-link-and-boot.md) ("Split", agent S10a)
- **Branch / PR:** `worktree-agent-a8666d73685f55939` (worktree `.claude/worktrees/agent-a8666d73685f55939`, fast-forwarded to `97674ba` first) / not pushed, merged by the orchestrator
- **Status at end:** Done (template and generic-helper part of the link)

## Goal
Resolve the template part of the 393 unresolved symbols left when linking `redalert` (GCC `-m32`): the class template
members (`VectorClass`, `DynamicVectorClass`, `CCPtr`, `TFixedIHeapClass`, `MixFileClass`), `ABS`/`MIN`/`MAX`/`Bound`, and
the enum operators. No behaviour change, no class-layout change. Platform symbols belong to S10b.

## What was done
1. Built `redalert` on a fresh `build/` and extracted the unique `undefined reference` names: **393**, identical to
   [`link-unresolved-2026-10-15.txt`](../codebase/link-unresolved-2026-10-15.txt).
2. Found two causes (details in [compat-notes.md "Templates"](../codebase/compat-notes.md#templates)):
   - Out-of-line members of class templates live in a `.CPP` (`VECTOR`, `DYNAVEC`, `CCPTR`, `HEAP`, `MIXFILE`). Watcom
     generated them there for every instantiation in view (`VECTOR.CPP`'s own comment says so; `FUNCTION.H` and
     `MIXFILE.CPP` declare dummy `extern` objects to force it). GCC/MSVC do not.
   - Bare declarations beside a template (`int MIN(int, int);`, `inline ThreatType operator |(ThreatType, ThreatType);`,
     `friend Int<P> Generate_Prime(...)`) were Watcom instantiation requests; in standard C++ they are separate functions
     that win overload resolution and are never defined.
3. Chose explicit instantiation in the defining `.CPP` plus defined stand-ins for the bare declarations (ADR-0021).
   Moving bodies into headers was rejected: `CCPtr<T>::CCPtr(T*)` and `TFixedIHeapClass<T>::Load` need complete types,
   and `CCPTR.CPP` notes the bodies were moved out of the header because of recursive includes.
4. Added `#ifndef __WATCOMC__` instantiation lists at the end of the five `.CPP`s. Every type came from the unresolved
   list; whole-class instantiation compiled for all of them, so no member-wise lists were needed.
5. `WWSTD.H` (`ABS`, `MIN`, `MAX`) and `JSHELL.H` (`Bound`): Watcom declarations kept under `__WATCOMC__`, otherwise
   `inline` definitions that call the template (`return MIN<int>(a, b);`).
6. New `CODE/ENUMOPS.H` with `RA_ENUM_POSTINC(T)` / `RA_ENUM_BITOPS(T)`; the 39 enum-operator declarations in `DEFINES.H`,
   `JSHELL.H`, `DRIVE.H`, `EDIT.H`, `GADGET.H` now use them (replaced by a script matching the exact declaration forms).
7. `INT.H`: `Int<P>` befriends the `Generate_Prime`/`Gcd` templates instead of declaring non-template friends.
   `RemainderTable` is private, so friendship is needed; the friend's `= 0` default argument is dropped (not allowed on a
   non-defining friend declaration; the only caller, `PK.CPP`, passes all three arguments).
8. **Dead end:** the first build failed in 164 files with `jshell.h: fatal error: ENUMOPS.H: No such file`. Headers
   reached through the case shim are opened from `build/case_shim/CODE/`, which only contains spellings that differ
   from the on-disk name. Switched to `#include "enumops.h"` and re-ran CMake configure. Documented in compat-notes.
9. Instantiating the members for real exposed two `-fpermissive` warnings ("no arguments to 'Length' that depend on a
   template parameter"): now `this->Length()` in `DYNAVEC.CPP` and `this->Unlink()` in `MIXFILE.CPP`. Same members as
   before (GCC already resolved them in the dependent base).
10. Relinked: **80** unresolved, none of them template symbols, none new.

## Decisions
- [ADR-0021](../decisions/0021-explicit-template-instantiation.md) (Proposed): explicit instantiation lists in the
  defining `.CPP`; bare "instantiation" declarations become definitions with the template's body.
- Small: left the unused bare declarations `int swap(int, int)` / `long swap(long, long)` (`WWSTD.H`), the `WWALLOC.H`
  operators (header unused), `class MixFileClass<CDFileClass>;` (`CCFILE.H`) and the stale `WIN32LIB/MISC/WWSTD.H`
  untouched; all listed in compat-notes.

## Files changed
| File / folder | Change |
|---------------|--------|
| `CODE/VECTOR.CPP`, `CODE/DYNAVEC.CPP`, `CODE/CCPTR.CPP`, `CODE/HEAP.CPP`, `CODE/MIXFILE.CPP` | Explicit instantiation lists at the end; `this->` for two dependent-base names |
| `CODE/ENUMOPS.H` (new) | `RA_ENUM_POSTINC` / `RA_ENUM_BITOPS` macros |
| `CODE/DEFINES.H`, `CODE/JSHELL.H`, `CODE/DRIVE.H`, `CODE/EDIT.H`, `CODE/GADGET.H` | Include `enumops.h`; enum-operator declarations → macros. `JSHELL.H`: `Bound` overloads defined |
| `WIN32LIB/INCLUDE/WWSTD.H` | `ABS`/`MIN`/`MAX` overloads defined (Watcom declarations kept under `__WATCOMC__`) |
| `CODE/INT.H` | Friend templates for `Generate_Prime` / `Gcd` |
| `docs/decisions/0021-explicit-template-instantiation.md` | New ADR |
| `docs/codebase/compat-notes.md` | New "Templates" section |
| `docs/logs/2026-10-15-S10a-link-templates.md` | This log |

## Verification
```
cmake -S . -B build -G Ninja -DRA_TARGET_ARCH=x86
cmake --build build --target redalert -- -k 0 > link.log 2>&1
grep -o "undefined reference to \`[^']*'" link.log | sed "s/undefined reference to \`//; s/'$//" | sort -u | wc -l
```
- Before: **393**. After: **80**. `comm` of the two lists: 313 removed, 0 added.
- Removed by group: `TFixedIHeapClass` 124, `VectorClass` 69, `DynamicVectorClass` 59, enum operators 31, `CCPtr` 18,
  `MixFileClass` 6, `ABS`/`MIN`×2/`MAX`/`Bound` 5, `Generate_Prime` 1.
- Remaining 80, all S10b: Win32 API 50 (file, window/message, registry, `timeSetEvent`/`timeKillEvent`, `Sleep`, …),
  asm leftovers 9 (`CPUType`, `VendorID`, `Detect_MMX_Availability`, `Force_VM_Page_In`, `MCGA_Blit`, `MCGA_BufferCopy`,
  `UnVQ_4x2`, `WaitVB`, `WaitNoVB`), movie/MPEG 7, Winsock 6, `Mono_*` 5, `DirectSoundCreate` 1, `Stop_Profiler` 1, `main` 1.
- `python3 scripts/compile_progress.py --build-dir build --no-configure` → `307 of 307 translation units compile (100.0%)`.
- `cmake --build build --target ra_tests && ctest --test-dir build` → `100% tests passed, 0 tests failed out of 32`.
- No new compiler warnings except `-Wdelete-incomplete` in `MIXFILE.CPP` (`delete` of a `void *` buffer, pre-existing
  code that is now instantiated; frees memory as under Watcom).
- MSVC not built locally; the code uses only standard explicit instantiation and friend templates.

## Determinism impact
None expected, and reasoned per change:
- Explicit instantiation compiles the **same template bodies**; it only decides which object file holds them. No class
  layout, member order or virtual table changed (`TFixedIHeapClass`, `DynamicVectorClass<TriggerClass *>` and friends
  are simulation state).
- `ABS`/`MIN`/`MAX`/`Bound` and enum operators: the set of overload candidates is unchanged (the declarations existed
  before; they now have bodies), so every call picks the same function, and the body is the template's body.
  `Bound` and `ABS` are used in simulation code (`FOOT.CPP` formation moves, `UNIT.CPP`, `FACTORY.CPP`).
- `this->Length()` / `this->Unlink()`: same member functions GCC already called.
- `Generate_Prime` is key generation (`PK.CPP`), not simulation.
To be confirmed by the S14 sync harness like everything else.

## Problems & open questions
- Adding a new header requires a CMake re-configure (case shim). CI always configures fresh.
- The `Winsock` vector instantiation (`WinsockInterfaceClass::tWinsockBufferType *`) must be removed with the Winsock
  code in S12.

## Next steps
- S10b: the remaining 80 platform symbols. S10c: boot.
- When a new template type is used, add it to the list in the defining `.CPP` (see compat-notes "Templates").

## Review & merge notes (orchestrator)

- Merged into `claude/cool-bohr-9vg7ac` on 2026-10-15 with no conflicts. Verified on the merged tree (GCC `-m32`): 307/307 compile,
  `ctest` 32/32, and linking `redalert` leaves **80** unresolved symbols (was 393), none of them templates. S10b was told to merge this
  and check for a full link.
