// S07-A: Buffer_Remap (WIN32LIB/DRAWBUFF/REMAP.CPP, ported from REMAP.ASM).
// (x, y, width, height) form; x + width and y + height are exclusive.
#include "s07a_surface.h"

// Table: 0 -> 9, n -> n + 1 (so 0 pixels are remapped too: no transparency here).
static void make_table(unsigned char *table)
{
    for (int i = 0; i < 256; i++) table[i] = (unsigned char)(i + 1);
    table[0] = 9;
}

static void load_ramp(TestSurface &s)
{
    S07A_LOAD_PICTURE(s,
        "12341",
        "23412",
        ".....",
        "34123");
}

RA_TEST(remaps_rectangle) {
    unsigned char table[256];
    make_table(table);
    TestSurface s(5, 4);
    load_ramp(s);
    Buffer_Remap(&s.vp, 1, 1, 3, 2, table);
    S07A_EXPECT_PICTURE(s,
        "12341",
        "24522",
        ".999.",
        "34123");
}

RA_TEST(null_table_does_nothing) {
    TestSurface s(5, 4);
    load_ramp(s);
    Buffer_Remap(&s.vp, 0, 0, 5, 4, NULL);
    S07A_EXPECT_PICTURE(s, "12341", "23412", ".....", "34123");
}

// Viewport 3x2 at (1,1) with 1 byte of Pitch; rectangle sticks out on every side.
RA_TEST(clipped_to_viewport) {
    unsigned char table[256];
    make_table(table);
    TestSurface s(5, 4, 1, 1, 3, 2, /*pitch*/ 1);
    load_ramp(s);
    Buffer_Remap(&s.vp, -3, -3, 20, 20, table);
    S07A_EXPECT_PICTURE(s,
        "12341",
        "24522",
        ".999.",
        "34123");
}

RA_TEST(clip_left_only) {
    unsigned char table[256];
    make_table(table);
    TestSurface s(5, 4);
    load_ramp(s);
    Buffer_Remap(&s.vp, -2, 0, 4, 1, table);     // covers x = -2..1 -> 0..1
    S07A_EXPECT_PICTURE(s, "23341", "23412", ".....", "34123");
}

RA_TEST(rejected_or_empty) {
    unsigned char table[256];
    make_table(table);
    TestSurface s(5, 4);
    load_ramp(s);
    Buffer_Remap(&s.vp, 6, 0, 2, 2, table);      // right of the viewport
    Buffer_Remap(&s.vp, 0, -5, 2, 2, table);     // above
    Buffer_Remap(&s.vp, 5, 0, 3, 1, table);      // x0 == Width: clipped to zero width
    Buffer_Remap(&s.vp, 1, 1, 0, 2, table);      // zero width
    Buffer_Remap(&s.vp, 1, 1, 2, 0, table);      // zero height
    Buffer_Remap(&s.vp, 3, 1, -2, 2, table);     // negative width
    S07A_EXPECT_PICTURE(s, "12341", "23412", ".....", "34123");
}

RA_TEST_MAIN()
