// S07-A: Buffer_Fill_Rect (WIN32LIB/DRAWBUFF/FILLRECT.CPP, ported from FILLRECT.ASM).
// Corners are inclusive and may come in any order; the rectangle is clipped to the viewport.
#include "s07a_surface.h"

RA_TEST(inclusive_corners) {
    TestSurface s(6, 4);
    Buffer_Fill_Rect(&s.vp, 1, 1, 3, 2, 4);
    S07A_EXPECT_PICTURE(s,
        "......",
        ".444..",
        ".444..",
        "......");
}

RA_TEST(corners_in_any_order) {
    TestSurface s(6, 4);
    Buffer_Fill_Rect(&s.vp, 3, 2, 1, 1, 4);
    TestSurface t(6, 4);
    Buffer_Fill_Rect(&t.vp, 1, 2, 3, 1, 4);
    S07A_EXPECT_PICTURE(s, "......", ".444..", ".444..", "......");
    S07A_EXPECT_PICTURE(t, "......", ".444..", ".444..", "......");
}

RA_TEST(single_pixel) {
    TestSurface s(3, 3);
    Buffer_Fill_Rect(&s.vp, 1, 1, 1, 1, 2);
    S07A_EXPECT_PICTURE(s, "...", ".2.", "...");
}

// Viewport 4x3 at (1,1) inside a 7x5 buffer, with 1 byte of Pitch.
RA_TEST(clipped_on_all_sides_stays_in_viewport) {
    TestSurface s(7, 5, 1, 1, 4, 3, /*pitch*/ 1);
    Buffer_Fill_Rect(&s.vp, -5, -5, 50, 50, 3);
    S07A_EXPECT_PICTURE(s,
        ".......",
        ".3333..",
        ".3333..",
        ".3333..",
        ".......");
}

RA_TEST(clipped_left_top) {
    TestSurface s(5, 4);
    Buffer_Fill_Rect(&s.vp, -2, -1, 1, 1, 6);
    S07A_EXPECT_PICTURE(s,
        "66...",
        "66...",
        ".....",
        ".....");
}

RA_TEST(clipped_right_bottom) {
    TestSurface s(5, 4);
    Buffer_Fill_Rect(&s.vp, 3, 2, 9, 9, 6);
    S07A_EXPECT_PICTURE(s,
        ".....",
        ".....",
        "...66",
        "...66");
}

RA_TEST(entirely_outside_draws_nothing) {
    TestSurface s(5, 4);
    Buffer_Fill_Rect(&s.vp, 5, 0, 7, 2, 1);      // starts at x == Width
    Buffer_Fill_Rect(&s.vp, 0, 4, 2, 6, 1);      // starts at y == Height
    Buffer_Fill_Rect(&s.vp, -6, 0, -1, 2, 1);    // ends left of the viewport
    Buffer_Fill_Rect(&s.vp, 0, -6, 2, -1, 1);    // ends above the viewport
    S07A_EXPECT_PICTURE(s, ".....", ".....", ".....", ".....");
}

// The asm took the coordinates as WORDs (movsx): only the low 16 bits count.
RA_TEST(coordinates_are_truncated_to_16_bits) {
    TestSurface s(5, 3);
    Buffer_Fill_Rect(&s.vp, 0x10001, 0x20000, 0x30002, 0x10001, 8);   // = (1,0)-(2,1)
    Buffer_Fill_Rect(&s.vp, 0xFFFF, 2, 0, 2, 9);                      // 0xFFFF = -1 -> (0,2)-(0,2)
    S07A_EXPECT_PICTURE(s,
        ".88..",
        ".88..",
        "9....");
}

RA_TEST(full_rows_without_padding) {
    // XAdd + Pitch == 0: the asm filled all rows as one run; same bytes.
    TestSurface s(4, 3);
    Buffer_Fill_Rect(&s.vp, 0, 1, 3, 2, 5);
    S07A_EXPECT_PICTURE(s, "....", "5555", "5555");
}

RA_TEST_MAIN()
