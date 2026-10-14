// S07-C: text printing (CODE/2TXTPRNT.CPP) and font colour table (WIN32LIB/FONT/SETFPAL.CPP).
// Synthetic font; expected pixels worked out by hand from CODE/2TXTPRNT.ASM (git history).
#include "ra_test.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vector>

extern "C" {
extern unsigned char ColorXlat[256];		// defined in 2TXTPRNT.CPP
long Buffer_Print(void *thisptr, const char *str, int x, int y, int fcolor, int bcolor);
void *Get_Font_Palette_Ptr(void);
void Set_Font_Palette_Range(void const *palette, int start_idx, int end_idx);

// Normally defined in WIN32LIB/FONT/LOADFONT.CPP.
int FontXSpacing = 0;
int FontYSpacing = 0;
void const *FontPtr = NULL;
}

// First fields of GraphicViewPortClass (GBUFFER.H), as the asm saw them.
struct TestViewPort {
	long	Offset;
	int		Width;
	int		Height;
	int		XAdd;
	int		XPos;
	int		YPos;
	long	Pitch;
	void	*GraphicBuff;
};

#if defined(RA_EXPECT_32BIT)
static_assert(offsetof(TestViewPort, Pitch) == 24, "GBUFFER.INC layout");
#endif

// ---------------------------------------------------------------------------
// Synthetic font. Header: offsets of the info (4), offset (6), width (8),
// data (10) and height (12) blocks. Info block byte 4 = max height (4).
//   'A' width 3, 1 blank row on top, 2 data rows:
//        row 0 pixels 1,2,3 -> bytes 0x21 0x03 (low nibble first)
//        row 1 pixels 0,1,0 -> bytes 0x10 0x00
//   'B' width 2, 1 blank row on top, 0 data rows
//   'C' width 1, 0 blank rows, 4 data rows: pixels 2,0,1,15; the unused high
//        nibbles hold junk (0xF?, 0xE?, ...), which must be ignored.
// ---------------------------------------------------------------------------
static std::vector<unsigned char> Make_Font()
{
	const int info = 14, offs = info + 6, widths = offs + 512, heights = widths + 256, data = heights + 512;
	std::vector<unsigned char> f(data + 64, 0);
	auto put16 = [&](int at, int v) { f[at] = (unsigned char)v; f[at + 1] = (unsigned char)(v >> 8); };
	put16(0, (int)f.size());
	put16(4, info);
	put16(6, offs);
	put16(8, widths);
	put16(10, data);
	put16(12, heights);
	f[info + 4] = 4;		// max height
	f[info + 5] = 3;		// max width

	int at = data;
	// 'A'
	put16(offs + 'A' * 2, at);
	f[widths + 'A'] = 3;
	f[heights + 'A' * 2] = 1;
	f[heights + 'A' * 2 + 1] = 2;
	f[at++] = 0x21; f[at++] = 0x03;
	f[at++] = 0x10; f[at++] = 0x00;
	// 'B'
	put16(offs + 'B' * 2, at);
	f[widths + 'B'] = 2;
	f[heights + 'B' * 2] = 1;
	f[heights + 'B' * 2 + 1] = 0;
	// 'C'
	put16(offs + 'C' * 2, at);
	f[widths + 'C'] = 1;
	f[heights + 'C' * 2] = 0;
	f[heights + 'C' * 2 + 1] = 4;
	f[at++] = 0xF2; f[at++] = 0xE0; f[at++] = 0xD1; f[at++] = 0xCF;
	return f;
}

static std::vector<unsigned char> g_font = Make_Font();

enum { W = 16, H = 12, BG = 0xEE, FG = 0x0A };

struct Screen {
	unsigned char pix[W * H];
	TestViewPort vp;
	Screen() {
		memset(pix, BG, sizeof(pix));
		memset(&vp, 0, sizeof(vp));
		vp.Offset = (long)(intptr_t)pix;
		vp.Width = W;
		vp.Height = H;
	}
	long addr(int x, int y) const { return (long)(intptr_t)(pix + y * W + x); }
	unsigned char at(int x, int y) const { return pix[y * W + x]; }
};

static void Reset_Font_State(int xspacing, int yspacing)
{
	unsigned char pal[16];
	for (int i = 0; i < 16; i++) pal[i] = (unsigned char)(0x40 + i);
	Set_Font_Palette_Range(pal, 0, 15);
	FontPtr = g_font.data();
	FontXSpacing = xspacing;
	FontYSpacing = yspacing;
}

