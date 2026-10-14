// Golden tests for the shape renderer (S07-B): CODE/SHAPEDRW.CPP, the port of CODE/2KEYFBUF.ASM.
//
// Every expected buffer below was worked out by hand from the assembly, not by running the C++.
// "asm NNNN" refers to a line of 2KEYFBUF.ASM as it was before S07-B deleted it
// (git show 66de4c5:CODE/2KEYFBUF.ASM).
//
// Common fixture (see Page and the tables below):
//   * The page is 8 bytes wide and 8 rows high; the viewport is Width 8, Height 6, XAdd 0, Pitch 0,
//     so one row is 8 bytes. Rows 6-7 are padding that the predator effect may read.
//   * Background colour at (row r, column c) = 0x20 + 0x10*r + c, e.g. row 1 = 30 31 .. 37.
//   * Ghost table: is_translucent[] is 0xFF except [0x05]=0, [0x06]=1, [0x36]=0;
//     translucent row 0 maps dest d -> 0xA0|(d&15), row 1 maps d -> 0xB0|(d&15).
//   * Fade table FadeInc[v] = v+1, so fading N times adds N.
//   * Unless a test says otherwise the 3x2 shape is   01 00 05
//                                                    06 07 00   drawn at x=2, y=1.
#include "ra_test.h"
#include "SHAPEDRW.H"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <initializer_list>

extern "C" {
bool UseOldShapeDraw = false;
int UseBigShapeBuffer = 0;
char *BigShapeBufferStart = 0;
char *TheaterShapeBufferStart = 0;
}

namespace {

typedef unsigned char u8;

enum {
    CENTER = BF_SHAPE_CENTER, TRANS = BF_SHAPE_TRANS, FADING = BF_SHAPE_FADING,
    PREDATOR = BF_SHAPE_PREDATOR, GHOST = BF_SHAPE_GHOST, PARTIAL = BF_SHAPE_PARTIAL
};

const int PW = 8;    // page row length
const int PR = 8;    // page rows

u8 Background(int r, int c) { return (u8)(0x20 + 0x10 * r + c); }

struct Page {
    u8 mem[PW * PR];
    ShapeViewType view;
    Page() {
        for (int r = 0; r < PR; r++)
            for (int c = 0; c < PW; c++) mem[r * PW + c] = Background(r, c);
        view.Offset = mem;
        view.Width = 8;
        view.Height = 6;
        view.XAdd = 0;
        view.Pitch = 0;
    }
};

struct Px { int r, c, v; };

// Checks the whole page: the listed pixels must have the listed value, every other one its background.
void Expect_Page(Page const &p, std::initializer_list<Px> changes, int line)
{
    u8 expected[PW * PR];
    for (int r = 0; r < PR; r++)
        for (int c = 0; c < PW; c++) expected[r * PW + c] = Background(r, c);
    for (Px const &x : changes) expected[x.r * PW + x.c] = (u8)x.v;
    for (int i = 0; i < PW * PR; i++) {
        if (expected[i] != p.mem[i]) {
            char msg[128];
            snprintf(msg, sizeof(msg), "pixel row %d col %d: expected %02X got %02X", i / PW, i % PW,
                     expected[i], p.mem[i]);
            ra_test::fail(__FILE__, line, msg);
        }
    }
}
#define EXPECT_PAGE(page, ...) Expect_Page(page, __VA_ARGS__, __LINE__)

u8 Ghost[256 * 4];
u8 FadeInc[256];
u8 FadeCycle[256];

void Init_Tables()
{
    memset(Ghost, 0xFF, sizeof(Ghost));
    Ghost[0x05] = 0;
    Ghost[0x06] = 1;
    Ghost[0x36] = 0;
    for (int d = 0; d < 256; d++) {
        Ghost[256 + d] = (u8)(0xA0 | (d & 15));
        Ghost[512 + d] = (u8)(0xB0 | (d & 15));
        FadeInc[d] = (u8)(d + 1);
        FadeCycle[d] = 0x10;    // everything falls into the cycle 10 -> 11 -> 12 -> 10
    }
    FadeCycle[0x10] = 0x11;
    FadeCycle[0x11] = 0x12;
    FadeCycle[0x12] = 0x10;
}

const u8 Shape3x2[6] = {0x01, 0x00, 0x05,
                        0x06, 0x07, 0x00};

// Old system (no line headers): plain pixel pointer.
void Old_System()
{
    Init_Tables();
    UseOldShapeDraw = false;
    UseBigShapeBuffer = 0;
    BigShapeBufferStart = 0;
    TheaterShapeBufferStart = 0;
}

long Draw(int x, int y, int w, int h, void const *src, ShapeViewType const &view, int flags, ...)
{
    va_list args;
    va_start(args, flags);
    long result = Buffer_Frame_To_View(x, y, w, h, (void *)src, view, flags, args);
    va_end(args);
    return result;
}

// Big shape buffer: frame = ShapeFrameHeaderType + one line byte per row + pixels (CODE/2KEYFRAM.CPP).
u8 BigBuffer[8192];
u8 TheaterBuffer[1024];

void New_System()
{
    Init_Tables();
    memset(BigBuffer, 0, sizeof(BigBuffer));
    memset(TheaterBuffer, 0, sizeof(TheaterBuffer));
    UseOldShapeDraw = false;
    UseBigShapeBuffer = 1;
    BigShapeBufferStart = (char *)BigBuffer;
    TheaterShapeBufferStart = (char *)TheaterBuffer;
}

ShapeFrameHeaderType *Make_Frame(u8 *base, size_t at, int w, int h, u8 const *pixels, int theater = 0)
{
    ShapeFrameHeaderType *hdr = (ShapeFrameHeaderType *)(base + at);
    size_t data = at + sizeof(ShapeFrameHeaderType) + (size_t)h;
    hdr->draw_flags = 0xFFFFFFFFu;    // "headers need to be generated"
    hdr->shape_data = (char *)(uintptr_t)data;
    hdr->shape_buffer = theater;
    memcpy(base + data, pixels, (size_t)(w * h));
    return hdr;
}

u8 const *Line_Headers(ShapeFrameHeaderType const *hdr) { return (u8 const *)hdr + sizeof(ShapeFrameHeaderType); }

} // namespace

