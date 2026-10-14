// S07-A: Clip_Rect and Confine_Rect (WIN32LIB/MISC/CLIPRECT.CPP, ported from CLIPRECT.ASM).
#include "ra_test.h"

// Same declarations as WIN32LIB/INCLUDE/MISC.H.
#if defined(_MSC_VER) || defined(__MINGW32__)
#define TEST_CDECL __cdecl
#else
#define TEST_CDECL
#endif
extern "C" int TEST_CDECL Clip_Rect(int *x, int *y, int *dw, int *dh, int width, int height);
extern "C" int TEST_CDECL Confine_Rect(int *x, int *y, int dw, int dh, int width, int height);

struct R { int x, y, w, h; };

static int clip(R &r, int width, int height) { return Clip_Rect(&r.x, &r.y, &r.w, &r.h, width, height); }

#define CHECK_RECT(r, ex, ey, ew, eh)                                                  \
    do { RA_CHECK_EQ(ex, (r).x); RA_CHECK_EQ(ey, (r).y); RA_CHECK_EQ(ew, (r).w); RA_CHECK_EQ(eh, (r).h); } while (0)

RA_TEST(clip_inside_returns_0) {
    R r = {2, 3, 4, 5};
    RA_CHECK_EQ(0, clip(r, 10, 10));
    CHECK_RECT(r, 2, 3, 4, 5);
    R edge = {6, 5, 4, 5};             // x + w == width is still inside
    RA_CHECK_EQ(0, clip(edge, 10, 10));
    CHECK_RECT(edge, 6, 5, 4, 5);
}

RA_TEST(clip_outside_returns_minus_1) {
    R a = {-10, 0, 5, 5};              // left
    R b = {11, 0, 5, 5};               // right
    R c = {0, -10, 5, 5};              // above
    R d = {0, 11, 5, 5};               // below
    RA_CHECK_EQ(-1, clip(a, 10, 10));
    RA_CHECK_EQ(-1, clip(b, 10, 10));
    RA_CHECK_EQ(-1, clip(c, 10, 10));
    RA_CHECK_EQ(-1, clip(d, 10, 10));
    CHECK_RECT(a, -10, 0, 5, 5);       // untouched
}

RA_TEST(clip_left_top) {
    R r = {-2, -3, 5, 6};
    RA_CHECK_EQ(1, clip(r, 10, 10));
    CHECK_RECT(r, 0, 0, 3, 3);
}

RA_TEST(clip_right_bottom) {
    R r = {7, 8, 5, 6};
    RA_CHECK_EQ(1, clip(r, 10, 10));
    CHECK_RECT(r, 7, 8, 3, 2);
}

RA_TEST(clip_all_sides) {
    R r = {-1, -1, 20, 20};
    RA_CHECK_EQ(1, clip(r, 10, 8));
    CHECK_RECT(r, 0, 0, 10, 8);
}

// Starting exactly at the right edge is not rejected; the right clip leaves width 0 -> -1,
// after storing the new width.
RA_TEST(clip_to_zero_width_returns_minus_1) {
    R r = {10, 2, 3, 2};
    RA_CHECK_EQ(-1, clip(r, 10, 10));
    CHECK_RECT(r, 10, 2, 0, 2);
}

// A start past the right edge with a negative width ends inside: neither rejected nor changed.
RA_TEST(clip_negative_width_quirk_returns_1) {
    R r = {12, 2, -5, 2};
    RA_CHECK_EQ(1, clip(r, 10, 10));
    CHECK_RECT(r, 12, 2, -5, 2);
}

RA_TEST(confine_inside_returns_0) {
    int x = 3, y = 4;
    RA_CHECK_EQ(0, Confine_Rect(&x, &y, 5, 5, 10, 10));
    RA_CHECK_EQ(3, x);
    RA_CHECK_EQ(4, y);
    x = 5; y = 5;                      // touching the right/bottom edge is inside
    RA_CHECK_EQ(0, Confine_Rect(&x, &y, 5, 5, 10, 10));
    RA_CHECK_EQ(5, x);
    RA_CHECK_EQ(5, y);
}

RA_TEST(confine_moves_back_inside) {
    int x = -4, y = 8;
    RA_CHECK_EQ(1, Confine_Rect(&x, &y, 5, 5, 10, 10));
    RA_CHECK_EQ(0, x);
    RA_CHECK_EQ(5, y);
    x = 9; y = -1;
    RA_CHECK_EQ(1, Confine_Rect(&x, &y, 5, 5, 10, 10));
    RA_CHECK_EQ(5, x);
    RA_CHECK_EQ(0, y);
}

// Kept from the asm: a position of exactly 0 counts as "adjusted" (returns 1) although it does not move.
RA_TEST(confine_zero_position_reports_shift) {
    int x = 0, y = 3;
    RA_CHECK_EQ(1, Confine_Rect(&x, &y, 5, 5, 10, 10));
    RA_CHECK_EQ(0, x);
    RA_CHECK_EQ(3, y);
    x = 3; y = 0;
    RA_CHECK_EQ(1, Confine_Rect(&x, &y, 5, 5, 10, 10));
    RA_CHECK_EQ(3, x);
    RA_CHECK_EQ(0, y);
}

RA_TEST_MAIN()
