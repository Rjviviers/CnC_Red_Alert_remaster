// S07-A: Init_Stamps, Buffer_Draw_Stamp, Buffer_Draw_Stamp_Clip
// (WIN32LIB/DRAWBUFF/STAMP.CPP, ported from STAMP.ASM).
#include "s07a_surface.h"
#include "TILE.H"

#include <vector>

// Builds an icon set in memory: IControl_Type header, icon data, transparency flags, map.
struct IconSet {
    std::vector<unsigned char> mem;

    IconSet(int w, int h, int count, const unsigned char *pixels,
            const unsigned char *trans, const unsigned char *map, int map_size)
    {
        int const header = (int)sizeof(IControl_Type);
        int const data = w * h * count;
        mem.assign(header + data + count + map_size, 0);
        IControl_Type *ctl = (IControl_Type *)&mem[0];
        ctl->Width = (short)w;
        ctl->Height = (short)h;
        ctl->Count = (short)count;
        ctl->Icons = header;
        ctl->TransFlag = header + data;
        ctl->Map = header + data + count;
        std::memcpy(&mem[header], pixels, data);
        std::memcpy(&mem[header + data], trans, count);
        std::memcpy(&mem[header + data + count], map, map_size);
        // Init_Stamps caches the last icon set by ADDRESS (as STAMP.ASM did). A new set can be allocated
        // where a previous test's freed set lived (MSVC's heap does this), and would then be drawn with
        // the old header. Forget the cache so every test starts clean.
        LastIconset = NULL;
    }

    const void *ptr() const { return &mem[0]; }
};

// Three 4x4 icons. Icon 0 is opaque, icon 1 has transparent pixels, icon 2 is solid 9s.
static const unsigned char ICONS[3 * 16] = {
    1, 2, 3, 4,   5, 6, 7, 8,   1, 2, 3, 4,   5, 6, 7, 8,     // icon 0
    0, 3, 3, 0,   3, 0, 0, 3,   3, 0, 0, 3,   0, 3, 3, 0,     // icon 1
    9, 9, 9, 9,   9, 9, 9, 9,   9, 9, 9, 9,   9, 9, 9, 9,     // icon 2
};
static const unsigned char TRANS[3] = {0, 1, 0};
// Logical icon -> physical icon. Logical 3 maps to 7, which is out of range.
static const unsigned char MAP[4] = {2, 0, 1, 7};

static const unsigned char Z = 35;

RA_TEST(header_layout_matches_stamp_inc) {
#if defined(RA_EXPECT_32BIT)
    RA_CHECK_EQ(40, (int)sizeof(IControl_Type));    // STAMP.INC IControl_Type (32-bit longs)
#endif
}

RA_TEST(opaque_icon_through_map) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(7, 6, 1, 1, 6, 5, /*pitch*/ 1);
    s.fill(Z);
    Buffer_Draw_Stamp(&s.vp, set.ptr(), 1, 1, 0, NULL);     // logical 1 -> icon 0
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZZ",
        "ZZ1234Z",
        "ZZ5678Z",
        "ZZ1234Z",
        "ZZ5678Z",
        "ZZZZZZZ");
}

RA_TEST(transparent_icon_skips_zero) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(4, 4);
    s.fill(Z);
    Buffer_Draw_Stamp(&s.vp, set.ptr(), 2, 0, 0, NULL);     // logical 2 -> icon 1 (IsTrans)
    S07A_EXPECT_PICTURE(s,
        "Z33Z",
        "3ZZ3",
        "3ZZ3",
        "Z33Z");
}

// With a remap table, transparency is tested AFTER the remap, and the IsTrans flag is ignored:
// a source 0 remapped to a colour is drawn, a colour remapped to 0 is not.
RA_TEST(remap_then_transparency) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    unsigned char table[256];
    for (int i = 0; i < 256; i++) table[i] = (unsigned char)i;
    table[0] = 4;
    table[3] = 0;
    table[5] = 6;
    TestSurface s(4, 4);
    s.fill(Z);
    Buffer_Draw_Stamp(&s.vp, set.ptr(), 2, 0, 0, table);    // icon 1: zeros become 4, threes vanish
    S07A_EXPECT_PICTURE(s,
        "4ZZ4",
        "Z44Z",
        "Z44Z",
        "4ZZ4");
    TestSurface t(4, 4);
    Buffer_Draw_Stamp(&t.vp, set.ptr(), 1, 0, 0, table);    // icon 0 (opaque flag ignored)
    S07A_EXPECT_PICTURE(t, "12.4", "6678", "12.4", "6678");
}

RA_TEST(out_of_range_icons_draw_nothing) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(4, 4);
    Buffer_Draw_Stamp(&s.vp, set.ptr(), 3, 0, 0, NULL);     // map gives 7 >= count
    Buffer_Draw_Stamp(&s.vp, NULL, 0, 0, 0, NULL);          // no icon set
    S07A_EXPECT_PICTURE(s, "....", "....", "....", "....");
}

// The asm's map lookup is `mov bl,[map+icon]`: it replaces only the LOW BYTE of the icon number.
// Icon 256 becomes (256 & ~0xFF) | map[256] = 256, which is out of range, so nothing is drawn.
// (The map has 257 entries so that the read is valid.)
RA_TEST(map_lookup_replaces_low_byte_only) {
    unsigned char bigmap[257];
    std::memset(bigmap, 0, sizeof(bigmap));
    IconSet set(4, 4, 3, ICONS, TRANS, bigmap, 257);
    TestSurface s(4, 4);
    Buffer_Draw_Stamp(&s.vp, set.ptr(), 256, 0, 0, NULL);   // (256 & ~0xFF) | map[256] = 256
    S07A_EXPECT_PICTURE(s, "....", "....", "....", "....");
}