/* ======================================================================== */
/* Old system: BF_* routines (one per jflags value, BufferFrameTable 793-844) */
/* ======================================================================== */

// BF_Copy (asm 1567-1602): rep movsb/movsd, zero is copied too.
RA_TEST(bf_copy) {
    Old_System();
    Page p;
    RA_CHECK_EQ(0, Draw(2, 1, 3, 2, Shape3x2, p.view, 0));
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 3, 0x00}, {1, 4, 0x05}, {2, 2, 0x06}, {2, 3, 0x07}, {2, 4, 0x00}});
}

// BF_Copy, wide (>= 10 bytes uses the dword loop, asm 1569-1591); same bytes.
RA_TEST(bf_copy_wide) {
    Old_System();
    u8 mem[20 * 2];
    memset(mem, 0xEE, sizeof(mem));
    ShapeViewType v = {mem, 20, 2, 0, 0};
    u8 src[17];
    for (int i = 0; i < 17; i++) src[i] = (u8)(i + 1);
    Draw(1, 1, 17, 1, src, v, 0);
    u8 expected[40];
    memset(expected, 0xEE, sizeof(expected));
    for (int i = 0; i < 17; i++) expected[20 + 1 + i] = (u8)(i + 1);
    RA_CHECK_MEM_EQ(expected, mem, sizeof(mem));
}

// BF_Trans (asm 2686-2697): "test bl,bl / jz trans_pixel" skips colour 0.
RA_TEST(bf_trans) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 4, 0x05}, {2, 2, 0x06}, {2, 3, 0x07}});
}

// BF_Ghost (asm 2743-2764): is_trans[src]==FF -> src, else translucent[g][dest]. 0 is drawn (is_trans[0]=FF).
// 05 on dest 34 -> row0 -> A4; 06 on dest 42 -> row1 -> B2.
RA_TEST(bf_ghost) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, GHOST, Ghost);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 3, 0x00}, {1, 4, 0xA4}, {2, 2, 0xB2}, {2, 3, 0x07}, {2, 4, 0x00}});
}

// BF_Ghost_Trans (asm 2811-2838).
RA_TEST(bf_ghost_trans) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | GHOST, Ghost);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 4, 0xA4}, {2, 2, 0xB2}, {2, 3, 0x07}});
}

// BF_Fading (asm 2890-2904): FadingNum lookups per pixel, colour 0 included. Count 1: +1.
RA_TEST(bf_fading) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, FADING, FadeInc, 1);
    EXPECT_PAGE(p, {{1, 2, 0x02}, {1, 3, 0x01}, {1, 4, 0x06}, {2, 2, 0x07}, {2, 3, 0x08}, {2, 4, 0x01}});
}

// BF_Fading_Trans (asm 2957-2977), count 3: +3, colour 0 skipped.
RA_TEST(bf_fading_trans_count3) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | FADING, FadeInc, 3);
    EXPECT_PAGE(p, {{1, 2, 0x04}, {1, 4, 0x08}, {2, 2, 0x09}, {2, 3, 0x0A}});
}

// Fade count is "and eax,03fh" (asm 1337): 65 -> 1, 64 -> 0. Count 0 runs "dec ecx / jnz" 2^32 times.
// FadeCycle: 10->11->12->10, everything else -> 10.  2^32 mod 3 = 1, so 10 -> 11;
// 05 -> 10 after one step, then 2^32-1 (divisible by 3) more steps -> 10.
RA_TEST(bf_fading_count_masks_and_zero_means_2pow32) {
    Old_System();
    const u8 src[2] = {0x10, 0x05};
    Page p;
    Draw(2, 1, 2, 1, src, p.view, FADING, FadeInc, 65);
    EXPECT_PAGE(p, {{1, 2, 0x11}, {1, 3, 0x06}});
    Page q;
    Draw(2, 1, 2, 1, src, q.view, FADING, FadeCycle, 64);
    EXPECT_PAGE(q, {{1, 2, 0x11}, {1, 3, 0x10}});
    Page z;
    Draw(2, 1, 2, 1, src, z.view, FADING, FadeCycle, 0);
    EXPECT_PAGE(z, {{1, 2, 0x11}, {1, 3, 0x10}});
}

// BF_Ghost_Fading (asm 3029-3062): ghost first, then fade the result (the "jmp store_pixel" is commented out).
// 01->02, 00->01, 05->A4->A5 / 06->B2->B3, 07->08, 00->01.
RA_TEST(bf_ghost_fading) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, GHOST | FADING, Ghost, FadeInc, 1);
    EXPECT_PAGE(p, {{1, 2, 0x02}, {1, 3, 0x01}, {1, 4, 0xA5}, {2, 2, 0xB3}, {2, 3, 0x08}, {2, 4, 0x01}});
}

