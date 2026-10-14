/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	platform_demo (session S08): exercises the platform layer the way the game does.
**
**	- draws a test pattern into an 8-bit 640x400 buffer, exactly like the game's
**	  VisiblePage (the buffer is registered with Platform_Set_Frame_Buffer and the
**	  pump presents it);
**	- cycles palette entries 96..127 at the game's 60 Hz tick (like water/fire cycling);
**	- echoes every input event as the game would see it (WW key code, ASCII,
**	  mouse position in game coordinates) on stdout and draws a cursor marker;
**	- Escape or closing the window quits; F11 or Alt+Enter toggles fullscreen;
**	  F1/F2/F3 = integer/fit/stretch scaling, F4 toggles 4:3 aspect correction.
**
**	Options: --frames N (quit after N frames), --height 480, --fullscreen,
**	         --integer | --stretch, --no-aspect, --dump FILE.bmp (save the last
**	         frame through the palette, for headless checks).
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace {

const int CYCLE_FIRST = 96;
const int CYCLE_COUNT = 32;

unsigned char Frame[640 * 480];
unsigned char Palette[768];			/* 6-bit, like the game's palettes */
int Width = 640;
int Height = 400;

void Put(int x, int y, unsigned char c)
{
	if (x >= 0 && y >= 0 && x < Width && y < Height) Frame[y * Width + x] = c;
}

void Fill(int x, int y, int w, int h, unsigned char c)
{
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) Put(x + i, y + j, c);
	}
}

void Build_Palette(void)
{
	for (int i = 0; i < 256; i++) {
		unsigned char *p = &Palette[i * 3];
		if (i < 16) {
			/* EGA-like basics */
			static const unsigned char ega[16][3] = {
				{0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},
				{21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63}};
			p[0] = ega[i][0]; p[1] = ega[i][1]; p[2] = ega[i][2];
		} else if (i < 80) {		/* grey ramp */
			p[0] = p[1] = p[2] = (unsigned char)(i - 16);
		} else if (i < 96) {		/* red ramp */
			p[0] = (unsigned char)((i - 80) * 4 + 3); p[1] = 0; p[2] = 0;
		} else if (i < 128) {		/* cycling range: a rainbow */
			int k = i - 96;
			int seg = k / 8, f = (k % 8) * 8;
			unsigned char r = 0, g = 0, b = 0;
			switch (seg) {
				case 0: r = 63; g = (unsigned char)f; break;
				case 1: r = (unsigned char)(63 - f); g = 63; break;
				case 2: g = 63; b = (unsigned char)f; break;
				default: g = (unsigned char)(63 - f); b = 63; r = (unsigned char)f; break;
			}
			p[0] = r; p[1] = g; p[2] = b;
		} else {					/* blue/green ramps */
			int k = i - 128;
			p[0] = 0; p[1] = (unsigned char)(k / 2); p[2] = (unsigned char)(63 - k / 2);
		}
	}
}

void Cycle_Palette(void)
{
	unsigned char first[3];
	memcpy(first, &Palette[CYCLE_FIRST * 3], 3);
	memmove(&Palette[CYCLE_FIRST * 3], &Palette[(CYCLE_FIRST + 1) * 3], (CYCLE_COUNT - 1) * 3);
	memcpy(&Palette[(CYCLE_FIRST + CYCLE_COUNT - 1) * 3], first, 3);
	Platform_Set_Palette(Palette, 6);
}