// Switching between icon sets re-reads the header (Init_Stamps caches the last one).
RA_TEST(switching_icon_sets) {
    IconSet a(4, 4, 3, ICONS, TRANS, MAP, 4);
    static const unsigned char SMALL[4] = {7, 8, 9, 1};
    static const unsigned char SMALL_TRANS[1] = {0};
    static const unsigned char SMALL_MAP[1] = {0};
    IconSet b(4, 1, 1, SMALL, SMALL_TRANS, SMALL_MAP, 1);   // 4x1: Height/4 = 0 rows in the fast copy
    TestSurface s(4, 4);
    Buffer_Draw_Stamp(&s.vp, a.ptr(), 0, 0, 0, NULL);       // logical 0 -> icon 2 (all 9)
    Buffer_Draw_Stamp(&s.vp, b.ptr(), 0, 0, 0, NULL);       // opaque fast path draws no rows
    S07A_EXPECT_PICTURE(s, "9999", "9999", "9999", "9999");
    RA_CHECK(LastIconset == b.ptr());
    RA_CHECK_EQ(1, IconCount);
    RA_CHECK_EQ(4, IconSize);
    unsigned char table[256];
    for (int i = 0; i < 256; i++) table[i] = (unsigned char)i;
    Buffer_Draw_Stamp(&s.vp, b.ptr(), 0, 0, 1, table);      // remap path draws every row
    S07A_EXPECT_PICTURE(s, "9999", "7891", "9999", "9999");
}

// Kept from the asm: the opaque fast path copies Width/4 DWORDs per row, and both pointers only
// advance by the bytes copied (+ the row modulo, Width + XAdd + Pitch - IconWidth, for the
// destination). For a 6-wide icon on a 6-wide page the four 4-byte rows land back to back.
// RA's icons are 24x24, where this is a normal copy.
RA_TEST(opaque_fast_path_copies_whole_dwords_only) {
    unsigned char wide[24];
    for (int i = 0; i < 24; i++) wide[i] = (unsigned char)(i + 1);
    static const unsigned char T[1] = {0};
    static const unsigned char M[1] = {0};
    IconSet set(6, 4, 1, wide, T, M, 1);
    TestSurface s(6, 4);
    s.fill(Z);
    Buffer_Draw_Stamp(&s.vp, set.ptr(), 0, 0, 0, NULL);
    S07A_EXPECT_PICTURE(s,
        "123456",
        "789ABC",
        "DEFGZZ",
        "ZZZZZZ");
}

// ---- Buffer_Draw_Stamp_Clip: window (x, y, width, height); position is window-relative. ----

RA_TEST(clip_inside_window) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(8, 6);
    s.fill(Z);
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 1, 1, 0, NULL, 2, 1, 6, 5);  // at (3,1)
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZZZ",
        "ZZZ1234Z",
        "ZZZ5678Z",
        "ZZZ1234Z",
        "ZZZ5678Z",
        "ZZZZZZZZ");
}

// Window (1,1) size 3x2: the icon at window (-1,-1) = viewport (0,0) is cut on all four sides.
RA_TEST(clip_all_sides_opaque) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(6, 5);
    s.fill(Z);
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 1, -1, -1, NULL, 1, 1, 3, 2);
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZ",
        "Z678ZZ",
        "Z234ZZ",
        "ZZZZZZ",
        "ZZZZZZ");
}

RA_TEST(clip_transparent_and_remap) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(6, 5);
    s.fill(Z);
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 2, 1, 1, NULL, 0, 0, 3, 3);   // icon 1 at (1,1), 2x2 visible
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZ",
        "ZZ3ZZZ",
        "Z3ZZZZ",
        "ZZZZZZ",
        "ZZZZZZ");
    unsigned char table[256];
    for (int i = 0; i < 256; i++) table[i] = (unsigned char)(i + 1);
    TestSurface t(6, 5);
    t.fill(Z);
    Buffer_Draw_Stamp_Clip(&t.vp, set.ptr(), 1, 2, 3, table, 0, 0, 5, 4);  // icon 0, rows 0 only
    S07A_EXPECT_PICTURE(t,
        "ZZZZZZ",
        "ZZZZZZ",
        "ZZZZZZ",
        "ZZ234Z",
        "ZZZZZZ");
}

RA_TEST(clip_rejects_outside_window) {
    IconSet set(4, 4, 3, ICONS, TRANS, MAP, 4);
    TestSurface s(8, 8);
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 1, 4, 0, NULL, 0, 0, 4, 4);   // x == window width
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 1, 0, 4, NULL, 0, 0, 4, 4);   // y == window height
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 1, -4, 0, NULL, 0, 0, 4, 4);  // ends at the left edge
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 1, 0, -4, NULL, 0, 0, 4, 4);  // ends at the top edge
    Buffer_Draw_Stamp_Clip(&s.vp, set.ptr(), 3, 0, 0, NULL, 0, 0, 4, 4);   // icon out of range
    for (size_t i = 0; i < s.buf.size(); i++) RA_CHECK_EQ(0, s.buf[i]);
}

RA_TEST_MAIN()