// BF_Ghost_Fading_Trans (asm 3114-3153).
RA_TEST(bf_ghost_fading_trans) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | GHOST | FADING, Ghost, FadeInc, 1);
    EXPECT_PAGE(p, {{1, 2, 0x02}, {1, 4, 0xA5}, {2, 2, 0xB3}, {2, 3, 0x08}});
}

// BF_Predator (asm 3201-3236). Predator arg 0: BFPredOffset = 0 (asm 1364-1382), BFPartialPred = 0x100,
// so every pixel is a predator pixel. Source is ignored; dest = dest[+BFPredTable[i]] with
// BFPredTable = 1,3,2,5,2,3,4,1 (asm 178), i advancing by one per predator pixel.
// r1c2<-r1c3(33) r1c3<-r1c6(36) r1c4<-r1c6(36) r2c2<-r2c7(47) r2c3<-r2c5(45) r2c4<-r2c7(47).
RA_TEST(bf_predator) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, PREDATOR, 0);
    EXPECT_PAGE(p, {{1, 2, 0x33}, {1, 3, 0x36}, {1, 4, 0x36}, {2, 2, 0x47}, {2, 3, 0x45}, {2, 4, 0x47}});
}

// BF_Predator_Trans (asm 3283-3317): transparent pixels don't advance the predator state.
// r1c2(01)<-r1c3=33; r1c4(05)<-+3=r1c7=37; r2c2(06)<-+2=r2c4=44; r2c3(07)<-+5 = linear 19+5=24 = r3c0 = 50.
RA_TEST(bf_predator_trans) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | PREDATOR, 0);
    EXPECT_PAGE(p, {{1, 2, 0x33}, {1, 4, 0x37}, {2, 2, 0x44}, {2, 3, 0x50}});
}

// Predator arg 3: (3*2)&0Eh = 6 -> start at table entry 3 (5,2,3,4,1,1).
RA_TEST(bf_predator_start_offset) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, PREDATOR, 3);
    EXPECT_PAGE(p, {{1, 2, 0x37}, {1, 3, 0x35}, {1, 4, 0x37}, {2, 2, 0x46}, {2, 3, 0x44}, {2, 4, 0x45}});
}

// Negative predator arg (asm 1370-1375): -1 -> -2 -> neg 2 -> BFPredOffset = FFFFFF02, which reads the
// computed table BFPredNegTable[i] = {-1,-3,-2,-5,-2,-4,-3,-1}[i] + Width + XAdd + Pitch (asm 1386-1398).
// Row length 8 -> {7,5,6,3,6,4,5,7}, starting at entry 1.
// r1c2(01)<-+5=r1c7=37; r1c4(05)<-+6=linear 18=r2c2=42; r2c2(06)<-+3=r2c5=45; r2c3(07)<-+6=linear 25=r3c1=51.
RA_TEST(bf_predator_negative_offset) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | PREDATOR, -1);
    EXPECT_PAGE(p, {{1, 2, 0x37}, {1, 4, 0x42}, {2, 2, 0x45}, {2, 3, 0x51}});

    // Same memory seen as Width 6, XAdd 1, Pitch 1: the row is still 8 bytes, so the same table and result
    // (the table adds GVPXAdd and GVPPitch, asm 1393-1394).
    Page q;
    q.view.Width = 6;
    q.view.XAdd = 1;
    q.view.Pitch = 1;
    Draw(2, 1, 3, 2, Shape3x2, q.view, TRANS | PREDATOR, -1);
    EXPECT_PAGE(q, {{1, 2, 0x37}, {1, 4, 0x42}, {2, 2, 0x45}, {2, 3, 0x51}});
}

// Predator arg 0x40000000: "shl eax,1" gives 80000000h, negative -> neg -> &0Eh = 0 -> FFFFFF00, entry 0 = 7.
// r1c2 <- linear 10+7 = 17 = r2c1 = 41.
RA_TEST(bf_predator_shift_overflow_selects_negative_table) {
    Old_System();
    const u8 one = 0x09;
    Page p;
    Draw(2, 1, 1, 1, &one, p.view, PREDATOR, 0x40000000);
    EXPECT_PAGE(p, {{1, 2, 0x41}});
}

// SHAPE_PARTIAL 0x80 (asm 1404-1410): count += 0x80 per pixel, a predator pixel every second time.
// BF_Predator (no TRANS) skips non-predator pixels: pixels 2, 4, 6 are predator pixels with offsets 1,3,2.
// r1c3<-r1c4=34, r2c2<-r2c5=45, r2c4<-r2c6=46; r1c2, r1c4, r2c3 keep the background.
RA_TEST(bf_predator_partial) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, PREDATOR | PARTIAL, 0, 0x80);
    EXPECT_PAGE(p, {{1, 3, 0x34}, {2, 2, 0x45}, {2, 4, 0x46}});
}

// BF_Predator_Trans + PARTIAL: a non-predator pixel jumps to store_pixel and draws the SOURCE colour
// (asm 3297-3298, unlike BF_Predator). 01 -> 01; 05 pred +1 -> r1c5=35; 06 -> 06; 07 pred +3 -> r2c6=46.
RA_TEST(bf_predator_trans_partial_draws_source) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | PREDATOR | PARTIAL, 0, 0x80);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 4, 0x35}, {2, 2, 0x06}, {2, 3, 0x46}});
}

