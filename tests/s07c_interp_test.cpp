// S07-C: 2x interpolating upscalers and leftovers of CODE/WINASM.ASM (now CODE/WINASM.CPP).
// Synthetic interpolation table; expected output worked out by hand from WINASM.ASM (git history).
#include "ra_test.h"

#include <string.h>

extern "C" {
// Normally defined in CODE/INTERPAL.CPP.
unsigned char PaletteInterpolationTable[256][256];

void Asm_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width);
void Asm_Interpolate_Line_Double(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width);
void Asm_Interpolate_Line_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width);
void Stop_Execution(void);
void Int3(void);
extern int MouseQX;
extern int MouseQY;
}

// T[a][b] = a*16 + b, so mix(b, a) is easy to read: T[2][1] = 0x21.
static void Init_Table()
{
	for (int a = 0; a < 256; a++)
		for (int b = 0; b < 256; b++)
			PaletteInterpolationTable[a][b] = (unsigned char)(a * 16 + b);
}

enum { FILL = 0xEE };

// Source: 3 lines of 4 pixels (the third is only read by the Line_Interpolate mode).
static unsigned char g_src[3][4] = {
	{ 1, 2, 3, 4 },
	{ 5, 6, 7, 8 },
	{ 9, 10, 11, 12 },
};

// Interpolated rows: p0, T[p1][p0], p1, T[p2][p1], p2, T[p3][p2], p3, 0.
static const unsigned char ROW0[8] = { 1, 0x21, 2, 0x32, 3, 0x43, 4, 0 };
static const unsigned char ROW1[8] = { 5, 0x65, 6, 0x76, 7, 0x87, 8, 0 };
static const unsigned char ROW2[8] = { 9, 0xA9, 10, 0xBA, 11, 0xCB, 12, 0 };

RA_TEST(interpolate_rows_at_dest_width_stride) {
	Init_Table();
	unsigned char dest[40];
	memset(dest, FILL, sizeof(dest));
	Asm_Interpolate(&g_src[0][0], dest, 2, 4, 20);	// dest_width 20 = 2 * pitch 10
	unsigned char expect[40];
	memset(expect, FILL, sizeof(expect));
	memcpy(expect + 0, ROW0, 8);
	memcpy(expect + 20, ROW1, 8);
	RA_CHECK_MEM_EQ(expect, dest, sizeof(dest));
}

RA_TEST(interpolate_line_double_copies_each_row_below) {
	Init_Table();
	unsigned char dest[40];
	memset(dest, FILL, sizeof(dest));
	Asm_Interpolate_Line_Double(&g_src[0][0], dest, 2, 4, 20);
	unsigned char expect[40];
	memset(expect, FILL, sizeof(expect));
	memcpy(expect + 0, ROW0, 8);
	memcpy(expect + 10, ROW0, 8);
	memcpy(expect + 20, ROW1, 8);
	memcpy(expect + 30, ROW1, 8);
	RA_CHECK_MEM_EQ(expect, dest, sizeof(dest));
}

RA_TEST(interpolate_line_interpolate_mixes_rows_and_reads_one_past_end) {
	Init_Table();
	unsigned char dest[40];
	memset(dest, FILL, sizeof(dest));
	Asm_Interpolate_Line_Interpolate(&g_src[0][0], dest, 2, 4, 20);
	unsigned char mid[8];
	for (int i = 0; i < 8; i++) mid[i] = PaletteInterpolationTable[ROW1[i]][ROW0[i]];
	unsigned char expect[40];
	memset(expect, FILL, sizeof(expect));
	memcpy(expect + 0, ROW0, 8);
	memcpy(expect + 10, mid, 8);
	memcpy(expect + 20, ROW2, 8);		// asm quirk: source line `lines` (one past the end), not line 1
	RA_CHECK_MEM_EQ(expect, dest, sizeof(dest));
}

RA_TEST(interpolate_full_width_line_matches_reference) {
	// A 320-pixel line, as the game uses, against the formula from the asm.
	Init_Table();
	static unsigned char src[2][320];
	for (int i = 0; i < 320; i++) { src[0][i] = (unsigned char)(i * 37 + 11); src[1][i] = (unsigned char)(255 - i); }
	static unsigned char dest[2 * 1280];
	memset(dest, FILL, sizeof(dest));
	Asm_Interpolate(&src[0][0], dest, 2, 320, 1280);
	for (int line = 0; line < 2; line++) {
		const unsigned char *p = src[line];
		const unsigned char *d = dest + line * 1280;
		bool ok = true;
		for (int i = 0; i < 319; i++) {
			ok = ok && d[2 * i] == p[i] && d[2 * i + 1] == PaletteInterpolationTable[p[i + 1]][p[i]];
		}
		ok = ok && d[638] == p[319] && d[639] == 0 && d[640] == FILL;
		RA_CHECK(ok);
	}
}

RA_TEST(leftover_symbols) {
	Stop_Execution();		// no-ops, as in the asm
	Int3();
	RA_CHECK_EQ(0, MouseQX);
	RA_CHECK_EQ(0, MouseQY);
}

RA_TEST_MAIN()
