# File formats: LCW and XOR delta

Written in session S07-D (2026-10-14), from the code: the 1995 C decoder `CODE/LCW.CPP`, the asm that
S07-D ported (`CODE/LCWCOMP.ASM`, `WIN32LIB/IFF/LCWUNCMP.ASM`, `WIN32LIB/WSA/XORDELTA.ASM`) and their
callers. Both are lossless, byte-oriented formats used by Westwood games of this era. The modding community
calls them "Format80" (LCW) and "Format40" (XOR delta), but those names do not appear in this code.

Implementations and tests:

| Format | Code | Tests |
|--------|------|-------|
| LCW decode | [`CODE/LCW.CPP`](../../CODE/LCW.CPP): `LCW_Uncomp` (game), `LCW_Uncompress` (library C API) | [`tests/s07d_lcw_test.cpp`](../../tests/s07d_lcw_test.cpp) |
| LCW encode | [`CODE/LCWCOMP.CPP`](../../CODE/LCWCOMP.CPP): `LCW_Comp` | same |
| XOR delta  | [`WIN32LIB/WSA/XORDELTA.CPP`](../../WIN32LIB/WSA/XORDELTA.CPP): `Apply_XOR_Delta`, `Apply_XOR_Delta_To_Page_Or_Viewport` | [`tests/s07d_xordelta_test.cpp`](../../tests/s07d_xordelta_test.cpp) |

On 32-bit x86 GCC (the `linux-gcc-x86` CI job) the tests also run the **original asm instructions**
([`tests/s07d_asm_oracle.cpp`](../../tests/s07d_asm_oracle.cpp)) and compare byte for byte.

## LCW

A stream of commands. Each starts with one command byte `n`; multi-byte numbers are 16-bit little endian.
"Output" means the bytes decoded so far, starting at the beginning of the destination buffer.

| `n` (bits) | Bytes that follow | Meaning |
|------------|-------------------|---------|
| `0cccpppp` | `p_low` | **Short copy**: copy `c+3` (3-10) bytes from `p` (12 bits, 0-4095) bytes back in the output |
| `10cccccc`, c > 0 | `c` bytes | **Literal**: copy the next `c` (1-63) bytes of the stream |
| `10000000` | none | **End of data** |
| `11cccccc`, c < 62 | `w` | **Medium copy**: copy `c+3` (3-64) bytes from output offset `w` (absolute, from the start) |
| `11111110` (`FE`) | `w`, `b` | **Run**: write byte `b`, `w` times |
| `11111111` (`FF`) | `w1`, `w2` | **Long copy**: copy `w1` bytes from output offset `w2` (absolute) |

Copies read the output one byte at a time, forwards, so a copy may overlap the bytes it is writing: copying
6 bytes from 2 back after "AB" gives "ABABAB". Every decoder in the codebase relies on this.

Because medium and long copies use 16-bit absolute offsets, one LCW stream covers at most 64 KB of output.
Longer data is split into blocks (see "Where it is used").

### The two decoders (one implementation)

`CODE/LCW.CPP` holds the only decoder (`Decode`); both entry points call it:

- `LCW_Uncomp(src, dst, length = 0)` — the game's. It ignores `length` and stops only at the end code.
- `extern "C" LCW_Uncompress(src, dst, length)` — the library's (`WIN32LIB/INCLUDE/IFF.H`, `WINVQ` `COMPRESS.H`;
  callers: keyframe shapes `CODE/2KEYFRAM.CPP`, `WIN32LIB/WSA/WSA.CPP`, `WIN32LIB/IFF/LOAD.CPP`, the VQA player).
  Like the asm it replaces, it never writes more than `length` bytes: every command is clipped to the space
  left, and it stops when the output is full even without an end code.

Several callers decode **in place**: the compressed data sits at the end of the output buffer
(`CODE/LCWSTRAW.CPP`, `WIN32LIB/WSA/WSA.CPP`, the VQA loader). That only works if the decoder never writes
ahead of its current output position. The 1995 C code could, by up to 4 bytes after a run; S07-D fixed that.

Both return the number of bytes written.

### The compressor (`LCW_Comp`)

Greedy, exactly as the 1990 asm (the output is byte-identical, which matters if anyone compares compressed
files or save data):