// BF_Predator_Ghost (asm 3364-3407): the predator colour then goes through the ghost table.
// Offsets as bf_predator; r1c3 and r1c4 pick up 36 which is translucent (row 0): A3 (dest 33), A4 (dest 34).
RA_TEST(bf_predator_ghost) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, PREDATOR | GHOST, Ghost, 0);
    EXPECT_PAGE(p, {{1, 2, 0x33}, {1, 3, 0xA3}, {1, 4, 0xA4}, {2, 2, 0x47}, {2, 3, 0x45}, {2, 4, 0x47}});
}

// BF_Predator_Ghost_Trans + PARTIAL: non-predator pixels ghost their source colour.
// 01 -> 01; 05 pred +1 -> 35; 06 -> row1[42] = B2; 07 pred +3 -> r2c6 = 46.
RA_TEST(bf_predator_ghost_trans_partial) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | PREDATOR | GHOST | PARTIAL, Ghost, 0, 0x80);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 4, 0x35}, {2, 2, 0xB2}, {2, 3, 0x46}});
}

// BF_Predator_Fading (asm 3554-3591): predator colour, then faded. Offsets advance with ADD here.
RA_TEST(bf_predator_fading) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, PREDATOR | FADING, FadeInc, 1, 0);
    EXPECT_PAGE(p, {{1, 2, 0x34}, {1, 3, 0x37}, {1, 4, 0x37}, {2, 2, 0x48}, {2, 3, 0x46}, {2, 4, 0x48}});
}

// BF_Predator_Fading_Trans + PARTIAL (asm 3643-3686): non-predator pixels fade the source.
// 01 -> 02; 05 pred +1 -> 35 -> 36; 06 -> 07; 07 pred +3 -> 46 -> 47.
RA_TEST(bf_predator_fading_trans_partial) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | PREDATOR | FADING | PARTIAL, FadeInc, 1, 0, 0x80);
    EXPECT_PAGE(p, {{1, 2, 0x02}, {1, 4, 0x36}, {2, 2, 0x07}, {2, 3, 0x47}});
}

// BF_Predator_Ghost_Fading (asm 3738-3793): predator, ghost, fade.
// r1c2: 33->34; r1c3: 36 -> row0[33]=A3 -> A4; r1c4: 36 -> row0[34]=A4 -> A5; r2: 47->48, 45->46, 47->48.
RA_TEST(bf_predator_ghost_fading) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, PREDATOR | GHOST | FADING, Ghost, FadeInc, 1, 0);
    EXPECT_PAGE(p, {{1, 2, 0x34}, {1, 3, 0xA4}, {1, 4, 0xA5}, {2, 2, 0x48}, {2, 3, 0x46}, {2, 4, 0x48}});
}

// BF_Predator_Ghost_Fading_Trans (asm 3845-3906), with and without PARTIAL.
// No partial: 01 <- +1 = 33 -> 34; 05 <- +3 = 37 -> 38; 06 <- +2 = 44 -> 45; 07 <- +5 = r3c0 = 50 -> 51.
// Partial 0x80: 01 -> 02; 05 <- +1 = 35 -> 36; 06 -> row1[42] = B2 -> B3; 07 <- +3 = 46 -> 47.
RA_TEST(bf_predator_ghost_fading_trans) {
    Old_System();
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS | PREDATOR | GHOST | FADING, Ghost, FadeInc, 1, 0);
    EXPECT_PAGE(p, {{1, 2, 0x34}, {1, 4, 0x38}, {2, 2, 0x45}, {2, 3, 0x51}});
    Page q;
    Draw(2, 1, 3, 2, Shape3x2, q.view, TRANS | PREDATOR | GHOST | FADING | PARTIAL, Ghost, FadeInc, 1, 0, 0x80);
    EXPECT_PAGE(q, {{1, 2, 0x02}, {1, 4, 0x36}, {2, 2, 0xB3}, {2, 3, 0x47}});
}

/* ======================================================================== */
/* Arguments, centring, null source                                         */
/* ======================================================================== */

// src == 0 returns at once (asm 1181-1182).
RA_TEST(null_source_draws_nothing) {
    Old_System();
    Page p;
    RA_CHECK_EQ(0, Draw(2, 1, 3, 2, 0, p.view, TRANS));
    EXPECT_PAGE(p, {});
}

// SHAPE_CENTER (asm 1226-1234): x -= w>>1, y -= h>>1. (3,2) with 3x2 -> (2,1).
RA_TEST(center_flag) {
    Old_System();
    Page p;
    Draw(3, 2, 3, 2, Shape3x2, p.view, CENTER | TRANS);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 4, 0x05}, {2, 2, 0x06}, {2, 3, 0x07}});
}

/* ======================================================================== */
/* Clipping (asm 1420-1492): Cohen-Sutherland codes from sign bits.         */
/* A clipped shape always uses the old BF_* routines (asm 1462).            */
/* Shape for these tests:  01 02 03                                         */
/*                         04 05 06   (BF_Copy)                             */
/* ======================================================================== */

namespace {
const u8 Clip3x2[6] = {1, 2, 3, 4, 5, 6};
}