// Expected image: background plus explicit pixels.
struct Expect {
	unsigned char pix[W * H];
	Expect() { memset(pix, BG, sizeof(pix)); }
	void set(int x, int y, int c) { pix[y * W + x] = (unsigned char)c; }
};

// Expected 'A' at top-left (x, y) with the font palette 0x40+i and fg FG.
static void Expect_A(Expect &e, int x, int y, int bg)
{
	if (bg) { e.set(x, y, bg); e.set(x + 1, y, bg); e.set(x + 2, y, bg); }	// top blank
	e.set(x, y + 1, FG); e.set(x + 1, y + 1, 0x42); e.set(x + 2, y + 1, 0x43);	// 1,2,3
	if (bg) e.set(x, y + 2, bg);
	e.set(x + 1, y + 2, FG);													// 0,1,0
	if (bg) e.set(x + 2, y + 2, bg);
	if (bg) { e.set(x, y + 3, bg); e.set(x + 1, y + 3, bg); e.set(x + 2, y + 3, bg); }	// bottom blank
}

// Must run first: checks the table's initial contents.
RA_TEST(colorxlat_initial_table_and_pointer) {
	RA_CHECK(Get_Font_Palette_Ptr() == (void *)ColorXlat);
	for (int i = 0; i < 16; i++) RA_CHECK_EQ(i, ColorXlat[i]);
	for (int k = 1; k < 16; k++) RA_CHECK_EQ(k, ColorXlat[k * 16]);
	RA_CHECK_EQ(0, ColorXlat[17]);
	RA_CHECK_EQ(0, ColorXlat[239]);
}

RA_TEST(set_font_palette_range_writes_both_nibble_entries) {
	unsigned char pal[3] = { 0x81, 0x82, 0x83 };
	Set_Font_Palette_Range(pal, 3, 5);
	RA_CHECK_EQ(0x81, ColorXlat[3]);  RA_CHECK_EQ(0x81, ColorXlat[48]);
	RA_CHECK_EQ(0x82, ColorXlat[4]);  RA_CHECK_EQ(0x82, ColorXlat[64]);
	RA_CHECK_EQ(0x83, ColorXlat[5]);  RA_CHECK_EQ(0x83, ColorXlat[80]);
	RA_CHECK_EQ(6, ColorXlat[6]);     RA_CHECK_EQ(2, ColorXlat[2]);

	unsigned char other[2] = { 0x91, 0x92 };
	Set_Font_Palette_Range(other, 5, 3);			// end < start: nothing
	RA_CHECK_EQ(0x83, ColorXlat[5]);

	Set_Font_Palette_Range(other, 0x12, 0x13);		// masked to 2..3, palette read from its start
	RA_CHECK_EQ(0x91, ColorXlat[2]);  RA_CHECK_EQ(0x91, ColorXlat[32]);
	RA_CHECK_EQ(0x92, ColorXlat[3]);  RA_CHECK_EQ(0x92, ColorXlat[48]);
}

RA_TEST(print_transparent_background) {
	Reset_Font_State(1, 0);
	Screen s;
	long next = Buffer_Print(&s.vp, "A", 2, 3, FG, 0);
	Expect e;
	Expect_A(e, 2, 3, 0);
	RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	RA_CHECK(next == s.addr(2 + 3 + 1, 3));
	RA_CHECK_EQ(FG, ColorXlat[1]);
	RA_CHECK_EQ(FG, ColorXlat[16]);
	RA_CHECK_EQ(0, ColorXlat[0]);
}

RA_TEST(print_opaque_background_fills_blank_rows) {
	Reset_Font_State(1, 0);
	Screen s;
	long next = Buffer_Print(&s.vp, "A", 2, 3, FG, 7);
	Expect e;
	Expect_A(e, 2, 3, 7);
	RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	RA_CHECK(next == s.addr(6, 3));
}

RA_TEST(high_nibble_indexes_table_unshifted) {
	// The high nibble is looked up at ColorXlat[nibble << 4], not ColorXlat[nibble].
	Reset_Font_State(1, 0);
	ColorXlat[2] = 0x99;		// low-nibble entry for 2 differs from the high-nibble one (0x42)
	Screen s;
	Buffer_Print(&s.vp, "A", 0, 0, FG, 0);
	RA_CHECK_EQ(0x42, s.at(1, 1));
}

RA_TEST(zero_height_char_skips_bottom_fill) {
	// Quirk kept from the asm: with no data rows, only the top blank is filled.
	Reset_Font_State(1, 0);
	Screen s;
	long next = Buffer_Print(&s.vp, "B", 0, 0, FG, 7);
	Expect e;
	e.set(0, 0, 7); e.set(1, 0, 7);
	RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	RA_CHECK(next == s.addr(3, 0));
}