void Draw_Pattern(void)
{
	memset(Frame, 0, sizeof(Frame));
	/* 16x16 swatches of every palette index */
	for (int c = 0; c < 256; c++) {
		Fill(8 + (c % 16) * 20, 8 + (c / 16) * 20, 18, 18, (unsigned char)c);
	}
	/* the cycling band */
	for (int x = 0; x < 288; x++) {
		Fill(336 + x, 8, 1, 60, (unsigned char)(CYCLE_FIRST + (x / 9) % CYCLE_COUNT));
	}
	/* 1-pixel checkerboard and lines: shows scaling quality */
	for (int y = 80; y < 200; y++) {
		for (int x = 336; x < 480; x++) Put(x, y, ((x + y) & 1) ? 15 : 0);
		for (int x = 488; x < 632; x++) Put(x, y, (y % 4 == 0) ? 15 : 8);
	}
	/* grey ramp */
	for (int x = 0; x < 64; x++) Fill(336 + x * 4, 210, 4, 40, (unsigned char)(16 + x));
	/* a 1-pixel white border on the very edge: must be fully visible */
	for (int x = 0; x < Width; x++) { Put(x, 0, 15); Put(x, Height - 1, 15); }
	for (int y = 0; y < Height; y++) { Put(0, y, 15); Put(Width - 1, y, 15); }
	/* circles must look round when 4:3 aspect correction is on (pixels 1.2 tall) */
	for (int a = 0; a < 3600; a++) {
		double t = a * 3.14159265358979 / 1800.0;
		for (int r = 30; r <= 60; r += 15) {
			/* radius in "4:3 display" units: y is scaled by 1/1.2 */
			int x = 480 + (int)(r * cos(t));
			int y = 320 + (int)(r * sin(t) / (Height == 400 ? 1.2 : 1.0));
			Put(x, y, 14);
		}
	}
}

unsigned char Base[640 * 480];		/* the pattern without the cursor */

void Draw_Cursor(int mx, int my, bool erase)
{
	for (int i = -6; i <= 6; i++) {
		int xs[2] = { mx + i, mx };
		int ys[2] = { my, my + i };
		for (int k = 0; k < 2; k++) {
			int x = xs[k], y = ys[k];
			if (x >= 0 && y >= 0 && x < Width && y < Height) {
				Put(x, y, erase ? Base[y * Width + x] : 15);
			}
		}
	}
}

bool Write_BMP(const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f) return false;
	unsigned char pal[768];
	Platform_Get_Palette(pal);
	int row = Width * 3;
	unsigned size = 54 + (unsigned)(row * Height);
	unsigned char h[54] = { 'B', 'M' };
	h[2] = (unsigned char)size; h[3] = (unsigned char)(size >> 8); h[4] = (unsigned char)(size >> 16); h[5] = (unsigned char)(size >> 24);
	h[10] = 54; h[14] = 40;
	h[18] = (unsigned char)Width; h[19] = (unsigned char)(Width >> 8);
	h[22] = (unsigned char)Height; h[23] = (unsigned char)(Height >> 8);
	h[26] = 1; h[28] = 24;
	fwrite(h, 1, 54, f);
	for (int y = Height - 1; y >= 0; y--) {
		for (int x = 0; x < Width; x++) {
			const unsigned char *c = &pal[Frame[y * Width + x] * 3];
			unsigned char bgr[3] = { c[2], c[1], c[0] };
			fwrite(bgr, 1, 3, f);
		}
	}
	fclose(f);
	return true;
}

} // namespace