// Left: x=-1 -> scr_x=1, x=0, x1=2.
RA_TEST(clip_left) {
    Old_System();
    Page p;
    Draw(-1, 1, 3, 2, Clip3x2, p.view, 0);
    EXPECT_PAGE(p, {{1, 0, 2}, {1, 1, 3}, {2, 0, 5}, {2, 1, 6}});
}

// Right: x=6, x1=9 > Width 8 -> x1=8; the source skips 1 byte per row (scr_adjust_width).
RA_TEST(clip_right) {
    Old_System();
    Page p;
    Draw(6, 1, 3, 2, Clip3x2, p.view, 0);
    EXPECT_PAGE(p, {{1, 6, 1}, {1, 7, 2}, {2, 6, 4}, {2, 7, 5}});
}

// Top: y=-1 -> scr_y=1, y=0, one row left.
RA_TEST(clip_top) {
    Old_System();
    Page p;
    Draw(2, -1, 3, 2, Clip3x2, p.view, 0);
    EXPECT_PAGE(p, {{0, 2, 4}, {0, 3, 5}, {0, 4, 6}});
}

// Bottom: y=5, y1=7 > Height 6 -> y1=6; row 6 (outside the viewport) is untouched.
RA_TEST(clip_bottom) {
    Old_System();
    Page p;
    Draw(2, 5, 3, 2, Clip3x2, p.view, 0);
    EXPECT_PAGE(p, {{5, 2, 1}, {5, 3, 2}, {5, 4, 3}});
}

// Corners.
RA_TEST(clip_corners) {
    Old_System();
    Page a;
    Draw(-2, -1, 3, 2, Clip3x2, a.view, 0);    // top-left: only pixel 06
    EXPECT_PAGE(a, {{0, 0, 6}});
    Page b;
    Draw(7, 5, 3, 2, Clip3x2, b.view, 0);      // bottom-right: only pixel 01
    EXPECT_PAGE(b, {{5, 7, 1}});
    Page c;
    Draw(7, -1, 3, 2, Clip3x2, c.view, 0);     // top-right: only pixel 04
    EXPECT_PAGE(c, {{0, 7, 4}});
    Page d;
    Draw(-2, 5, 3, 2, Clip3x2, d.view, 0);     // bottom-left: only pixel 03
    EXPECT_PAGE(d, {{5, 0, 3}});
}

// Exactly touching the right and bottom edges is not clipped (x1 > Width is the test, asm 1436, 1453).
RA_TEST(clip_exact_fit) {
    Old_System();
    Page p;
    Draw(5, 4, 3, 2, Clip3x2, p.view, 0);
    EXPECT_PAGE(p, {{4, 5, 1}, {4, 6, 2}, {4, 7, 3}, {5, 5, 4}, {5, 6, 5}, {5, 7, 6}});
}

// Fully outside on each side: either trivially rejected (test dl,cl) or clipped to an empty rectangle
// (asm 1545-1549). x=-3 / x=8 / y=-2 / y=6 are the empty-rectangle cases, the others are rejected.
RA_TEST(clip_fully_outside) {
    Old_System();
    const int xs[] = {-3, -4, 8, 9, 2, 2, 2, 2};
    const int ys[] = {1, 1, 1, 1, -2, -3, 6, 7};
    for (int i = 0; i < 8; i++) {
        Page p;
        Draw(xs[i], ys[i], 3, 2, Clip3x2, p.view, 0);
        EXPECT_PAGE(p, {});
    }
}

// A viewport inside a bigger buffer: Offset at row 1 col 1, Width 5, Height 3, XAdd 3 (row = 8 bytes).
// Shape at (3,2): x1=6 > 5 -> 2 columns, y1=4 > 3 -> 1 row; first pixel at Offset + 2*8 + 3 = r3c4.
RA_TEST(clip_subviewport_with_xadd) {
    Old_System();
    Page p;
    p.view.Offset = p.mem + PW + 1;
    p.view.Width = 5;
    p.view.Height = 3;
    p.view.XAdd = 3;
    Draw(3, 2, 3, 2, Clip3x2, p.view, 0);
    EXPECT_PAGE(p, {{3, 4, 1}, {3, 5, 2}});
}

// Clipping with transparency uses BF_Trans on the clipped rectangle.
RA_TEST(clip_left_trans) {
    Old_System();
    Page p;
    Draw(-1, 1, 3, 2, Shape3x2, p.view, TRANS);    // visible columns: 00 05 / 07 00
    EXPECT_PAGE(p, {{1, 1, 0x05}, {2, 0, 0x07}});
}

/* ======================================================================== */
/* New system: big shape buffer with line headers (asm 1195-1210, 1263-1324, */
/* Setup_Shape_Header 941-1082, Single_Line_* 1612-2652).                    */
/* ======================================================================== */

