// S07-A: Linear_Blit_To_Linear (WIN32LIB/DRAWBUFF/BITBLIT.CPP, ported from BITBLIT.ASM).
// Destinations are pre-filled with 'Z' (35) so that copied zeros are visible.
#include "s07a_surface.h"

static void load_src(TestSurface &s)
{
    S07A_LOAD_PICTURE(s,
        "1234",
        "5.67",
        "89AB");
}

static const unsigned char Z = 35;

RA_TEST(copies_rectangle_between_viewports) {
    TestSurface src(4, 3);
    load_src(src);
    TestSurface dst(6, 5, 1, 1, 4, 3, /*pitch*/ 1);
    dst.fill(Z);
    RA_CHECK_EQ(0, Linear_Blit_To_Linear(&src.vp, &dst.vp, 1, 0, 0, 1, 3, 2, 0));
    S07A_EXPECT_PICTURE(dst,
        "ZZZZZZ",
        "ZZZZZZ",
        "Z234ZZ",
        "Z.67ZZ",
        "ZZZZZZ");
}

RA_TEST(transparent_skips_colour_zero) {
    TestSurface src(4, 3);
    load_src(src);
    TestSurface dst(6, 5, 1, 1, 4, 3, 1);
    dst.fill(Z);
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 1, 0, 0, 1, 3, 2, 1);
    S07A_EXPECT_PICTURE(dst,
        "ZZZZZZ",
        "ZZZZZZ",
        "Z234ZZ",
        "ZZ67ZZ",
        "ZZZZZZ");
}

// Only bit 0 of `trans` counts (`test [trans],1`).
RA_TEST(trans_flag_uses_bit_zero_only) {
    TestSurface src(4, 3);
    load_src(src);
    TestSurface dst(4, 3);
    dst.fill(Z);
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 4, 3, 2);
    S07A_EXPECT_PICTURE(dst, "1234", "5.67", "89AB");
}

// Clipping the SOURCE does not move the destination: with x = -1 the pixels from x = 0
// land at dest_x0 (the image shifts by one compared with an unclipped copy).
RA_TEST(source_clip_shifts_image) {
    TestSurface src(4, 1);
    S07A_LOAD_PICTURE(src, "1234");
    TestSurface dst(6, 1);
    Linear_Blit_To_Linear(&src.vp, &dst.vp, -1, 0, 2, 0, 3, 1, 0);
    S07A_EXPECT_PICTURE(dst, "..12..");
}

RA_TEST(source_clip_right_and_bottom) {
    TestSurface src(4, 3);
    load_src(src);
    TestSurface dst(5, 4);
    dst.fill(Z);
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 2, 1, 0, 0, 9, 9, 0);
    S07A_EXPECT_PICTURE(dst,
        "67ZZZ",
        "ABZZZ",
        "ZZZZZ",
        "ZZZZZ");
}

// Clipping the DESTINATION keeps source and destination aligned.
RA_TEST(dest_clip_left_and_right) {
    TestSurface src(4, 1);
    S07A_LOAD_PICTURE(src, "1234");
    TestSurface a(6, 1);
    Linear_Blit_To_Linear(&src.vp, &a.vp, 0, 0, -1, 0, 4, 1, 0);
    S07A_EXPECT_PICTURE(a, "234...");
    TestSurface b(6, 1);
    Linear_Blit_To_Linear(&src.vp, &b.vp, 0, 0, 4, 0, 4, 1, 0);
    S07A_EXPECT_PICTURE(b, "....12");
}

RA_TEST(dest_clip_top_and_bottom) {
    TestSurface src(4, 3);
    load_src(src);
    TestSurface a(4, 2);
    a.fill(Z);
    Linear_Blit_To_Linear(&src.vp, &a.vp, 0, 0, 0, -1, 4, 3, 0);
    S07A_EXPECT_PICTURE(a, "5.67", "89AB");
    TestSurface b(4, 2);
    b.fill(Z);
    Linear_Blit_To_Linear(&src.vp, &b.vp, 0, 0, 0, 1, 4, 3, 0);
    S07A_EXPECT_PICTURE(b, "ZZZZ", "1234");
}

RA_TEST(rejected_blits_draw_nothing) {
    TestSurface src(4, 3);
    load_src(src);
    TestSurface dst(4, 3);
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 0, 0, 5, 0, 2, 2, 0);    // dest right of viewport
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 0, 0, 0, -9, 2, 2, 0);   // dest above
    Linear_Blit_To_Linear(&src.vp, &dst.vp, -9, 0, 0, 0, 2, 2, 0);   // source left of viewport
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 0, 2, 0);    // zero width
    Linear_Blit_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 0, 0);    // zero height
    S07A_EXPECT_PICTURE(dst, "....", "....", "....");
}

// Same buffer, source before destination: the asm copies backwards, like memmove.
RA_TEST(overlap_backward_in_row) {
    TestSurface s(8, 1);
    S07A_LOAD_PICTURE(s, "12345678");
    Linear_Blit_To_Linear(&s.vp, &s.vp, 0, 0, 2, 0, 6, 1, 0);
    S07A_EXPECT_PICTURE(s, "12123456");
}

RA_TEST(overlap_forward_in_row) {
    TestSurface s(8, 1);
    S07A_LOAD_PICTURE(s, "12345678");
    Linear_Blit_To_Linear(&s.vp, &s.vp, 2, 0, 0, 0, 6, 1, 0);
    S07A_EXPECT_PICTURE(s, "34567878");
}

RA_TEST(overlap_backward_rows) {
    TestSurface s(3, 4);
    S07A_LOAD_PICTURE(s, "123", "456", "789", "...");
    Linear_Blit_To_Linear(&s.vp, &s.vp, 0, 0, 0, 1, 3, 3, 0);
    S07A_EXPECT_PICTURE(s, "123", "123", "456", "789");
}

// With transparency the copy direction shows in the result: pixels written earlier in the
// pass can be read again (forward) or not (backward).
RA_TEST(overlap_transparent_direction) {
    TestSurface a(8, 1);
    S07A_LOAD_PICTURE(a, "1.3.5678");
    Linear_Blit_To_Linear(&a.vp, &a.vp, 0, 0, 2, 0, 4, 1, 1);      // backward: right to left
    S07A_EXPECT_PICTURE(a, "1.1.3678");

    TestSurface b(8, 1);
    S07A_LOAD_PICTURE(b, "1.3.5678");
    Linear_Blit_To_Linear(&b.vp, &b.vp, 2, 0, 0, 0, 4, 1, 1);      // forward: left to right
    S07A_EXPECT_PICTURE(b, "3.565678");
}

RA_TEST(same_address_is_a_no_op) {
    TestSurface s(4, 1);
    S07A_LOAD_PICTURE(s, "1234");
    Linear_Blit_To_Linear(&s.vp, &s.vp, 0, 0, 0, 0, 4, 1, 0);
    S07A_EXPECT_PICTURE(s, "1234");
}

RA_TEST_MAIN()
