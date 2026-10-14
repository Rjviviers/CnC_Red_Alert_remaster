// S07-A: Buffer_Put_Pixel (WIN32LIB/DRAWBUFF/PUTPIX.CPP, ported from PUTPIX.ASM).
#include "s07a_surface.h"

RA_TEST(writes_inside_viewport_with_xadd_and_pitch) {
    TestSurface s(6, 4, 1, 1, 4, 3, /*pitch*/ 1);
    Buffer_Put_Pixel(&s.vp, 0, 0, 1);
    Buffer_Put_Pixel(&s.vp, 3, 0, 2);
    Buffer_Put_Pixel(&s.vp, 1, 1, 3);
    Buffer_Put_Pixel(&s.vp, 3, 2, 4);
    S07A_EXPECT_PICTURE(s,
        "......",
        ".1..2.",
        "..3...",
        "....4.");
}

RA_TEST(ignores_out_of_range_including_negative) {
    TestSurface s(6, 4, 1, 1, 4, 3);
    Buffer_Put_Pixel(&s.vp, 4, 0, 9);
    Buffer_Put_Pixel(&s.vp, -1, 0, 9);
    Buffer_Put_Pixel(&s.vp, 0, 3, 9);
    Buffer_Put_Pixel(&s.vp, 0, -1, 9);
    Buffer_Put_Pixel(&s.vp, -100000, -100000, 9);
    S07A_EXPECT_PICTURE(s,
        "......",
        "......",
        "......",
        "......");
}

RA_TEST(writes_full_byte_value) {
    TestSurface s(1, 1);
    Buffer_Put_Pixel(&s.vp, 0, 0, 0xFE);
    RA_CHECK_EQ(0xFE, s.buf[0]);
}

RA_TEST_MAIN()
