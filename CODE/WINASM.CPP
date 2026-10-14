//
//	Command & Conquer Red Alert(tm)
//	Copyright 2025 Electronic Arts Inc.
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S  I N C  **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : WINASM.CPP (was WINASM.ASM)              *
 *                                                                         *
 *                   Programmer : Steve Tall (asm original)                *
 *                                                                         *
 *                   Start Date : October 26th, 1995                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Portable C++ replacement for WINASM.ASM (session S07-C, 2026-10-14).    *
 *                                                                         *
 * Ported (same output as the asm):                                        *
 *   Asm_Interpolate, Asm_Interpolate_Line_Double,                         *
 *   Asm_Interpolate_Line_Interpolate -- 2x upscalers that use the         *
 *       palette interpolation table (callers: CODE/INTERPAL.CPP)          *
 *   MouseQX, MouseQY -- C-linkage globals (CODE/ENDING.CPP, INTRO.CPP)    *
 *   Stop_Execution, Int3 -- were empty in the asm (int 3 commented out)   *
 *                                                                         *
 * Link stubs for dead asm (no behaviour on a modern system):              *
 *   ModeX_Blit -- VGA mode X blit (only for a 320-wide SeenBuff, which    *
 *       the Win32 game never uses; CODE/GSCREEN.CPP still references it)  *
 *   Set_Palette_Register -- VGA DAC write (only CODE/WINSTUB.CPP          *
 *       Colour_Debug, which nothing calls)                                *
 *                                                                         *
 * Not carried over: Asm_Create_Palette_Interpolation_Table (its only call *
 * is commented out, CODE/INTERPAL.CPP:150; the C++                        *
 * Create_Palette_Interpolation_Table is used instead), the C-linkage      *
 * Memory_Error_Exit variable (the game uses the C++ one in                *
 * WIN32LIB/MEM/ALLOC.CPP), and the modem stubs (removed in S05).          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Prototypes must match CODE/INTERPAL.CPP, CODE/KEY.CPP, CODE/GSCREEN.CPP and
// CODE/WINSTUB.CPP (all extern "C", cdecl).

#include <string.h>

class GraphicBufferClass;

extern "C" {

// Defined in CODE/INTERPAL.CPP. Entry [a][b] is the colour halfway between
// palette colours a and b. The asm indexed it with (b | a << 8).
extern unsigned char PaletteInterpolationTable[256][256];

void Asm_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width);
void Asm_Interpolate_Line_Double(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width);
void Asm_Interpolate_Line_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width);

void Stop_Execution(void);
void Int3(void);
void ModeX_Blit(GraphicBufferClass *source);
void Set_Palette_Register(int number, int red, int green, int blue);

/*
** C-linkage mouse position globals from the asm data segment. Nothing in the
** Win32 game writes them (the live values are KeyboardClass::MouseQX/MouseQY),
** so they stay 0; kept so ENDING.CPP and INTRO.CPP link as before.
*/
int MouseQX = 0;
int MouseQY = 0;

}	// extern "C"


/*
** Scratch lines, 640 DWORDs each in the asm. Large enough for source lines up
** to 1280 pixels; the game uses 320.
*/
static unsigned char LineBuffer[640 * 4];
static unsigned char TopLine[640 * 4];
static unsigned char BottomLine[640 * 4];


/*
** Interpolates one source line of `source_width` pixels into 2 * source_width
** destination pixels:
**     p0, mix(p0,p1), p1, mix(p1,p2), ..., p[w-1], 0
** where mix(a, b) = PaletteInterpolationTable[b][a]. The last destination
** pixel is always 0 ("a blank on the end of a row").
** If `copy` is not NULL the same bytes are also written there.
**
** As in the asm, the source is consumed in pixel pairs: (w - 2) / 2 pairs plus
** a final pair, so 2 * ((w - 2) / 2) + 2 bytes; this equals w for even widths.
** Returns the source pointer after them.
**
** The asm's pair counter would underflow (and run away) for widths below 4;
** here such widths just do the final pair. The game always passes 320.
*/
static unsigned char const *Interpolate_Line(unsigned char const *src, unsigned char *dest, unsigned char *copy, int source_width)
{
	unsigned pairs = (unsigned)(source_width - 2) >> 1;
	if (source_width < 4) pairs = 0;		// see above (asm would underflow)

	for (unsigned i = 0; i < pairs; i++) {
		unsigned char p0 = src[0];
		unsigned char p1 = src[1];
		unsigned char p2 = src[2];
		unsigned char out0 = p0;
		unsigned char out1 = PaletteInterpolationTable[p1][p0];
		unsigned char out2 = p1;
		unsigned char out3 = PaletteInterpolationTable[p2][p1];
		dest[0] = out0; dest[1] = out1; dest[2] = out2; dest[3] = out3;
		if (copy) {
			copy[0] = out0; copy[1] = out1; copy[2] = out2; copy[3] = out3;
			copy += 4;
		}
		src += 2;
		dest += 4;
	}

	/*
	** Last two pixels and the blank.
	*/
	unsigned char p0 = src[0];
	unsigned char p1 = src[1];
	unsigned char mid = PaletteInterpolationTable[p1][p0];
	dest[0] = p0; dest[1] = mid; dest[2] = p1; dest[3] = 0;
	if (copy) {
		copy[0] = p0; copy[1] = mid; copy[2] = p1; copy[3] = 0;
	}
	return src + 2;
}


