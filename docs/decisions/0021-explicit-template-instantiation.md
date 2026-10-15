# ADR-0021: Explicit template instantiation in the defining `.CPP`, defined stand-ins for Watcom "instantiation declarations"

- **Status:** Proposed
- **Date:** 2026-10-15
- **Session:** S10a

## Context
After S06 every source compiles, but linking `redalert` left 393 unresolved symbols; about 300 of them came from
templates ([full list](../codebase/link-unresolved-2026-10-15.txt)). Two Watcom C++ 10.6 behaviours are behind them:

1. **Template members defined in a `.CPP`.** `VectorClass<T>` (`VECTOR.CPP`), `DynamicVectorClass<T>` (`DYNAVEC.CPP`),
   `CCPtr<T>` (`CCPTR.CPP`), `TFixedIHeapClass<T>` (`HEAP.CPP`) and `MixFileClass<T>` (`MIXFILE.CPP`) keep their
   out-of-line members in a `.CPP`. Watcom generated them there for every instantiation that module could see
   (`VECTOR.CPP`: "can be located here ONLY if all the instantiations are declared in a header file this module
   includes"; `FUNCTION.H` and `MIXFILE.CPP` even declare dummy objects such as `extern MFCD temp;` to make sure).
   GCC and MSVC only generate a template member where its body is visible, so every other module's use is undefined.
2. **Bare declarations used as "instantiate this template for this signature".** `int MIN(int, int);` next to
   `template<class T> T MIN(T, T)` (`WWSTD.H`; same for `ABS`, `MAX`, `Bound`), `inline ThreatType operator |(ThreatType,
   ThreatType);` next to the `JSHELL.H` enum-operator templates (39 such declarations), and `friend Int<P>
   Generate_Prime(...)` inside `Int<P>` (`INT.H`). Standard C++ reads each as a separate non-template function that
   overload resolution *prefers* over the template and that is never defined.

Constraints: no behaviour change and no class-layout change (heaps of units and vectors of triggers are simulation
state), and no ODR hazards.

## Options considered
1. **Move the member bodies into the headers (or `.inl` files they include).** Matches modern practice and needs no
   type lists. But the bodies need complete types: `CCPtr<T>::CCPtr(T*)` reads `ptr->ID`, `TFixedIHeapClass<T>::Load`
   placement-constructs `T`, and `CCPTR.CPP` says the bodies were moved out of the header precisely because the headers
   are mutually recursive. Every module that names `CCPtr<HouseClass>` would then have to see `HOUSE.H` complete, which
   is not true of the include graph today. It also makes every one of ~180 game modules recompile these bodies, and
   puts a copy of each instantiation in every object file (COMDAT-folded, so ODR-safe, but slower).
2. **Explicit instantiation definitions at the end of the defining `.CPP`** (`template class VectorClass<CellClass>;`
   and so on), one list per file. This is what Watcom effectively did: the bodies are compiled once, in the same
   module, with the same headers in view. Each instantiation exists in exactly one object file, so there is nothing
   for the ODR to compare. Cost: the type lists must be maintained. A missing type is a link error, never a silent
   change.
3. **Remove the bare declarations** (for `MIN`, enum operators, …) so calls fall through to the templates. Changes the
   overload candidate set: a call that converted its arguments to `int MIN(int, int)` would now fail to deduce, or
   pick a different overload, and where `JSHELL.H` is not in view the enum operators would silently become the
   built-in `int` operators.
4. **Define the declared functions** with a body identical to the template's. The candidate set, and therefore
   every overload choice, is unchanged; only the missing definition is supplied.

## Decision
Option 2 for template members, and option 4 for the bare declarations:

- `VECTOR.CPP`, `DYNAVEC.CPP`, `CCPTR.CPP`, `HEAP.CPP` and `MIXFILE.CPP` end with an `#ifndef __WATCOMC__` block of
  `template class X<T>;` for every type the game uses. Whole-class instantiation is the default; if a member does not
  compile for some `T` (Watcom only generated what was used), instantiate members one by one for that `T` and say why.
- `ABS`/`MIN`/`MAX` (`WIN32LIB/INCLUDE/WWSTD.H`) and `Bound` (`JSHELL.H`) keep the declarations under `__WATCOMC__`;
  elsewhere they become `inline` functions that call the template (`return MIN<int>(a, b);`).
- Enum operators: the declarations are replaced by `RA_ENUM_POSTINC(T)` / `RA_ENUM_BITOPS(T)` from the new
  `CODE/ENUMOPS.H`. Under Watcom they expand to the original declaration; elsewhere to an `inline` definition whose
  body is copied verbatim from the `JSHELL.H` template (copied, not forwarded, because `DEFINES.H` can be compiled
  without `JSHELL.H`).
- `Int<P>` befriends the `Generate_Prime` and `Gcd` templates (`template<class T> friend T Generate_Prime(...)`)
  instead of declaring non-template friends.

## Consequences
- The template part of the link (313 symbols) resolves; 393 → 80 unresolved, all platform symbols (S10b).
- **New template uses need a line in the type list** of the defining `.CPP`. The link error names the exact
  instantiation to add. Removing a type (for example `WinsockInterfaceClass::tWinsockBufferType *` once S12 replaces
  the Winsock transport) means removing its line.
- New per-enum operators should use the macros, not a bare declaration.
- No simulation change: every function body is the one Watcom compiled, and no class layout changed. The only
  source edits inside bodies are `this->Length()` / `this->Unlink()` (names from a dependent base; GCC's
  `-fpermissive` already resolved them to the same members).
- `CODE/ENUMOPS.H` is included as `"enumops.h"` (lower case, like the rest of the code) so the case shim (ADR-0008)
  links it; an existing build directory needs a CMake re-configure to pick it up.
