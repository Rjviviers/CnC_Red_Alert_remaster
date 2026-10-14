// S07-A: Buffer_To_Page (WIN32LIB/DRAWBUFF/TOPAGE.CPP, ported from TOPAGE.ASM).
// Copies a linear buffer (w bytes per row) into a viewport rectangle. Colour 0 is copied.
#include "s07a_surface.h"

static const unsigned char Z = 35;

static const unsigned char SRC[9] = {1, 2, 3,
                                     4, 0, 6,
                                     7, 8, 9};

RA_TEST(copies_buffer_into_viewport) {
    TestSurface s(6, 5, 1, 1, 4, 3, /*pitch*/ 1);
    s.fill(Z);
    RA_CHECK_EQ(0, Buffer_To_Page(1, 0, 3, 3, (void *)SRC, &s.vp));
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZ",
        "ZZ123Z",
        "ZZ4.6Z",
        "ZZ789Z",
        "ZZZZZZ");
}

// Source pixels clipped off the left/top are skipped in the buffer.
RA_TEST(clip_left_top_skips_source) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    s.fill(Z);
    Buffer_To_Page(-1, -2, 3, 3, (void *)SRC, &s.vp);
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZ",
        "Z89ZZZ",
        "ZZZZZZ",
        "ZZZZZZ",
        "ZZZZZZ");
}

RA_TEST(clip_right_bottom) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    s.fill(Z);
    Buffer_To_Page(2, 1, 3, 3, (void *)SRC, &s.vp);
    S07A_EXPECT_PICTURE(s,
        "ZZZZZZ",
        "ZZZZZZ",
        "ZZZ12Z",
        "ZZZ4.Z",
        "ZZZZZZ");
}

RA_TEST(null_buffer_or_outside_does_nothing) {
    TestSurface s(4, 3);
    s.fill(Z);
    Buffer_To_Page(0, 0, 3, 3, NULL, &s.vp);
    Buffer_To_Page(4, 0, 3, 3, (void *)SRC, &s.vp);     // x == Width
    Buffer_To_Page(0, -3, 3, 3, (void *)SRC, &s.vp);    // above (y1 == 0 is not < 0, but no rows)
    Buffer_To_Page(0, 0, 3, 0, (void *)SRC, &s.vp);     // zero height
    S07A_EXPECT_PICTURE(s, "ZZZZ", "ZZZZ", "ZZZZ");
}

RA_TEST_MAIN()
