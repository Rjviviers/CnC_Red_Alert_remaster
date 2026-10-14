// S08: frame placement in the window and mouse coordinate scaling (platform/src/viewport.cpp).
#include "ra_test.h"
#include "viewport.h"
#include "platform.h"

using namespace ra_platform;

RA_TEST(integer_scaling_without_aspect) {
    Rect r = Compute_Viewport(1920, 1080, 640, 400, PLATFORM_SCALE_INTEGER, false);
    RA_CHECK_EQ(1280, r.W);           // 2x: 3x would be 1200 tall
    RA_CHECK_EQ(800, r.H);
    RA_CHECK_EQ(320, r.X);
    RA_CHECK_EQ(140, r.Y);

    r = Compute_Viewport(640, 400, 640, 400, PLATFORM_SCALE_INTEGER, false);
    RA_CHECK_EQ(0, r.X); RA_CHECK_EQ(0, r.Y); RA_CHECK_EQ(640, r.W); RA_CHECK_EQ(400, r.H);
}

RA_TEST(integer_scaling_with_aspect_uses_4_3) {
    // 640x400 shown as 640x480 units: 2x = 1280x960 fits in 1920x1080.
    Rect r = Compute_Viewport(1920, 1080, 640, 400, PLATFORM_SCALE_INTEGER, true);
    RA_CHECK_EQ(1280, r.W);
    RA_CHECK_EQ(960, r.H);
    RA_CHECK_EQ(320, r.X);
    RA_CHECK_EQ(60, r.Y);
    // 640x480 is already 4:3: aspect correction changes nothing.
    Rect a = Compute_Viewport(1920, 1080, 640, 480, PLATFORM_SCALE_INTEGER, true);
    Rect b = Compute_Viewport(1920, 1080, 640, 480, PLATFORM_SCALE_INTEGER, false);
    RA_CHECK_EQ(a.W, b.W); RA_CHECK_EQ(a.H, b.H);
}

RA_TEST(integer_falls_back_to_fit_when_too_small) {
    Rect r = Compute_Viewport(320, 240, 640, 400, PLATFORM_SCALE_INTEGER, false);
    RA_CHECK_EQ(320, r.W);
    RA_CHECK_EQ(200, r.H);
    RA_CHECK_EQ(20, r.Y);
}

RA_TEST(fit_keeps_aspect_and_centres) {
    Rect r = Compute_Viewport(1920, 1080, 640, 400, PLATFORM_SCALE_FIT, true);   // 4:3 in 16:9
    RA_CHECK_EQ(1440, r.W);
    RA_CHECK_EQ(1080, r.H);
    RA_CHECK_EQ(240, r.X);
    RA_CHECK_EQ(0, r.Y);
    r = Compute_Viewport(1920, 1080, 640, 400, PLATFORM_SCALE_FIT, false);       // 16:10 in 16:9
    RA_CHECK_EQ(1728, r.W);
    RA_CHECK_EQ(1080, r.H);
    RA_CHECK_EQ(96, r.X);
    r = Compute_Viewport(1000, 2000, 640, 400, PLATFORM_SCALE_FIT, false);       // tall window
    RA_CHECK_EQ(1000, r.W);
    RA_CHECK_EQ(625, r.H);
    RA_CHECK_EQ(687, r.Y);
}

RA_TEST(stretch_fills_window) {
    Rect r = Compute_Viewport(1000, 700, 640, 400, PLATFORM_SCALE_STRETCH, true);
    RA_CHECK_EQ(0, r.X); RA_CHECK_EQ(0, r.Y); RA_CHECK_EQ(1000, r.W); RA_CHECK_EQ(700, r.H);
}

RA_TEST(degenerate_sizes) {
    Rect r = Compute_Viewport(0, 0, 640, 400, PLATFORM_SCALE_FIT, true);
    RA_CHECK_EQ(0, r.W);
    r = Compute_Viewport(1, 1, 640, 400, PLATFORM_SCALE_FIT, true);
    RA_CHECK(r.W >= 1 && r.H >= 1);
}

RA_TEST(mouse_maps_back_to_game_pixels) {
    Rect view = Compute_Viewport(1920, 1080, 640, 400, PLATFORM_SCALE_INTEGER, true);  // 320,60 1280x960
    int x = -1, y = -1;
    Output_To_Game(view, 640, 400, 320.0, 60.0, &x, &y);            // top-left corner
    RA_CHECK_EQ(0, x); RA_CHECK_EQ(0, y);
    Output_To_Game(view, 640, 400, 320.0 + 1279.9, 60.0 + 959.9, &x, &y);   // bottom-right
    RA_CHECK_EQ(639, x); RA_CHECK_EQ(399, y);
    Output_To_Game(view, 640, 400, 320.0 + 640.0, 60.0 + 480.0, &x, &y);    // centre
    RA_CHECK_EQ(320, x); RA_CHECK_EQ(200, y);
    // Each game row is 2.4 output rows tall here: output row 60+3 is still game row 1.
    Output_To_Game(view, 640, 400, 321.0, 63.0, &x, &y);
    RA_CHECK_EQ(0, x); RA_CHECK_EQ(1, y);
}

RA_TEST(mouse_outside_the_frame_is_clamped) {
    Rect view = Compute_Viewport(1920, 1080, 640, 400, PLATFORM_SCALE_FIT, true);       // 240,0 1440x1080
    int x = -1, y = -1;
    Output_To_Game(view, 640, 400, 10.0, 500.0, &x, &y);            // left black bar
    RA_CHECK_EQ(0, x);
    Output_To_Game(view, 640, 400, 1919.0, -5.0, &x, &y);           // right bar, above the window
    RA_CHECK_EQ(639, x); RA_CHECK_EQ(0, y);
    Output_To_Game(view, 640, 400, 900.0, 5000.0, &x, &y);
    RA_CHECK_EQ(399, y);
}

RA_TEST(round_trip_every_game_pixel) {
    // The centre of every game pixel's output area maps back to that pixel.
    Rect view = Compute_Viewport(1366, 768, 640, 400, PLATFORM_SCALE_FIT, true);
    int bad = 0;
    for (int gy = 0; gy < 400; gy++) {
        for (int gx = 0; gx < 640; gx++) {
            double ox = view.X + (gx + 0.5) * view.W / 640.0;
            double oy = view.Y + (gy + 0.5) * view.H / 400.0;
            int x, y;
            Output_To_Game(view, 640, 400, ox, oy, &x, &y);
            if (x != gx || y != gy) bad++;
        }
    }
    RA_CHECK_EQ(0, bad);
}

RA_TEST_MAIN()
