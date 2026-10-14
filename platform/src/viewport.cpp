/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer: viewport computation (session S08). See viewport.h.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#include "viewport.h"
#include "../platform.h"

#include <math.h>

namespace ra_platform {

Rect Compute_Viewport(int out_w, int out_h, int game_w, int game_h, int scale_mode, bool aspect_correct)
{
	Rect r = { 0, 0, out_w, out_h };
	if (out_w <= 0 || out_h <= 0 || game_w <= 0 || game_h <= 0) {
		return r;
	}
	if (scale_mode == PLATFORM_SCALE_STRETCH) {
		return r;
	}

	/*
	** The shape the frame should have on screen. With aspect correction a frame is
	** shown at 4:3 whatever its pixel size (640x400 -> 640x480, 640x480 unchanged).
	*/
	long long disp_w = game_w;
	long long disp_h = aspect_correct ? (game_w * 3LL) / 4 : game_h;

	long long w = 0;
	long long h = 0;

	if (scale_mode == PLATFORM_SCALE_INTEGER) {
		long long k = out_w / disp_w;
		if (out_h / disp_h < k) k = out_h / disp_h;
		if (k >= 1) {
			w = disp_w * k;
			h = disp_h * k;
		}
	}

	if (w == 0) {
		/* FIT (also the INTEGER fallback when the window is smaller than 1x). */
		if ((long long)out_w * disp_h <= (long long)out_h * disp_w) {
			w = out_w;
			h = ((long long)out_w * disp_h) / disp_w;
		} else {
			h = out_h;
			w = ((long long)out_h * disp_w) / disp_h;
		}
		if (w < 1) w = 1;
		if (h < 1) h = 1;
	}

	r.W = (int)w;
	r.H = (int)h;
	r.X = (out_w - r.W) / 2;
	r.Y = (out_h - r.H) / 2;
	return r;
}


void Output_To_Game(const Rect &view, int game_w, int game_h, double out_x, double out_y, int *game_x, int *game_y)
{
	int gx = 0;
	int gy = 0;
	if (view.W > 0 && view.H > 0) {
		gx = (int)floor((out_x - view.X) * game_w / view.W);
		gy = (int)floor((out_y - view.Y) * game_h / view.H);
	}
	if (gx < 0) gx = 0;
	if (gy < 0) gy = 0;
	if (gx > game_w - 1) gx = game_w - 1;
	if (gy > game_h - 1) gy = game_h - 1;
	if (game_x) *game_x = gx;
	if (game_y) *game_y = gy;
}

} // namespace ra_platform