/*********************************************************************************************
 * Asm_Interpolate -- interpolate a 320x200 buffer to a 640x400 screen                       *
 *                                                                                           *
 * INPUT:	ptr to source buffer (320x200 image)                                             *
 *              ptr to dest buffer (640x400)                                                 *
 *              height of source buffer                                                      *
 *              width of source buffer                                                       *
 *              dest_width: bytes from one OUTPUT row to the next one written (the caller    *
 *                passes 2 * pitch, so every other screen line is left untouched)            *
 *                                                                                           *
 * HISTORY:                                                                                  *
 *   12/15/95 ST : Created (asm).                                                            *
 *===========================================================================================*/
void Asm_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width)
{
	unsigned char const *src = src_ptr;
	unsigned char *old_dest = dest_ptr;

	do {
		src = Interpolate_Line(src, old_dest, NULL, src_width);
		old_dest += dest_width;
	} while (--lines > 0);		// asm: "dec; jnz" (a count of 0 would underflow there)
}


/*********************************************************************************************
 * Asm_Interpolate_Line_Double -- as Asm_Interpolate, but each output row is also copied to   *
 *                                the line below it (dest_width / 2 further on).              *
 *                                                                                           *
 * The copy is (src_width / 2) DWORDs, i.e. 2 * src_width bytes for even widths.             *
 *===========================================================================================*/
void Asm_Interpolate_Line_Double(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width)
{
	unsigned char const *src = src_ptr;
	unsigned char *old_dest = dest_ptr;
	unsigned half = (unsigned)dest_width >> 1;
	size_t copy_bytes = (size_t)((unsigned)src_width >> 1) * 4;

	do {
		src = Interpolate_Line(src, old_dest, LineBuffer, src_width);
		old_dest += half;
		memcpy(old_dest, LineBuffer, copy_bytes);
		old_dest += half;
	} while (--lines > 0);
}


/*
** mix(source1[i], source2[i]) for 2 * source_width pixels.
*/
static void Interpolate_Between_Lines(unsigned char const *source1, unsigned char const *source2,
									  unsigned char *destination, int source_width)
{
	int count = source_width + source_width;
	for (int i = 0; i < count; i++) {
		destination[i] = PaletteInterpolationTable[source2[i]][source1[i]];
	}
}


/*********************************************************************************************
 * Asm_Interpolate_Line_Interpolate -- as Asm_Interpolate, but the line between two output    *
 *                                     rows is the mix of the two rows.                      *
 *                                                                                           *
 * Output (half = dest_width / 2 bytes apart): row 2k = line k, row 2k+1 = mix(line k,       *
 * line k+1) for k = 0 .. lines-2, then one last row.                                        *
 *                                                                                           *
 * KEPT FROM THE ASM, ON PURPOSE: that last row is the interpolation of source line `lines`, *
 * ONE PAST THE END of the source image (the asm had advanced its source pointer past the     *
 * last line), and source line lines-1 never appears on its own. The caller must therefore   *
 * provide lines + 1 readable source lines. No caller uses this mode: CopyType 2 is never    *
 * selected in the game (CODE/INTERPAL.CPP, MAPSEL.CPP, SCORE.CPP only use 0 and 1).         *
 *                                                                                           *
 * Each output row is (src_width / 2) DWORDs. Counts below 2 lines would underflow the asm's *
 * loop counter; here the loop runs once and the function returns.                           *
 *===========================================================================================*/
void Asm_Interpolate_Line_Interpolate(unsigned char *src_ptr, unsigned char *dest_ptr, int lines, int src_width, int dest_width)
{
	unsigned char const *src = src_ptr;
	unsigned char *old_dest = dest_ptr;
	unsigned char *next_line = TopLine;
	unsigned char *last_line = BottomLine;
	size_t copy_bytes = (size_t)((unsigned)src_width >> 1) * 4;
	unsigned half = (unsigned)dest_width >> 1;
	unsigned char *swap;

	Interpolate_Line(src, next_line, NULL, src_width);
	swap = next_line; next_line = last_line; last_line = swap;
	src += src_width;
	lines--;

	do {
		Interpolate_Line(src, next_line, NULL, src_width);
		Interpolate_Between_Lines(last_line, next_line, LineBuffer, src_width);

		memcpy(old_dest, last_line, copy_bytes);
		old_dest += half;
		memcpy(old_dest, LineBuffer, copy_bytes);
		old_dest += half;
		src += src_width;

		swap = next_line; next_line = last_line; last_line = swap;
	} while (--lines > 0);

	Interpolate_Line(src, next_line, NULL, src_width);		// one past the end, see above
	memcpy(old_dest, next_line, copy_bytes);
}


/*
** The asm versions were "nop; ret" and "ret" (the int 3 was commented out).
** Stop_Execution is called on Scroll Lock (CODE/KEY.CPP); Int3 only under MEM_CHECK.
*/
void Stop_Execution(void)
{
}

void Int3(void)
{
}


/*
** Dead on Win32, kept only so the remaining references link. See the file header.
** Session S08 should remove the 320-wide branch in GScreenClass::Blit_Display and
** Colour_Debug, after which these can go.
*/
void ModeX_Blit(GraphicBufferClass *)
{
}

void Set_Palette_Register(int, int, int, int)
{
}
