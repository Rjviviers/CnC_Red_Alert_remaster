// S07-A: Linear_Scale_To_Linear (WIN32LIB/DRAWBUFF/SCALE.CPP, ported from SCALE.ASM).
// Expected pixels follow the asm's stepping: 16.16 fixed point across a row (fraction reset to 0
// on every row) and a row accumulator that starts at -dst_height.
#include "s07a_surface.h"

static const unsigned char Z = 35;

// 2x2 -> 4x4. x step 0x8000: each source pixel twice. Rows: 2/4 = 0 rem 2, accumulator
// -4 -> -2 -> 0 -> 2: the source row only advances after the THIRD destination row.
RA_TEST(upscale_2x_row_pattern) {
    TestSurface src(2, 2);
    S07A_LOAD_PICTURE(src, "12", "34");
    TestSurface dst(4, 4);
    RA_CHECK_EQ(0, Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 2, 4, 4, 0, NULL));
    S07A_EXPECT_PICTURE(dst,
        "1122",
        "1122",
        "1122",
        "3344");
}

// 4x4 -> 2x2 with source and destination viewports that have row padding and pitch.
RA_TEST(downscale_2x_with_pitch) {
    TestSurface src(6, 5, 1, 1, 4, 4, /*pitch*/ 1);
    S07A_LOAD_PICTURE(src,
        "......",
        ".1234.",
        ".5678.",
        ".9ABC.",
        ".DEFG.");
    TestSurface dst(4, 4, 1, 1, 2, 2, 1);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 4, 4, 2, 2, 0, NULL);
    S07A_EXPECT_PICTURE(dst,
        "....",
        ".13.",
        ".9B.",
        "....");
}

// 3 -> 2 across: step 0x18000 = 1 + 1/2: pixels 0 and 1. 3 -> 2 down: 1 rem 1,
// accumulator -2 -> -1: rows 0 and 1.
RA_TEST(non_integer_ratio) {
    TestSurface src(3, 3);
    S07A_LOAD_PICTURE(src, "123", "456", "789");
    TestSurface dst(2, 2);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 3, 3, 2, 2, 0, NULL);
    S07A_EXPECT_PICTURE(dst, "12", "45");
}

RA_TEST(transparent_skips_source_zero) {
    TestSurface src(2, 1);
    S07A_LOAD_PICTURE(src, "1.");
    TestSurface dst(4, 1);
    dst.fill(Z);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 1, 4, 1, 1, NULL);
    S07A_EXPECT_PICTURE(dst, "11ZZ");
}

RA_TEST(remap_applies_to_every_pixel) {
    char table[256];
    for (int i = 0; i < 256; i++) table[i] = (char)(i + 1);
    TestSurface src(2, 1);
    S07A_LOAD_PICTURE(src, "1.");
    TestSurface dst(2, 1);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 1, 2, 1, 0, table);
    S07A_EXPECT_PICTURE(dst, "21");
}

// With trans + remap the zero test is on the SOURCE pixel: a colour remapped to 0 is still drawn.
RA_TEST(transparent_remap_tests_source_before_remap) {
    char table[256];
    for (int i = 0; i < 256; i++) table[i] = (char)i;
    table[0] = 9;   // never used: source zeros are skipped
    table[2] = 0;   // drawn as 0
    TestSurface src(3, 1);
    S07A_LOAD_PICTURE(src, ".23");
    TestSurface dst(3, 1);
    dst.fill(Z);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 3, 1, 3, 1, 1, table);
    S07A_EXPECT_PICTURE(dst, "Z.3");
}

// Destination clipped on the left: src_x0 = (0 - dst_x) * src_w / dst_w + src_x = 2 * 2 / 4 = 1.
RA_TEST(dest_clip_left_recomputes_source) {
    TestSurface src(2, 1);
    S07A_LOAD_PICTURE(src, "12");
    TestSurface dst(3, 1);
    dst.fill(Z);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, -2, 0, 2, 1, 4, 1, 0, NULL);
    S07A_EXPECT_PICTURE(dst, "22Z");
}

// Destination clipped on the right: dst_x1 = Width.
RA_TEST(dest_clip_right) {
    TestSurface src(2, 1);
    S07A_LOAD_PICTURE(src, "12");
    TestSurface dst(3, 1);
    dst.fill(Z);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 1, 0, 2, 1, 4, 1, 0, NULL);
    S07A_EXPECT_PICTURE(dst, "Z11");
}

// Source clipped on the left: dst_x0 = (0 - src_x) * dst_w / src_w + dst_x = 1 * 4 / 2 + 0 = 2.
RA_TEST(source_clip_left_recomputes_dest) {
    TestSurface src(2, 1);
    S07A_LOAD_PICTURE(src, "12");
    TestSurface dst(4, 1);
    dst.fill(Z);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, -1, 0, 0, 0, 2, 1, 4, 1, 0, NULL);
    S07A_EXPECT_PICTURE(dst, "ZZ11");
}

RA_TEST(zero_sizes_or_outside_do_nothing) {
    TestSurface src(2, 2);
    S07A_LOAD_PICTURE(src, "12", "34");
    TestSurface dst(4, 4);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 0, 2, 4, 4, 0, NULL);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 0, 4, 4, 0, NULL);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 2, 0, 4, 0, NULL);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 0, 0, 2, 2, 4, 0, 0, NULL);
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 0, 0, 5, 0, 2, 2, 4, 4, 0, NULL);   // dest right
    Linear_Scale_To_Linear(&src.vp, &dst.vp, 3, 0, 0, 0, 2, 2, 4, 4, 0, NULL);   // source right
    S07A_EXPECT_PICTURE(dst, "....", "....", "....", "....");
}

RA_TEST_MAIN()
