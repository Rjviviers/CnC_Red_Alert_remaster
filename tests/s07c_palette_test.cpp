// S07-C: palette routines (WIN32LIB/PALETTE/PAL.CPP) and fading tables (WIN32LIB/MISC/FADING.CPP).
// Synthetic palettes; expected tables worked out by hand from PAL.ASM / FADING.ASM (git history).
#include "ra_test.h"

#include <string.h>

extern "C" {
extern unsigned char CurrentPalette[768];
void Set_Palette_Range(void *palette);
int Bump_Color(void *palette, int changable, int target);
void *Build_Fading_Table(void const *palette, void const *dest, long int color, long int frac);

// Normally in WIN32LIB/MISC/DDRAW.CPP (S08 replaces it). Records the call.
static int g_dd_calls = 0;
static void *g_dd_palette = 0;
void Set_DD_Palette(void *palette) { ++g_dd_calls; g_dd_palette = palette; }
}

static void Set_Rgb(unsigned char *pal, int index, int r, int g, int b)
{
	pal[index * 3 + 0] = (unsigned char)r;
	pal[index * 3 + 1] = (unsigned char)g;
	pal[index * 3 + 2] = (unsigned char)b;
}

// ---------------------------------------------------------------------------
// Set_Palette_Range / Bump_Color
// ---------------------------------------------------------------------------

RA_TEST(current_palette_starts_all_255) {		// must run before set_palette_range
	for (int i = 0; i < 768; i++) {
		if (CurrentPalette[i] != 255) { RA_CHECK_EQ(255, CurrentPalette[i]); break; }
	}
}

RA_TEST(set_palette_range_copies_and_hands_over) {
	unsigned char pal[768];
	for (int i = 0; i < 768; i++) pal[i] = (unsigned char)(i * 7);
	Set_Palette_Range(pal);
	RA_CHECK_MEM_EQ(pal, CurrentPalette, 768);
	RA_CHECK_EQ(1, g_dd_calls);
	RA_CHECK(g_dd_palette == (void *)pal);		// the caller's palette, not the copy
}

RA_TEST(bump_color_steps_each_gun_toward_target) {
	unsigned char pal[768];
	memset(pal, 0, sizeof(pal));
	Set_Rgb(pal, 1, 10, 20, 30);
	Set_Rgb(pal, 2, 12, 20, 28);

	RA_CHECK_EQ(1, Bump_Color(pal, 1, 2));
	RA_CHECK_EQ(11, pal[3]); RA_CHECK_EQ(20, pal[4]); RA_CHECK_EQ(29, pal[5]);
	RA_CHECK_EQ(1, Bump_Color(pal, 1, 2));
	RA_CHECK_EQ(12, pal[3]); RA_CHECK_EQ(20, pal[4]); RA_CHECK_EQ(28, pal[5]);
	RA_CHECK_EQ(0, Bump_Color(pal, 1, 2));
	RA_CHECK_EQ(12, pal[3]); RA_CHECK_EQ(20, pal[4]); RA_CHECK_EQ(28, pal[5]);
	RA_CHECK_EQ(12, pal[6]); RA_CHECK_EQ(28, pal[8]);	// target untouched

	// Colour numbers are WORDs: only the low 16 bits count.
	Set_Rgb(pal, 1, 0, 0, 0);
	RA_CHECK_EQ(1, Bump_Color(pal, 0x10001, 0x20002));
	RA_CHECK_EQ(1, pal[3]); RA_CHECK_EQ(1, pal[4]); RA_CHECK_EQ(1, pal[5]);
}

// ---------------------------------------------------------------------------
// Build_Fading_Table
//
// Palette A:  0 (0,0,0)   1 (0,0,0)   2 (10,10,10)  3 (20,20,20)
//             4 (40,40,40) 5 (63,63,63) 6 (37,37,37) 7..255 (63,0,63)
// ---------------------------------------------------------------------------
static void Make_Palette_A(unsigned char *pal)
{
	for (int i = 0; i < 256; i++) Set_Rgb(pal, i, 63, 0, 63);
	Set_Rgb(pal, 0, 0, 0, 0);
	Set_Rgb(pal, 1, 0, 0, 0);
	Set_Rgb(pal, 2, 10, 10, 10);
	Set_Rgb(pal, 3, 20, 20, 20);
	Set_Rgb(pal, 4, 40, 40, 40);
	Set_Rgb(pal, 5, 63, 63, 63);
	Set_Rgb(pal, 6, 37, 37, 37);
}

