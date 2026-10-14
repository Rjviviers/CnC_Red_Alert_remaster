// S07-C: software mouse cursor (WIN32LIB/KEYBOARD/WWMOUSE.CPP).
// Synthetic cursors and screens; expected results worked out by hand from WWMOUSE.ASM (git history).
#include "ra_test.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C" {
void Mouse_Shadow_Buffer(void *thisptr, void *srcdst, void *buffer, int x, int y, int hotx, int hoty, int store);
void Draw_Mouse(void *thisptr, void *srcdst, int x, int y);
void *ASM_Set_Mouse_Cursor(void *thisptr, int hotspotx, int hotspoty, void *cursor);

// --- Stand-ins for the shape library (WIN32LIB/SHAPE) and the LCW decoder (S07-D). ---
// Shape header: type(2) height(1) width(2) original height(1) size(2) data length(2) [colour table(16)].
char *_ShapeBuffer = 0;
int Get_Shape_Width(void const *shape) { const unsigned char *s = (const unsigned char *)shape; return s[3] | (s[4] << 8); }
int Get_Shape_Original_Height(void const *shape) { return ((const unsigned char *)shape)[5]; }
int Get_Shape_Uncomp_Size(void const *shape) { const unsigned char *s = (const unsigned char *)shape; return s[8] | (s[9] << 8); }

// Fake LCW: the "compressed" data in these tests is stored raw. Records the call.
static void *g_lcw_source = 0;
static void *g_lcw_dest = 0;
static unsigned long g_lcw_length = 0;
unsigned long LCW_Uncompress(void *source, void *dest, unsigned long length)
{
	g_lcw_source = source; g_lcw_dest = dest; g_lcw_length = length;
	memcpy(dest, source, length);
	return length;
}
}

// Same members as WWMouseClass (MOUSE.H) up to PrevCursor.
struct TestMouse {
	char	*MouseCursor;
	int		MouseXHot;
	int		MouseYHot;
	int		CursorWidth;
	int		CursorHeight;
	char	*MouseBuffer;
	int		MouseBuffX;
	int		MouseBuffY;
	int		MaxWidth;
	int		MaxHeight;
	int		MouseCXLeft;
	int		MouseCYUpper;
	int		MouseCXRight;
	int		MouseCYLower;
	char	MCFlags;
	char	MCCount;
	void	*Screen;
	char	*PrevCursor;
	char	rest[64];		// the remaining class members
};

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
struct PackProbe { char c; int i; };
// KEYBOARD/MOUSE.INC offsets, valid when structs are packed like Watcom /zp1.
static_assert(sizeof(PackProbe) != 5 || offsetof(TestMouse, PrevCursor) == 62, "MOUSE.INC layout");
static_assert(offsetof(TestViewPort, Pitch) == 24, "GBUFFER.INC layout");
#endif

enum { W = 8, H = 6, BG = 0x55 };

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
	unsigned char at(int x, int y) const { return pix[y * W + x]; }
};

// 3x2 cursor, hot spot (1,1):  1 0 2
//                              0 3 4
static char g_cursor[6] = { 1, 0, 2, 0, 3, 4 };

static void Init_Mouse(TestMouse &m)
{
	memset(&m, 0, sizeof(m));
	m.MouseCursor = g_cursor;
	m.MouseXHot = 1;
	m.MouseYHot = 1;
	m.CursorWidth = 3;
	m.CursorHeight = 2;
	m.MaxWidth = 4;
	m.MaxHeight = 4;
}

static int Count_Changed(const Screen &s)
{
	int n = 0;
	for (int i = 0; i < W * H; i++) n += s.pix[i] != BG;
	return n;
}

RA_TEST(draw_mouse_unclipped_with_transparency) {
	TestMouse m; Init_Mouse(m);
	Screen s;
	Draw_Mouse(&m, &s.vp, 4, 3);			// top-left (3,2)
	RA_CHECK_EQ(1, s.at(3, 2));
	RA_CHECK_EQ(BG, s.at(4, 2));
	RA_CHECK_EQ(2, s.at(5, 2));
	RA_CHECK_EQ(BG, s.at(3, 3));
	RA_CHECK_EQ(3, s.at(4, 3));
	RA_CHECK_EQ(4, s.at(5, 3));
	RA_CHECK_EQ(4, Count_Changed(s));
}

