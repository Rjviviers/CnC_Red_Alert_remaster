// S07-A: Buffer_Draw_Line (WIN32LIB/DRAWBUFF/DRAWLINE.CPP, ported from DRAWLINE.ASM).
// Expected pixels were worked out by hand from the asm's clip loop and Bresenham loop
// (accumulator starts at major/2, minor coordinate steps when it goes negative).
#include "s07a_surface.h"

RA_TEST(horizontal_and_vertical) {
    TestSurface s(6, 5);
    Buffer_Draw_Line(&s.vp, 4, 0, 1, 0, 1);      // reversed horizontal
    Buffer_Draw_Line(&s.vp, 5, 4, 5, 1, 2);      // reversed vertical
    Buffer_Draw_Line(&s.vp, 0, 3, 0, 3, 3);      // single point
    S07A_EXPECT_PICTURE(s,
        ".1111.",
        ".....2",
        ".....2",
        "3....2",
        ".....2");
}

// Long horizontal (> 15 pixels took the asm's DWORD path).
RA_TEST(long_horizontal) {
    TestSurface s(20, 1);
    Buffer_Draw_Line(&s.vp, 1, 0, 18, 0, 7);
    S07A_EXPECT_PICTURE(s, ".777777777777777777.");
}

// (0,0)-(5,2): accum 2,0,-2->3,1,-1->4,2 => steps after x=1 and x=3.
RA_TEST(shallow_line) {
    TestSurface s(6, 3);
    Buffer_Draw_Line(&s.vp, 0, 0, 5, 2, 1);
    S07A_EXPECT_PICTURE(s,
        "11....",
        "..11..",
        "....11");
}

RA_TEST(end_point_order_does_not_matter) {
    TestSurface s(6, 3);
    Buffer_Draw_Line(&s.vp, 5, 2, 0, 0, 1);
    S07A_EXPECT_PICTURE(s, "11....", "..11..", "....11");
}

RA_TEST(steep_line) {
    TestSurface s(3, 6);
    Buffer_Draw_Line(&s.vp, 0, 0, 2, 5, 4);
    S07A_EXPECT_PICTURE(s,
        "4..",
        "4..",
        ".4.",
        ".4.",
        "..4",
        "..4");
}

RA_TEST(leftward_shallow_line) {
    TestSurface s(6, 3);
    Buffer_Draw_Line(&s.vp, 5, 0, 0, 2, 1);
    S07A_EXPECT_PICTURE(s,
        "....11",
        "..11..",
        "11....");
}

RA_TEST(diagonal_with_pitch) {
    TestSurface s(6, 5, 1, 1, 4, 4, /*pitch*/ 1);
    Buffer_Draw_Line(&s.vp, 0, 0, 3, 3, 5);
    S07A_EXPECT_PICTURE(s,
        "......",
        ".5....",
        "..5...",
        "...5..",
        "....5.");
}

RA_TEST(clipped_horizontal_fills_row) {
    TestSurface s(8, 3);
    Buffer_Draw_Line(&s.vp, -2, 1, 9, 1, 2);
    S07A_EXPECT_PICTURE(s, "........", "22222222", "........");
}

// (-2,-2)-(9,9) on 8x6: clipped right to (7,7), left to (0,0), then bottom to (5,5).
RA_TEST(clipped_diagonal) {
    TestSurface s(8, 6);
    Buffer_Draw_Line(&s.vp, -2, -2, 9, 9, 3);
    S07A_EXPECT_PICTURE(s,
        "3.......",
        ".3......",
        "..3.....",
        "...3....",
        "....3...",
        ".....3..");
}

// (-3,0)-(10,4) on 8x6. Clipping truncates: right end -> (7, 4 + 12/-13 = 4),
// left end -> (0, 0 + 12/10 = 1). Then (0,1)-(7,4) is drawn: accum 3,0,-3->4,1,-2->5,2,-1->6,3.
RA_TEST(clipped_line_uses_truncating_division) {
    TestSurface s(8, 6);
    Buffer_Draw_Line(&s.vp, -3, 0, 10, 4, 6);
    S07A_EXPECT_PICTURE(s,
        "........",
        "66......",
        "..66....",
        "....66..",
        "......66",
        "........");
}

RA_TEST(outside_lines_draw_nothing) {
    TestSurface s(8, 6);
    Buffer_Draw_Line(&s.vp, -5, -1, -1, 3, 1);   // both ends left: trivial reject
    Buffer_Draw_Line(&s.vp, 0, 6, 7, 9, 1);      // both below
    Buffer_Draw_Line(&s.vp, -4, 3, 3, -4, 1);    // crosses the corner outside: rejected after one clip
    S07A_EXPECT_PICTURE(s, "........", "........", "........", "........", "........", "........");
}

RA_TEST_MAIN()
