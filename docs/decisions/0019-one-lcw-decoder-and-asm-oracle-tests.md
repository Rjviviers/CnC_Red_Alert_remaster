# ADR-0019: One LCW decoder; check asm ports against the original instructions in tests

- **Status:** Proposed
- **Date:** 2026-10-14
- **Session:** S07-D

## Context
S07-D replaced the data-codec assembly with C++ (see [assembly-inventory.md](../codebase/assembly-inventory.md),
[file-formats.md](../codebase/file-formats.md)). Two questions came up that other S07 packages will meet too.

1. **LCW had two decoders.** The game decoded map data with the C++ `LCW_Uncomp` (`CODE/LCW.CPP`); the library,
   keyframe shapes, WSA and VQA used the asm `LCW_Uncompress` (`WIN32LIB/IFF/LCWUNCMP.ASM`), which never writes more
   than the `length` it is given. ADR-0010 says to adopt `LCW.CPP`, but `LCW.CPP` ignores `length` and its run
   command could write up to 4 bytes past the run (harmful for callers that decode in place) and did not compile
   for 64-bit targets (`(unsigned)pointer`).
2. **How to prove "bit-exact".** Hand-made vectors and round trips cannot show that a ported *compressor* makes the
   same choices as the asm (any valid encoding round-trips). Comparing against the original needs the original
   code to run, but no assembler is planned for the build (ADR-0003, ADR-0010).

## Options considered
1. **Keep both decoders** (port the asm separately): two implementations of one format that can drift; the game
   and the library could decode the same data differently.
2. **One decoder in `LCW.CPP` with two entry points**: `LCW_Uncomp` (unbounded, as the game always used it) and
   `extern "C" LCW_Uncompress` (bounded, as the asm). Fix the run to write exactly its length.
3. **Golden vectors only** for the compressor: cheap, but the vectors would come from the port itself.
4. **Test-only asm oracle**: transcribe the original asm, instruction for instruction, into GNU assembler (Intel
   syntax) inside a test source, and compare the port against it on many generated inputs. GCC/Clang assemble it
   themselves (no TASM, nothing in the game build). It only runs on 32-bit x86 GCC/Clang, which the
   `linux-gcc-x86` CI job is.

## Decision
Option 2 and option 4. `CODE/LCW.CPP` is the only LCW decoder in the build; the game's `LCW_Uncomp` keeps its
behaviour (output proved unchanged against a verbatim copy of the 1995 code in `tests/s07d_lcw_test.cpp`). Ports
whose exactness matters are checked against a transcription of the original asm in `tests/` (S07-D:
`tests/s07d_asm_oracle.cpp`), guarded so other compilers simply skip those checks.

## Consequences
- Map data, shapes, WSA and VQA all decode through the same code; the library entry point is as safe as the asm.
- The oracle is test-only code. It adds assembly to the repository again, but nothing in the game links it, and
  it is needed only while someone wants to re-verify the port. It can be deleted once the ports are trusted.
- Other S07 packages can use the same technique (e.g. S07-B's shape renderer), with golden images for the parts an
  oracle cannot cover (code that reads `GraphicViewPortClass` at fixed offsets needs matching test structs).
- On MSVC the oracle checks are skipped; the hand-derived vectors and round trips still run there.
