// S07-A: Buffer_Get_Pixel (WIN32LIB/DRAWBUFF/GETPIX.CPP, ported from GETPIX.ASM).
#include "s07a_surface.h"

// 6x4 buffer filled with its own offsets; a 4x3 viewport at (1,1) with 1 byte of Pitch.
static void make_numbered(TestSurface &s)
{
    for (size_t i = 0; i < s.buf.size(); i++) s.buf[i] = (unsigned char)i;
}

RA_TEST(reads_inside_viewport_with_xadd_and_pitch) {
    TestSurface s(6, 4, 1, 1, 4, 3, /*pitch*/ 1);
    make_numbered(s);
    RA_CHECK_EQ(7, Buffer_Get_Pixel(&s.vp, 0, 0));     // buffer (1,1)
    RA_CHECK_EQ(10, Buffer_Get_Pixel(&s.vp, 3, 0));    // buffer (4,1)
    RA_CHECK_EQ(13, Buffer_Get_Pixel(&s.vp, 0, 1));    // row stride = 4 + XAdd 1 + Pitch 1 = 6
    RA_CHECK_EQ(22, Buffer_Get_Pixel(&s.vp, 3, 2));    // buffer (4,3)
}

RA_TEST(returns_value_as_unsigned_byte) {
    TestSurface s(2, 1);
    s.buf[1] = 0xF0;
    RA_CHECK_EQ(0xF0, Buffer_Get_Pixel(&s.vp, 1, 0));
}

// Out of range: the asm returned whatever was in EAX, which is the rejected coordinate.
// Negative coordinates are out of range because the bounds test is unsigned.
RA_TEST(out_of_range_returns_rejected_coordinate) {
    TestSurface s(6, 4, 1, 1, 4, 3);
    make_numbered(s);
    RA_CHECK_EQ(4, Buffer_Get_Pixel(&s.vp, 4, 0));     // x == Width
    RA_CHECK_EQ(-1, Buffer_Get_Pixel(&s.vp, -1, 0));
    RA_CHECK_EQ(3, Buffer_Get_Pixel(&s.vp, 0, 3));     // y == Height
    RA_CHECK_EQ(-2, Buffer_Get_Pixel(&s.vp, 2, -2));
    RA_CHECK_EQ(9, Buffer_Get_Pixel(&s.vp, 9, 9));     // x is tested first
}

RA_TEST_MAIN()