RA_TEST(draw_mouse_clipped_left_top) {
	TestMouse m; Init_Mouse(m);
	Screen s;
	Draw_Mouse(&m, &s.vp, 0, 0);			// top-left (-1,-1): only cursor (1,1),(2,1) visible
	RA_CHECK_EQ(3, s.at(0, 0));
	RA_CHECK_EQ(4, s.at(1, 0));
	RA_CHECK_EQ(2, Count_Changed(s));
}

RA_TEST(draw_mouse_clipped_right_bottom) {
	TestMouse m; Init_Mouse(m);
	Screen s;
	Draw_Mouse(&m, &s.vp, 8, 6);			// top-left (7,5): only cursor (0,0) visible
	RA_CHECK_EQ(1, s.at(7, 5));
	RA_CHECK_EQ(1, Count_Changed(s));
}

RA_TEST(draw_mouse_fully_off_screen) {
	TestMouse m; Init_Mouse(m);
	Screen s;
	Draw_Mouse(&m, &s.vp, 20, 1);			// both corners right of the screen
	Draw_Mouse(&m, &s.vp, 9, 3);			// left edge at x = width: zero visible columns
	Draw_Mouse(&m, &s.vp, -5, -5);			// both corners above/left
	RA_CHECK_EQ(0, Count_Changed(s));
}

RA_TEST(draw_mouse_respects_pitch) {
	// 8x6 viewport at (1,1) of a 12x8 buffer: bytes per line = 8 + XAdd 2 + Pitch 2.
	TestMouse m; Init_Mouse(m);
	unsigned char buf[12 * 8];
	memset(buf, BG, sizeof(buf));
	TestViewPort vp;
	memset(&vp, 0, sizeof(vp));
	vp.Offset = (long)(intptr_t)(buf + 12 + 1);
	vp.Width = 8; vp.Height = 6; vp.XAdd = 2; vp.Pitch = 2;
	Draw_Mouse(&m, &vp, 1, 1);				// top-left (0,0) of the viewport
	RA_CHECK_EQ(1, buf[1 * 12 + 1]);
	RA_CHECK_EQ(2, buf[1 * 12 + 3]);
	RA_CHECK_EQ(3, buf[2 * 12 + 2]);
	RA_CHECK_EQ(4, buf[2 * 12 + 3]);
	RA_CHECK_EQ(BG, buf[2 * 12 + 1]);
}

RA_TEST(shadow_buffer_store_and_restore) {
	TestMouse m; Init_Mouse(m);
	Screen s;
	for (int i = 0; i < W * H; i++) s.pix[i] = (unsigned char)i;
	unsigned char save[6];
	memset(save, 0xAA, sizeof(save));
	Mouse_Shadow_Buffer(&m, &s.vp, save, 4, 3, 1, 1, 1);		// store rect (3,2)-(5,3)
	unsigned char expect[6] = { 19, 20, 21, 27, 28, 29 };
	RA_CHECK_MEM_EQ(expect, save, 6);

	Screen before = s;
	Draw_Mouse(&m, &s.vp, 4, 3);
	RA_CHECK(memcmp(before.pix, s.pix, sizeof(s.pix)) != 0);
	Mouse_Shadow_Buffer(&m, &s.vp, save, 4, 3, 1, 1, 0);		// restore
	RA_CHECK_MEM_EQ(before.pix, s.pix, sizeof(s.pix));

	// Any store value other than 1 restores (the asm compares with 1).
	Draw_Mouse(&m, &s.vp, 4, 3);
	Mouse_Shadow_Buffer(&m, &s.vp, save, 4, 3, 1, 1, 2);
	RA_CHECK_MEM_EQ(before.pix, s.pix, sizeof(s.pix));
}

RA_TEST(shadow_buffer_clipped_keeps_buffer_positions) {
	TestMouse m; Init_Mouse(m);
	Screen s;
	for (int i = 0; i < W * H; i++) s.pix[i] = (unsigned char)i;
	unsigned char save[6];
	memset(save, 0xAA, sizeof(save));
	Mouse_Shadow_Buffer(&m, &s.vp, save, 0, 0, 1, 1, 1);		// top-left (-1,-1)
	unsigned char expect[6] = { 0xAA, 0xAA, 0xAA, 0xAA, 0, 1 };	// buffer row 1, columns 1-2
	RA_CHECK_MEM_EQ(expect, save, 6);
}

// ---------------------------------------------------------------------------
// ASM_Set_Mouse_Cursor
// ---------------------------------------------------------------------------

// Uncompressed 3x2 shape (type 2 = NOCOMP): RLE 05, 00 02 (two zeros), 06 07 08.
static unsigned char g_shape_raw[] = {
	2, 0,  2,  3, 0,  2,  16, 0,  6, 0,
	0x05, 0x00, 0x02, 0x06, 0x07, 0x08
};