RA_TEST(odd_width_ignores_high_nibble) {
	Reset_Font_State(1, 0);
	Screen s;
	long next = Buffer_Print(&s.vp, "C", 0, 0, FG, 0);
	Expect e;
	e.set(0, 0, 0x42);		// pixel 2
	// pixel 0 at y=1: transparent
	e.set(0, 2, FG);		// pixel 1
	e.set(0, 3, 0x4F);		// pixel 15
	RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	RA_CHECK(next == s.addr(2, 0));
}

RA_TEST(newline_goes_to_left_edge_return_to_start_x) {
	Reset_Font_State(1, 1);					// line step = max height 4 + 1
	{
		Screen s;
		long next = Buffer_Print(&s.vp, "A\nA", 5, 0, FG, 0);
		Expect e;
		Expect_A(e, 5, 0, 0);
		Expect_A(e, 0, 5, 0);
		RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
		RA_CHECK(next == s.addr(0 + 4, 5));
	}
	{
		Screen s;
		long next = Buffer_Print(&s.vp, "A\rA", 5, 0, FG, 0);
		Expect e;
		Expect_A(e, 5, 0, 0);
		Expect_A(e, 5, 5, 0);
		RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
		RA_CHECK(next == s.addr(5 + 4, 5));
	}
}

RA_TEST(char_past_right_edge_forces_line_feed_to_start_x) {
	// Characters advance 4 pixels (width 3 + spacing 1). From x=4: 4, 8, 12
	// (12 + 4 = 16 still fits a 16-wide viewport), then the 4th wraps to x=4.
	Reset_Font_State(1, 0);
	Screen s;
	long next = Buffer_Print(&s.vp, "AAAA", 4, 0, FG, 0);
	Expect e;
	Expect_A(e, 4, 0, 0);
	Expect_A(e, 8, 0, 0);
	Expect_A(e, 12, 0, 0);
	Expect_A(e, 4, 4, 0);
	RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	RA_CHECK(next == s.addr(8, 4));
}

RA_TEST(overflow_returns_zero) {
	Reset_Font_State(1, 0);
	{
		Screen s;		// 9 + 4 > 12: nothing drawn
		RA_CHECK_EQ(0, Buffer_Print(&s.vp, "A", 0, 9, FG, 7));
		Expect e;
		RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	}
	{
		Screen s;		// 8 + 4 = 12 fits exactly
		RA_CHECK(Buffer_Print(&s.vp, "A", 0, 8, FG, 0) == s.addr(4, 8));
	}
	{
		Screen s;		// third line would end at 16 > 12: first two lines drawn, result 0
		RA_CHECK_EQ(0, Buffer_Print(&s.vp, "A\nA\nA", 0, 4, FG, 0));
		Expect e;
		Expect_A(e, 0, 4, 0);
		Expect_A(e, 0, 8, 0);
		RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
	}
}

RA_TEST(no_font_returns_zero_without_touching_colours) {
	Reset_Font_State(1, 0);
	ColorXlat[1] = 0x55;
	FontPtr = NULL;
	Screen s;
	RA_CHECK_EQ(0, Buffer_Print(&s.vp, "A", 0, 0, FG, 7));
	RA_CHECK_EQ(0x55, ColorXlat[1]);
	Expect e;
	RA_CHECK_MEM_EQ(e.pix, s.pix, sizeof(s.pix));
}

RA_TEST(viewport_uses_xadd_and_pitch) {
	// A 16x5 viewport at (2,1) of a 20x6 buffer: bytes per line = 16 + 2 + 2.
	Reset_Font_State(1, 0);
	unsigned char buf[20 * 6];
	memset(buf, BG, sizeof(buf));
	TestViewPort vp;
	memset(&vp, 0, sizeof(vp));
	vp.Offset = (long)(intptr_t)(buf + 1 * 20 + 2);
	vp.Width = 16;
	vp.Height = 5;
	vp.XAdd = 2;
	vp.Pitch = 2;
	long next = Buffer_Print(&vp, "A", 0, 0, FG, 0);
	unsigned char e[20 * 6];
	memset(e, BG, sizeof(e));
	e[2 * 20 + 2] = FG; e[2 * 20 + 3] = 0x42; e[2 * 20 + 4] = 0x43;
	e[3 * 20 + 3] = FG;
	RA_CHECK_MEM_EQ(e, buf, sizeof(buf));
	RA_CHECK(next == vp.Offset + 4);
}

RA_TEST_MAIN()