// Setup_Shape_Header + C, K, g, G. Flags TRANS|GHOST, 4x4 at (2,0):
//   01 02 03 04 -> header 00 (copy)             -> 01 02 03 04
//   00 00 00 00 -> header 10h (skip)            -> unchanged
//   05 00 07 01 -> header 03 (trans+ghost: 'g') -> row0[42]=A2, keep 43, 07, 01
//   06 07 01 02 -> header 02 (ghost: 'G')       -> row1[52]=B2, 07, 01, 02
RA_TEST(new_system_headers_copy_skip_ghost) {
    New_System();
    const u8 pixels[16] = {1, 2, 3, 4, 0, 0, 0, 0, 5, 0, 7, 1, 6, 7, 1, 2};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 64, 4, 4, pixels);
    Page p;
    Draw(2, 0, 4, 4, hdr, p.view, TRANS | GHOST, Ghost);
    RA_CHECK_EQ(0x1040u, hdr->draw_flags);
    const u8 expected_headers[4] = {0x00, 0x10, 0x03, 0x02};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 4);
    EXPECT_PAGE(p, {{0, 2, 1}, {0, 3, 2}, {0, 4, 3}, {0, 5, 4},
                    {2, 2, 0xA2}, {2, 4, 7}, {2, 5, 1},
                    {3, 2, 0xB2}, {3, 3, 7}, {3, 4, 1}, {3, 5, 2}});

    // Second draw with the same flags: draw_flags matches (asm 1273-1276), headers are reused through
    // jump table 3 ("CTGg..."), same picture.
    Page q;
    Draw(2, 0, 4, 4, hdr, q.view, TRANS | GHOST, Ghost);
    RA_CHECK_MEM_EQ(p.mem, q.mem, sizeof(p.mem));

    // Different flags: draw_flags 1040h != 40h -> old system BF_Trans on the raw pixels (asm 1277-1278).
    Page r;
    Draw(2, 0, 4, 4, hdr, r.view, TRANS);
    RA_CHECK_EQ(0x1040u, hdr->draw_flags);    // not rebuilt
    EXPECT_PAGE(r, {{0, 2, 1}, {0, 3, 2}, {0, 4, 3}, {0, 5, 4},
                    {2, 2, 5}, {2, 4, 7}, {2, 5, 1},
                    {3, 2, 6}, {3, 3, 7}, {3, 4, 1}, {3, 5, 2}});
}

// Single_Line_Trans 'T' and header byte 01. Flags TRANS:
//   01 02 03 04 -> 00 'C';  01 00 02 00 -> 01 'T';  00 00 00 00 -> 10h 'K'.
RA_TEST(new_system_trans) {
    New_System();
    const u8 pixels[12] = {1, 2, 3, 4, 1, 0, 2, 0, 0, 0, 0, 0};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 4, 3, pixels);
    Page p;
    Draw(2, 0, 4, 3, hdr, p.view, TRANS);
    const u8 expected_headers[3] = {0x00, 0x01, 0x10};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 3);
    RA_CHECK_EQ(0x40u, hdr->draw_flags);
    EXPECT_PAGE(p, {{0, 2, 1}, {0, 3, 2}, {0, 4, 3}, {0, 5, 4}, {1, 2, 1}, {1, 4, 2}});
}

// Long_Single_Line_Copy (asm 1697-1722, width >= 16): plain copy.
RA_TEST(new_system_long_copy) {
    New_System();
    u8 pixels[17];
    for (int i = 0; i < 17; i++) pixels[i] = (u8)(i + 1);
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 17, 1, pixels);
    u8 mem[20 * 2];
    memset(mem, 0xEE, sizeof(mem));
    ShapeViewType v = {mem, 20, 2, 0, 0};
    Draw(1, 1, 17, 1, hdr, v, 0);
    u8 expected[40];
    memset(expected, 0xEE, sizeof(expected));
    for (int i = 0; i < 17; i++) expected[20 + 1 + i] = (u8)(i + 1);
    RA_CHECK_MEM_EQ(expected, mem, sizeof(mem));
    RA_CHECK_EQ(0, Line_Headers(hdr)[0]);
}

// Fading line routines and the CriticalFadeRedirections patch (asm 1347-1353).
// Flags TRANS|GHOST|FADING, 4x5 at (2,0). First draw (count 1) uses AllFlagsJumpTable:
//   01 02 03 04 -> 04 'S' Single_Fade        -> 02 03 04 05
//   00 00 00 00 -> 10h 'K'
//   01 00 02 03 -> 05 's' Single_Fade_Trans  -> 02 keep 03 04
//   05 01 02 03 -> 06 'H' Ghost_Fading       -> row0[52]=A2->A3, 02, 03, 04
//   06 00 01 05 -> 07 'h' Ghost_Fading_Trans -> row1[62]=B2->B3, keep, 02, row0[65]=A5->A6
// Second draw (count 2) reuses the headers through table 7, whose entries 4/5 are patched to
// Single_Line_Fading / _Trans: +2 everywhere.
RA_TEST(new_system_fading_routines_and_patch) {
    New_System();
    const u8 pixels[20] = {1, 2, 3, 4, 0, 0, 0, 0, 1, 0, 2, 3, 5, 1, 2, 3, 6, 0, 1, 5};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 128, 4, 5, pixels);
    Page p;
    Draw(2, 0, 4, 5, hdr, p.view, TRANS | GHOST | FADING, Ghost, FadeInc, 1);
    const u8 expected_headers[5] = {0x04, 0x10, 0x05, 0x06, 0x07};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 5);
    RA_CHECK_EQ(0x1140u, hdr->draw_flags);
    EXPECT_PAGE(p, {{0, 2, 2}, {0, 3, 3}, {0, 4, 4}, {0, 5, 5},
                    {2, 2, 2}, {2, 4, 3}, {2, 5, 4},
                    {3, 2, 0xA3}, {3, 3, 2}, {3, 4, 3}, {3, 5, 4},
                    {4, 2, 0xB3}, {4, 4, 2}, {4, 5, 0xA6}});

    Page q;
    Draw(2, 0, 4, 5, hdr, q.view, TRANS | GHOST | FADING, Ghost, FadeInc, 2);
    EXPECT_PAGE(q, {{0, 2, 3}, {0, 3, 4}, {0, 4, 5}, {0, 5, 6},
                    {2, 2, 3}, {2, 4, 4}, {2, 5, 5},
                    {3, 2, 0xA4}, {3, 3, 3}, {3, 4, 4}, {3, 5, 5},
                    {4, 2, 0xB4}, {4, 4, 3}, {4, 5, 0xA7}});
}