// Same image as a 16-colour shape (type 3 = NOCOMP|COMPACT), colour table 0xA0+i:
// RLE 01, 00 01, 02 0F 03 04  ->  A1 00 A2 AF A3 A4
static unsigned char g_shape_compact[] = {
	3, 0,  2,  3, 0,  2,  33, 0,  7, 0,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0x01, 0x00, 0x01, 0x02, 0x0F, 0x03, 0x04
};

// Compressed 16-colour shape (type 1 = COMPACT): staged in _ShapeBuffer, data "LCW" (raw here).
static unsigned char g_shape_lcw[] = {
	1, 0,  2,  3, 0,  2,  33, 0,  7, 0,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0x01, 0x00, 0x01, 0x02, 0x0F, 0x03, 0x04
};

struct CursorFixture {
	TestMouse m;
	char image[16];
	CursorFixture() {
		memset(&m, 0, sizeof(m));
		memset(image, 0x77, sizeof(image));
		m.MouseCursor = image;
		m.MaxWidth = 4;
		m.MaxHeight = 4;
		m.PrevCursor = (char *)0x1234;
	}
};

RA_TEST(set_cursor_uncompressed) {
	CursorFixture f;
	void *old = ASM_Set_Mouse_Cursor(&f.m, 2, 1, g_shape_raw);
	RA_CHECK(old == (void *)0x1234);
	RA_CHECK(f.m.PrevCursor == (char *)g_shape_raw);
	unsigned char expect[7] = { 5, 0, 0, 6, 7, 8, 0x77 };
	RA_CHECK_MEM_EQ(expect, f.image, 7);
	RA_CHECK_EQ(2, f.m.MouseXHot);
	RA_CHECK_EQ(1, f.m.MouseYHot);
	RA_CHECK_EQ(3, f.m.CursorWidth);
	RA_CHECK_EQ(2, f.m.CursorHeight);
}

RA_TEST(set_cursor_compact_remaps) {
	CursorFixture f;
	ASM_Set_Mouse_Cursor(&f.m, 0, 0, g_shape_compact);
	unsigned char expect[7] = { 0xA1, 0, 0xA2, 0xAF, 0xA3, 0xA4, 0x77 };
	RA_CHECK_MEM_EQ(expect, f.image, 7);
}

RA_TEST(set_cursor_compressed_is_staged_in_shape_buffer) {
	CursorFixture f;
	char staging[64];
	memset(staging, 0, sizeof(staging));
	_ShapeBuffer = staging;
	ASM_Set_Mouse_Cursor(&f.m, 0, 0, g_shape_lcw);
	unsigned char expect[7] = { 0xA1, 0, 0xA2, 0xAF, 0xA3, 0xA4, 0x77 };
	RA_CHECK_MEM_EQ(expect, f.image, 7);
	RA_CHECK_EQ(3, (unsigned char)staging[0]);		// type | NOCOMP
	RA_CHECK_EQ(0, staging[1]);
	RA_CHECK_MEM_EQ(g_shape_lcw + 2, staging + 2, 8 + 16);	// header + colour table
	RA_CHECK(g_lcw_source == (void *)(g_shape_lcw + 26));
	RA_CHECK(g_lcw_dest == (void *)(staging + 26));
	RA_CHECK_EQ(7, (long)g_lcw_length);					// Get_Shape_Uncomp_Size
	_ShapeBuffer = 0;
}

RA_TEST(set_cursor_too_big_only_swaps_prev_cursor) {
	CursorFixture f;
	f.m.MaxWidth = 2;							// shape is 3 wide
	f.m.CursorWidth = 9;
	void *old = ASM_Set_Mouse_Cursor(&f.m, 5, 5, g_shape_raw);
	RA_CHECK(old == (void *)0x1234);
	RA_CHECK(f.m.PrevCursor == (char *)g_shape_raw);
	RA_CHECK_EQ(9, f.m.CursorWidth);
	RA_CHECK_EQ(0, f.m.MouseXHot);
	RA_CHECK_EQ(0x77, (unsigned char)f.image[0]);

	CursorFixture g;
	g.m.MaxHeight = 1;							// shape is 2 high
	ASM_Set_Mouse_Cursor(&g.m, 5, 5, g_shape_raw);
	RA_CHECK_EQ(0x77, (unsigned char)g.image[0]);
	RA_CHECK_EQ(0, g.m.CursorHeight);
}

RA_TEST_MAIN()
