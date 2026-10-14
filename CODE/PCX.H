/*
**	Command & Conquer Red Alert(tm) - multiplayer port.
**	Reconstruction of a file that is missing from the Electronic Arts source release
**	(session S06a). Licensed like the rest of the game code:
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : PCX.H                                                        *
 *                                                                                             *
 *   RECONSTRUCTED in session S06a (2026-10-14). The game's own PCX header: the PCX file       *
 *   structures (identical to CODE/FILEPCX.H and WIN32LIB/INCLUDE/FILEPCX.H) and the          *
 *   game's Write_PCX_File (WRITEPCX.CPP). See docs/codebase/compat-notes.md.                 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

/*
**	Same guard as FILEPCX.H on purpose: WINSTUB.CPP includes FUNCTION.H (and so this file)
**	and then "filepcx.h"; with a shared guard the second definition of RGB/PCX_HEADER is
**	skipped instead of clashing.
*/
#ifndef PCX_H
#define PCX_H

class FileClass;
class GraphicBufferClass;
class PaletteClass;

/*
**	PCX file structures. These are read from and written to disk as raw bytes, so they are
**	byte-packed explicitly (the PCX header is 128 bytes).
*/
#pragma pack(push, 1)

typedef	struct {
	char	red;
	char	green;
	char	blue;
} RGB;

typedef	struct {
	char	id;
	char	version;
	char	encoding;
	char	pixelsize;
	short	x;
	short	y;
	short	width;
	short	height;
	short	xres;
	short	yres;
	RGB		ega_palette[16];
	char	nothing;
	char	color_planes;
	short	byte_per_line;
	short	palette_type;
	char	filler[58];
} PCX_HEADER;

#pragma pack(pop)

/*
**	Writes the picture in "pic" as a PCX file, with "palette" (6-bit VGA values, converted to
**	8 bits) appended. Returns false on success (WRITEPCX.CPP).
*/
int Write_PCX_File(FileClass & file, GraphicBufferClass & pic, PaletteClass * palette);

#if __cplusplus >= 201103L
static_assert(sizeof(RGB) == 3, "PCX palette entries are 3 bytes");
static_assert(sizeof(PCX_HEADER) == 128, "the PCX file header is 128 bytes");
#endif

#endif
