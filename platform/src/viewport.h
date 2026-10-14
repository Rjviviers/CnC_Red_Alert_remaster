/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer: where the game frame goes in the window, and mapping the mouse
**	back (session S08). Pure functions, unit-tested in tests/platform_viewport_test.cpp.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#ifndef RA_PLATFORM_VIEWPORT_H
#define RA_PLATFORM_VIEWPORT_H

namespace ra_platform {

struct Rect {
	int X;
	int Y;
	int W;
	int H;
};

/* Where to draw a game_w x game_h frame inside an out_w x out_h output (pixels).
**   scale_mode     PLATFORM_SCALE_INTEGER / _FIT / _STRETCH (platform.h)
**   aspect_correct show the frame at 4:3 (the 1996 CRT shape), i.e. as if it were
**                  game_w x (game_w * 3 / 4); for 640x400 every row is 1.2 pixels tall.
**                  Ignored for STRETCH.
** INTEGER picks the largest whole multiple of the (aspect-corrected) size whose
** horizontal factor is an integer; if even 1x does not fit it falls back to FIT.
** The result is centred; it never has a zero width or height when out_w, out_h > 0. */
Rect Compute_Viewport(int out_w, int out_h, int game_w, int game_h, int scale_mode, bool aspect_correct);

/* Map a point in output pixels to game coordinates for a frame drawn at `view`.
** Points outside the frame are clamped to the nearest edge pixel
** (0..game_w-1, 0..game_h-1), like the original cursor clip. */
void Output_To_Game(const Rect &view, int game_w, int game_h, double out_x, double out_y, int *game_x, int *game_y);

} // namespace ra_platform

#endif