RA_TEST(fade_toward_black_half) {
	// frac 128 -> DL = 64; toward 0: ideal = orig - (orig*64*2 >> 8) = orig - orig/2.
	unsigned char pal[768], table[256];
	Make_Palette_A(pal);
	memset(table, 0xCC, sizeof(table));
	RA_CHECK(Build_Fading_Table(pal, table, 1, 128) == (void *)table);
	RA_CHECK_EQ(0, table[0]);
	RA_CHECK_EQ(2, table[1]);	// ideal 0: colour 1 itself is excluded, colour 0 never used -> 2 (d=300)
	RA_CHECK_EQ(1, table[2]);	// ideal 5:  1 (d=75)
	RA_CHECK_EQ(2, table[3]);	// ideal 10: exact 2
	RA_CHECK_EQ(3, table[4]);	// ideal 20: exact 3
	RA_CHECK_EQ(6, table[5]);	// ideal 32: 6 (d=75) beats 4 (d=192)
	RA_CHECK_EQ(3, table[6]);	// ideal 19: 3 (d=3)
	for (int i = 7; i < 256; i++) {
		if (table[i] != 3) { RA_CHECK_EQ(3, table[i]); break; }	// ideal (32,0,32): 3 (d=688)
	}
}

RA_TEST(fade_toward_white_uses_8bit_signed_math) {
	// Toward (63,63,63): orig 10 -> AL = -53, AX = -53*64 = -3392, <<1 = 0xE580, AH = 0xE5,
	// 10 - 0xE5 = 37 (mod 256). Likewise 0 -> 32, 20 -> 42, 40 -> 52, 37 -> 50.
	unsigned char pal[768], table[256];
	Make_Palette_A(pal);
	Build_Fading_Table(pal, table, 5, 128);
	RA_CHECK_EQ(0, table[0]);
	RA_CHECK_EQ(6, table[1]);	// ideal 32: 6 (d=75)
	RA_CHECK_EQ(6, table[2]);	// ideal 37: exact 6
	RA_CHECK_EQ(4, table[3]);	// ideal 42: 4 (d=12)
	RA_CHECK_EQ(5, table[4]);	// ideal 52: 5 (d=363)
	RA_CHECK_EQ(4, table[5]);	// ideal 63: itself excluded; 4 (d=1587)
	RA_CHECK_EQ(4, table[6]);	// ideal 50: 4 (d=300)
	for (int i = 7; i < 256; i++) {
		if (table[i] != 5) { RA_CHECK_EQ(5, table[i]); break; }	// ideal (63,32,63): 5 (d=961)
	}
}

// Palette B: 0,1 (0,0,0)  2 (10,10,10)  3 (20,20,20)  4 (40,40,40)  5..255 (63,0,63)
static void Make_Palette_B(unsigned char *pal)
{
	for (int i = 0; i < 256; i++) Set_Rgb(pal, i, 63, 0, 63);
	Set_Rgb(pal, 0, 0, 0, 0);
	Set_Rgb(pal, 1, 0, 0, 0);
	Set_Rgb(pal, 2, 10, 10, 10);
	Set_Rgb(pal, 3, 20, 20, 20);
	Set_Rgb(pal, 4, 40, 40, 40);
}

RA_TEST(fade_zero_ties_go_to_higher_index_exact_stops_early) {
	// frac 0 (and 1: DL = frac/2 = 0): ideal = original colour.
	unsigned char pal[768], t0[256], t1[256];
	Make_Palette_B(pal);
	Build_Fading_Table(pal, t0, 1, 0);
	Build_Fading_Table(pal, t1, 1, 1);
	RA_CHECK_MEM_EQ(t0, t1, 256);
	RA_CHECK_EQ(0, t0[0]);
	RA_CHECK_EQ(2, t0[1]);
	RA_CHECK_EQ(3, t0[2]);		// 1 and 3 both at d=300: the later one wins ("<=")
	RA_CHECK_EQ(2, t0[3]);
	RA_CHECK_EQ(3, t0[4]);
	RA_CHECK_EQ(6, t0[5]);		// first exact match after skipping itself
	for (int i = 6; i < 256; i++) {
		if (t0[i] != 5) { RA_CHECK_EQ(5, t0[i]); break; }
	}
}

RA_TEST(fade_fraction_clamps_to_255) {
	unsigned char pal[768], t255[256], tbig[256], tneg[256];
	Make_Palette_A(pal);
	Build_Fading_Table(pal, t255, 1, 255);
	Build_Fading_Table(pal, tbig, 1, 0x100);
	Build_Fading_Table(pal, tneg, 1, -1);		// unsigned compare in the asm
	RA_CHECK_MEM_EQ(t255, tbig, 256);
	RA_CHECK_MEM_EQ(t255, tneg, 256);
	// frac 255 -> DL 127: orig 10 toward 0 -> AX = 10*127*2 = 2540 = 0x09EC, 10 - 9 = 1 -> (1,1,1): colour 1 (d=3).
	RA_CHECK_EQ(1, t255[2]);
}

RA_TEST(fade_null_arguments) {
	unsigned char pal[768], table[256];
	Make_Palette_A(pal);
	memset(table, 0xCC, sizeof(table));
	RA_CHECK(Build_Fading_Table(0, table, 1, 128) == (void *)table);
	RA_CHECK_EQ(0xCC, table[0]);
	RA_CHECK_EQ(0xCC, table[255]);
	RA_CHECK(Build_Fading_Table(pal, 0, 1, 128) == 0);
}

RA_TEST_MAIN()
