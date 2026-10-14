// S07-A: Buffer_To_Buffer (WIN32LIB/DRAWBUFF/TOBUFF.CPP, ported from TOBUFF.ASM).
// Copies a viewport rectangle into a linear buffer laid out with the REQUESTED width per row.
#include "s07a_surface.h"

static const unsigned char G = 0xEE;   // guard value for untouched buffer bytes

static void load_page(TestSurface &s)
{
    S07A_LOAD_PICTURE(s,
        "......",
        ".1234.",
        ".5678.",
        ".9ABC.",
        "......");
}

RA_TEST(copies_rectangle) {
    TestSurface s(6, 5, 1, 1, 4, 3, /*pitch*/ 1);
    load_page(s);
    unsigned char out[8];
    std::memset(out, G, sizeof(out));
    RA_CHECK_EQ(0, Buffer_To_Buffer(&s.vp, 1, 1, 2, 2, out, sizeof(out)));
    const unsigned char expected[8] = {6, 7, 10, 11, G, G, G, G};
    RA_CHECK_MEM_EQ(expected, out, sizeof(out));
}

// Pixels clipped off the left/top are skipped in the buffer too (left untouched).
RA_TEST(clip_left_top_keeps_buffer_layout) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    load_page(s);
    unsigned char out[9];
    std::memset(out, G, sizeof(out));
    Buffer_To_Buffer(&s.vp, -1, -1, 3, 3, out, sizeof(out));
    const unsigned char expected[9] = {G, G, G,
                                       G, 1, 2,
                                       G, 5, 6};
    RA_CHECK_MEM_EQ(expected, out, sizeof(out));
}

RA_TEST(clip_right_bottom) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    load_page(s);
    unsigned char out[9];
    std::memset(out, G, sizeof(out));
    Buffer_To_Buffer(&s.vp, 2, 1, 3, 3, out, sizeof(out));
    const unsigned char expected[9] = {7, 8, G,
                                       11, 12, G,
                                       G, G, G};
    RA_CHECK_MEM_EQ(expected, out, sizeof(out));
}

// Refused when w * (clipped height) > size.
RA_TEST(too_small_buffer_is_refused) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    load_page(s);
    unsigned char out[6];
    std::memset(out, G, sizeof(out));
    Buffer_To_Buffer(&s.vp, 0, 0, 2, 3, out, 5);
    const unsigned char untouched[6] = {G, G, G, G, G, G};
    RA_CHECK_MEM_EQ(untouched, out, sizeof(out));
    Buffer_To_Buffer(&s.vp, 0, 0, 2, 3, out, 6);   // exactly big enough
    const unsigned char copied[6] = {1, 2, 5, 6, 9, 10};
    RA_CHECK_MEM_EQ(copied, out, sizeof(out));
}

// Kept from the asm: the size test ignores the left/top skip. Here w * clipped height = 2 * 2 = 4
// passes size 4, although the copy writes up to offset 2 + 2 + 1 = 5.
RA_TEST(size_test_ignores_clip_skip) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    load_page(s);
    unsigned char out[8];
    std::memset(out, G, sizeof(out));
    Buffer_To_Buffer(&s.vp, 0, -1, 2, 3, out, 4);
    const unsigned char expected[8] = {G, G, 1, 2, 5, 6, G, G};
    RA_CHECK_MEM_EQ(expected, out, sizeof(out));
}

RA_TEST(outside_or_empty_copies_nothing) {
    TestSurface s(6, 5, 1, 1, 4, 3);
    load_page(s);
    unsigned char out[4];
    std::memset(out, G, sizeof(out));
    Buffer_To_Buffer(&s.vp, 4, 0, 2, 2, out, sizeof(out));    // x == Width: clipped to nothing
    Buffer_To_Buffer(&s.vp, -3, 0, 2, 2, out, sizeof(out));   // left of the viewport
    Buffer_To_Buffer(&s.vp, 0, 0, 0, 2, out, sizeof(out));    // zero width
    const unsigned char untouched[4] = {G, G, G, G};
    RA_CHECK_MEM_EQ(untouched, out, sizeof(out));
}

RA_TEST_MAIN()
