# ADR-0016: Drop the DirectDraw video-memory icon cache

- **Status:** Proposed (carries out the removal planned in the S02 inventory, row 21, and the S07/S08 briefs)
- **Date:** 2026-10-14
- **Session:** S07-A

## Context
Red Alert 95 copied terrain icons (template tiles) into DirectDraw video-memory surfaces so the hardware blitter
could draw them: `WIN32LIB/DRAWBUFF/ICONCACH.CPP` manages the surfaces, `STMPCACH.ASM` looked icons up and copied
them, and `GraphicViewPortClass::Draw_Stamp` (`GBUFFER.H`) chose between a cached hardware blit and the software
`Buffer_Draw_Stamp_Clip`. The port draws into 8-bit system-memory buffers and presents them through SDL (ADR-0004);
there is no video memory to cache into. The cache only ever changed speed, never the image: an icon that could not
be cached was drawn by the software routine.

## Options considered
1. **Port `STMPCACH.ASM` faithfully**: keeps dead DirectDraw machinery alive, and depends on register-convention
   callbacks into `ICONCACH.CPP` (`Get_Free_Cache_Slot_`, `Cache_New_Icon_`) that S08 will delete anyway.
2. **Remove the cache**: every icon goes through `Buffer_Draw_Stamp_Clip`, which is already ported and tested.

## Decision
Remove the cache. In S07-A, `STMPCACH.ASM` is replaced by a small `STMPCACH.CPP` whose `Is_Icon_Cached` always
returns -1 ("not cached") and whose `Cache_Copy_Icon` is a plain 24x24 copy, so the remaining callers still link.
S08, which owns the DirectDraw code, deletes the rest.

## Consequences
- Rendering output is unchanged; drawing terrain costs a software copy per icon, which is trivial on modern CPUs.
- Follow-up for S08: delete `ICONCACH.CPP`/`.H` (both copies), the cache branch of `Draw_Stamp` in `GBUFFER.H`,
  `IconCacheAllowed` (`BUFFGLBL.CPP`), the `Register_Icon_Set` call in `CODE/CDATA.CPP:3136`, then `STMPCACH.CPP`
  and its test. See [surface-primitives.md](../codebase/surface-primitives.md#for-s08-platform-layer).
- No effect on the simulation (drawing only).