// Single_Line_Predator 'P' and Single_Line_Predator_Trans 'p' (asm 2204-2284). Flags TRANS|PREDATOR, arg 0:
//   01 02 03 04 -> 08 'P': offsets 1,3,2,5 -> r0c3=23, r0c6=26, r0c6=26, linear 5+5=10=r1c2=32
//   01 00 02 00 -> 09 'p': offsets 2,3     -> r1c4=34, (skip), r1c7=37, (skip)
//   00 00 00 00 -> 10h 'K'
RA_TEST(new_system_predator) {
    New_System();
    const u8 pixels[12] = {1, 2, 3, 4, 1, 0, 2, 0, 0, 0, 0, 0};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 4, 3, pixels);
    Page p;
    Draw(2, 0, 4, 3, hdr, p.view, TRANS | PREDATOR, 0);
    const u8 expected_headers[3] = {0x08, 0x09, 0x10};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 3);
    EXPECT_PAGE(p, {{0, 2, 0x23}, {0, 3, 0x26}, {0, 4, 0x26}, {0, 5, 0x32}, {1, 2, 0x34}, {1, 4, 0x37}});
}

// New vs old system with SHAPE_PARTIAL: Single_Line_Predator skips non-predator pixels (asm 2215-2216),
// BF_Predator_Trans draws them (asm 3297-3298). Frame 03 04 (header 08 'P'), partial 0x80.
//   Unclipped at (2,0): 03 not drawn; 04 <- +1 = r0c4 = 24.
//   Clipped at (-1,0): old system, only 04 is visible, non-predator (count 0x80) -> source 04 at r0c0.
RA_TEST(new_system_predator_partial_vs_clipped) {
    New_System();
    const u8 pixels[2] = {3, 4};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 2, 1, pixels);
    Page p;
    Draw(2, 0, 2, 1, hdr, p.view, TRANS | PREDATOR | PARTIAL, 0, 0x80);
    RA_CHECK_EQ(0x08, Line_Headers(hdr)[0]);
    EXPECT_PAGE(p, {{0, 3, 0x24}});
    Page q;
    Draw(-1, 0, 2, 1, hdr, q.view, TRANS | PREDATOR | PARTIAL, 0, 0x80);
    EXPECT_PAGE(q, {{0, 0, 0x04}});
}

// Single_Line_Predator_Ghost 'Q' / _Trans 'q' (asm 2294-2401). Flags TRANS|GHOST|PREDATOR, arg 0, at (2,1):
//   05 01 02 -> 0Ah 'Q': 33 (not translucent); 36 -> row0[33]=A3; 36 -> row0[34]=A4
//   05 00 06 -> 0Bh 'q': +5 -> r2c7=47; skip; +2 -> r2c6=46
RA_TEST(new_system_predator_ghost) {
    New_System();
    const u8 pixels[6] = {5, 1, 2, 5, 0, 6};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 3, 2, pixels);
    Page p;
    Draw(2, 1, 3, 2, hdr, p.view, TRANS | GHOST | PREDATOR, Ghost, 0);
    const u8 expected_headers[2] = {0x0A, 0x0B};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 2);
    EXPECT_PAGE(p, {{1, 2, 0x33}, {1, 3, 0xA3}, {1, 4, 0xA4}, {2, 2, 0x47}, {2, 4, 0x46}});
}

// 'Q' looks up the ghost table with all of EAX (asm 2318-2319): after "and eax,0ffffh / mov al,[edi+eax]"
// bits 8-15 still hold the predator offset. Row of 300 bytes, arg -1 -> negative table entries
// -3+300 = 297 (129h) and -2+300 = 298 (12Ah): the index is 100h + colour, i.e. translucent row 0.
// Ghost table here: is_trans[05]=2 (so the header gets the ghost bit), row0[11h]=1, row1[d]=C0h|(d&15).
// Page filled with 11h: both pixels read 11h, look up is_trans[111h] = 1 -> row1[11h] = C1h.
// (An 8-bit index would read is_trans[11h] = FFh and draw 11h.)
RA_TEST(new_system_predator_ghost_wide_index) {
    New_System();
    static u8 wide_ghost[256 * 4];
    memset(wide_ghost, 0xFF, sizeof(wide_ghost));
    wide_ghost[0x05] = 2;
    wide_ghost[256 + 0x11] = 1;
    for (int d = 0; d < 256; d++) wide_ghost[512 + d] = (u8)(0xC0 | (d & 15));
    const u8 pixels[2] = {5, 5};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 2, 1, pixels);
    static u8 mem[300 * 3];
    memset(mem, 0x11, sizeof(mem));
    ShapeViewType v = {mem, 300, 1, 0, 0};
    Draw(0, 0, 2, 1, hdr, v, TRANS | GHOST | PREDATOR, wide_ghost, -1);
    RA_CHECK_EQ(0x0A, Line_Headers(hdr)[0]);
    RA_CHECK_EQ(0xC1, mem[0]);
    RA_CHECK_EQ(0xC1, mem[1]);
    RA_CHECK_EQ(0x11, mem[2]);
}