1. The first input byte is always a 1-byte literal (`81 b`).
2. At each position, first check for a **run**: only if the byte 64 positions ahead equals the current one,
   count equal bytes. A run of 65 or more becomes `FE w b`. Quirk: if the run reaches the end of the input,
   the asm counts one byte less, so the last input byte is never part of a run.
3. Otherwise search all earlier positions, front to back, for the **longest match** (it may overlap the
   current position). On a tie the **later** match wins (the asm uses `>=`).
4. Pick the command: length 1-2 → literal byte (appended to the current literal command until it holds
   63 bytes); length 3-10 and distance ≤ 4095 → short copy; length ≤ 64 → medium copy; else long copy.
5. Finish with the end code `80`.

Worst-case output: `length + length/63 + 3` bytes (incompressible data). Lengths and offsets are written as
16-bit words, so input blocks must be ≤ 64 KB. For inputs of 0 and 1 bytes the asm read past the input;
the port writes `80` and `81 b 80`.

### Where it is used

- **Map data in scenarios and saves**: `[MapPack]` and `[OverlayPack]` INI sections are Base64 text
  (`INIClass::Get_UUBlock`, `CODE/INI.CPP`) over an `LCWStraw`/`LCWPipe` block stream (`CODE/MAP.CPP`
  `Read_Binary`/`Write_Binary`, `CODE/OVERLAY.CPP`). Each block is a 4-byte header
  `{ uint16 CompCount; uint16 UncompCount; }` followed by `CompCount` bytes of LCW; blocks hold up to 8 KB
  (`LCWStraw`'s default `blocksize`). These decode with `LCW_Uncomp`, so **this is on the multiplayer
  simulation's input path**: every client must decode the same map. See the S07-D log for why the bytes
  are unchanged.
- **Keyframe shapes** (`CODE/2KEYFRAM.CPP`): key frames are LCW; other frames are LCW-compressed XOR deltas.
- **WSA animations** (`WIN32LIB/WSA/WSA.CPP`): each frame is an LCW-compressed XOR delta.
- **IFF/CPS pictures** (`WIN32LIB/IFF/LOAD.CPP`) and **VQA movies** (`WINVQ/VQA32`; movies are stubbed, ADR-0005).

## XOR delta

Describes the change from one frame to the next as bytes to XOR onto the previous frame (or, for the first
frame, to store). Command byte `n`, 16-bit little-endian words:

| `n` | Bytes that follow | Meaning |
|-----|-------------------|---------|
| `00` | `c`, `v` | **Short run**: `c` (1-255) pixels get `^= v` |
| `01`-`7F` | `n` bytes | **Short dump**: the next `n` pixels get `^=` the next `n` bytes |
| `81`-`FF` | none | **Short skip**: skip `n & 0x7F` (1-127) pixels |
| `80` | `w` | Long command, by the 16-bit word `w`: |
| | `w = 0` | **Stop** |
| | `w = 0x0001-0x7FFF` | **Long skip** of `w` pixels |
| | `w = 10cc cccc cccc cccc` | **Long dump**: `w & 0x3FFF` bytes follow |
| | `w = 11cc cccc cccc cccc`, `v` | **Long run**: `w & 0x3FFF` pixels get `^= v` |

A count of 0 in a run or dump made the asm loop 2^32 times (it decrements before testing); Westwood's
encoder never writes one, and the port treats it as "nothing".

`Apply_XOR_Delta(target, delta)` applies the commands to a linear buffer (the frame is stored contiguously).

`Apply_XOR_Delta_To_Page_Or_Viewport(target, delta, width, nextrow, copy)` applies them to a `width`-pixel-wide
rectangle inside a bigger buffer. A column counter wraps at `width`; at each wrap the pointer goes back to the
start of the row and adds `nextrow`. So `nextrow` is the buffer's **pitch** (start of one row to start of the
next), whatever the asm's comment says ("page width - anim width"); `WSA.CPP` passes the full buffer width.
Skips can cross several rows. With `copy` = `DO_COPY` (1) the bytes are stored instead of XORed; WSA uses that
for a first frame that is not itself a delta. `width` must be > 0 (the asm looped forever otherwise; the port
returns).
