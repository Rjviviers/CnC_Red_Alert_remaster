// S07-A: Buffer_Clear (WIN32LIB/DRAWBUFF/CLEAR.CPP, ported from CLEAR.ASM).
#include "s07a_surface.h"

RA_TEST(clears_only_the_viewport) {
    TestSurface s(6, 4, 1, 1, 3, 2, /*pitch*/ 1);
    Buffer_Clear(&s.vp, 5);
    S07A_EXPECT_PICTURE(s,
        "......",
        ".555..",
        ".555..",
        "......");
}

// Widths of 14 and more took the asm's DWORD-aligned path; the result must be the same.
RA_TEST(wide_viewport_unaligned_start) {
    TestSurface s(20, 3, 3, 1, 15, 1);
    Buffer_Clear(&s.vp, 7);
    S07A_EXPECT_PICTURE(s,
        "....................",
        "...777777777777777..",
        "....................");
}

RA_TEST(whole_buffer) {
    TestSurface s(3, 2);
    s.fill(9);
    Buffer_Clear(&s.vp, 0);
    S07A_EXPECT_PICTURE(s,
        "...",
        "...");
}

// The asm would loop ~4 billion rows for Height 0; the port draws nothing.
RA_TEST(zero_height_or_width_draws_nothing) {
    TestSurface s(4, 2, 0, 0, 4, 0);
    Buffer_Clear(&s.vp, 3);
    TestSurface t(4, 2, 0, 0, 0, 2);
    Buffer_Clear(&t.vp, 3);
    S07A_EXPECT_PICTURE(s, "....", "....");
    S07A_EXPECT_PICTURE(t, "....", "....");
}

RA_TEST_MAIN()