// Single_Line_Predator_Fading 'R' / _Trans 'r' advance the offset with "and [BYTE BFPredOffset],2"
// (asm 2429, 2484) instead of add: from 0 it stays 0, so every offset is BFPredTable[0] = 1.
// Flags TRANS|PREDATOR|FADING count 1, arg 0, at (2,1):
//   01 02 03 -> 0Ch 'R': 33+1, 34+1, 35+1 = 34 35 36
//   01 00 02 -> 0Dh 'r': r2c3=43 -> 44; skip; r2c5=45 -> 46
// With arg 1 the offset starts at 2 and stays 2 (2 AND 2): every offset is 3 -> 36 37 38.
RA_TEST(new_system_predator_fading_and_bug) {
    New_System();
    const u8 pixels[6] = {1, 2, 3, 1, 0, 2};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 3, 2, pixels);
    Page p;
    Draw(2, 1, 3, 2, hdr, p.view, TRANS | PREDATOR | FADING, FadeInc, 1, 0);
    const u8 expected_headers[2] = {0x0C, 0x0D};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 2);
    EXPECT_PAGE(p, {{1, 2, 0x34}, {1, 3, 0x35}, {1, 4, 0x36}, {2, 2, 0x44}, {2, 4, 0x46}});

    Page q;
    Draw(2, 1, 3, 1, Make_Frame(BigBuffer, 256, 3, 1, pixels), q.view, TRANS | PREDATOR | FADING, FadeInc, 1, 1);
    EXPECT_PAGE(q, {{1, 2, 0x36}, {1, 3, 0x37}, {1, 4, 0x38}});
}

// Single_Line_Predator_Ghost_Fading 'U' / _Trans 'u' clear EAX before the fade loop (asm 2561, 2633), so
// every drawn pixel becomes fade^N(0): FadeInc, count 1 -> 01; count 3 -> 03.
// Flags TRANS|GHOST|PREDATOR|FADING, arg 0, at (2,1):  05 01 02 -> 0Eh 'U';  05 00 01 -> 0Fh 'u'.
RA_TEST(new_system_predator_ghost_fading_fades_zero) {
    New_System();
    const u8 pixels[6] = {5, 1, 2, 5, 0, 1};
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 3, 2, pixels);
    Page p;
    Draw(2, 1, 3, 2, hdr, p.view, TRANS | GHOST | PREDATOR | FADING, Ghost, FadeInc, 1, 0);
    const u8 expected_headers[2] = {0x0E, 0x0F};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 2);
    EXPECT_PAGE(p, {{1, 2, 1}, {1, 3, 1}, {1, 4, 1}, {2, 2, 1}, {2, 4, 1}});
    Page q;
    Draw(2, 1, 3, 2, hdr, q.view, TRANS | GHOST | PREDATOR | FADING, Ghost, FadeInc, 3, 0);
    EXPECT_PAGE(q, {{1, 2, 3}, {1, 3, 3}, {1, 4, 3}, {2, 2, 3}, {2, 4, 3}});
}

// Theater frames take their pixels from TheaterShapeBufferStart (asm 1200-1205).
RA_TEST(new_system_theater_buffer) {
    New_System();
    const u8 pixels[2] = {7, 8};
    ShapeFrameHeaderType *hdr = Make_Frame(TheaterBuffer, 0, 2, 1, pixels, 1);
    Page p;
    Draw(2, 1, 2, 1, hdr, p.view, TRANS);
    EXPECT_PAGE(p, {{1, 2, 7}, {1, 3, 8}});
}

// UseOldShapeDraw (set by CC_Draw_Shape for rotated/scaled shapes) means "src is raw pixels" even when
// the big shape buffer is on (asm 1184-1188).
RA_TEST(use_old_shape_draw_takes_raw_pixels) {
    New_System();
    UseOldShapeDraw = true;
    Page p;
    Draw(2, 1, 3, 2, Shape3x2, p.view, TRANS);
    EXPECT_PAGE(p, {{1, 2, 0x01}, {1, 4, 0x05}, {2, 2, 0x06}, {2, 3, 0x07}});
    UseOldShapeDraw = false;
}

// A clipped big-buffer frame still gets its headers built (asm 1263-1292 run before clipping), but is
// drawn by BF_Ghost_Trans on the raw pixels.
RA_TEST(new_system_clipped_uses_old_routines) {
    New_System();
    ShapeFrameHeaderType *hdr = Make_Frame(BigBuffer, 0, 3, 2, Shape3x2);
    Page p;
    Draw(-1, 1, 3, 2, hdr, p.view, TRANS | GHOST, Ghost);
    RA_CHECK_EQ(0x1040u, hdr->draw_flags);
    const u8 expected_headers[2] = {0x03, 0x03};
    RA_CHECK_MEM_EQ(expected_headers, Line_Headers(hdr), 2);
    // visible: 00 05 / 07 00 at columns 0-1; 05 on dest 31 -> A1.
    EXPECT_PAGE(p, {{1, 1, 0xA1}, {2, 0, 0x07}});
}

RA_TEST_MAIN()