int main(int argc, char **argv)
{
	long max_frames = -1;
	const char *dump = NULL;
	PlatformVideoConfig config;
	Platform_Default_Video_Config(&config);

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--frames") && i + 1 < argc) max_frames = atol(argv[++i]);
		else if (!strcmp(argv[i], "--height") && i + 1 < argc) config.Height = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--fullscreen")) config.Fullscreen = 1;
		else if (!strcmp(argv[i], "--integer")) config.ScaleMode = PLATFORM_SCALE_INTEGER;
		else if (!strcmp(argv[i], "--stretch")) config.ScaleMode = PLATFORM_SCALE_STRETCH;
		else if (!strcmp(argv[i], "--no-aspect")) config.AspectCorrect = 0;
		else if (!strcmp(argv[i], "--dump") && i + 1 < argc) dump = argv[++i];
		else {
			printf("usage: %s [--frames N] [--height 400|480] [--fullscreen] [--integer|--stretch] [--no-aspect] [--dump FILE.bmp]\n", argv[0]);
			return 2;
		}
	}
	if (config.Height != 480) config.Height = 400;
	Height = config.Height;

	if (!Platform_Init()) {
		printf("Platform_Init failed: %s\n", Platform_Last_Error());
		return 1;
	}
	if (!Platform_Video_Init(&config, "Red Alert platform demo")) {
		printf("Platform_Video_Init failed: %s\n", Platform_Last_Error());
		Platform_Shutdown();
		return 1;
	}

	Build_Palette();
	Platform_Set_Palette(Palette, 6);
	Draw_Pattern();
	memcpy(Base, Frame, sizeof(Base));
	Platform_Set_Frame_Buffer(Frame, Width, Height, Width);
	Platform_Show_System_Cursor(false);

	printf("platform_demo: %dx%d, Esc quits, F11/Alt+Enter fullscreen, F1-F3 scaling, F4 aspect\n", Width, Height);

	uint64_t start_ms = Platform_Milliseconds64();
	uint32_t last_tick = 0;
	long frames = 0;
	int mx = 0, my = 0, old_mx = -1, old_my = -1;
	bool running = true;
	bool aspect = config.AspectCorrect != 0;

	while (running) {
		Platform_Pump_Events();
		if (Platform_Quit_Requested()) break;

		PlatformInputEvent ev;
		while (Platform_Get_Input(&ev)) {
			if (ev.Type == PLATFORM_INPUT_MOUSE) {
				printf("MOUSE key=0x%04X %s at (%d,%d)\n", (unsigned)ev.Key,
					(ev.Key & PLATFORM_WWKEY_RLS_BIT) ? "up  " : "down", (int)ev.X, (int)ev.Y);
				if (!(ev.Key & PLATFORM_WWKEY_RLS_BIT)) {
					if (old_mx >= 0) Draw_Cursor(old_mx, old_my, true);
					old_mx = old_my = -1;
					Fill(ev.X - 2, ev.Y - 2, 5, 5, 12);
					memcpy(Base, Frame, sizeof(Base));
				}
			} else {
				char ascii = Platform_Key_To_ASCII(ev.Key);
				printf("KEY   key=0x%04X vk=0x%02X%s%s%s%s ascii=%d%s%c%s\n", (unsigned)ev.Key, (unsigned)(ev.Key & 0xFF),
					(ev.Key & PLATFORM_WWKEY_SHIFT_BIT) ? " SHIFT" : "",
					(ev.Key & PLATFORM_WWKEY_CTRL_BIT) ? " CTRL" : "",
					(ev.Key & PLATFORM_WWKEY_ALT_BIT) ? " ALT" : "",
					(ev.Key & PLATFORM_WWKEY_RLS_BIT) ? " RELEASE" : "",
					(int)ascii, (ascii >= 32 && ascii < 127) ? " '" : "", (ascii >= 32 && ascii < 127) ? ascii : ' ',
					(ascii >= 32 && ascii < 127) ? "'" : "");
				if (ev.Key & PLATFORM_WWKEY_RLS_BIT) continue;
				switch (ev.Key & 0xFF) {
					case 0x1B: running = false; break;									/* VK_ESCAPE */
					case 0x7A: Platform_Toggle_Fullscreen(); break;					/* VK_F11 */
					case 0x70: Platform_Set_Scale_Mode(PLATFORM_SCALE_INTEGER, aspect); break;
					case 0x71: Platform_Set_Scale_Mode(PLATFORM_SCALE_FIT, aspect); break;
					case 0x72: Platform_Set_Scale_Mode(PLATFORM_SCALE_STRETCH, aspect); break;
					case 0x73: aspect = !aspect; Platform_Set_Scale_Mode(PLATFORM_SCALE_FIT, aspect); break;
					default: break;
				}
			}
			fflush(stdout);
		}

		/* 60 Hz game tick: cycle the palette once per tick, like the game does. */
		uint32_t tick = Platform_Ticks_In(Platform_Milliseconds64() - start_ms, 60);
		if (tick != last_tick) {
			last_tick = tick;
			Cycle_Palette();
		}

		Platform_Get_Mouse_Position(&mx, &my);
		if (mx != old_mx || my != old_my) {
			if (old_mx >= 0) Draw_Cursor(old_mx, old_my, true);
			Draw_Cursor(mx, my, false);
			old_mx = mx;
			old_my = my;
		}

		frames++;
		if (max_frames >= 0 && frames >= max_frames) break;
		Platform_Sleep(1);
	}

	uint64_t elapsed = Platform_Milliseconds64() - start_ms;
	printf("platform_demo: %ld frames, %u ticks in %llu ms (period %u ms)\n", frames,
		(unsigned)Platform_Ticks_In(elapsed, 60), (unsigned long long)elapsed, (unsigned)Platform_Tick_Period_Ms(60));

	if (dump) {
		Platform_Present();
		printf("platform_demo: %s %s\n", Write_BMP(dump) ? "wrote" : "could not write", dump);
	}

	Platform_Shutdown();
	return 0;
}
