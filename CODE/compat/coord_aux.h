/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Portable replacements for the Watcom "#pragma aux" helpers calcx/calcy in CODE/COORD.CPP
**	(session S06b). See docs/codebase/compat-notes.md "S06b notes".
*/

/*
**	Included by CODE/COORD.CPP when not compiling with Watcom. Self-contained so that
**	tests/s06b_coord_test.cpp can include it without the rest of the game. The original
**	assembly is still in COORD.CPP under #ifdef __WATCOMC__.
**
**	SIMULATION CODE: Move_Point and Normal_Move_Point (and through them Coord_Move) use these,
**	so the result must be bit-exact with the original instructions.
*/

#ifndef RA_COORD_AUX_H
#define RA_COORD_AUX_H

/*
**	Original (parm [ax] [bx], value [eax]):
**		imul bx          DX:AX = ax * bx           (signed 16 x 16 -> 32)
**		shl  ax,1
**		rcl  dx,1        DX:AX <<= 1
**		mov  al,ah
**		mov  ah,dl       AX = bits 8..23 of (product << 1) = bits 7..22 of the product
**		cwd
**	calcy then does "neg eax".
**
**	Only AX is defined: the upper half of EAX is whatever the caller left there (the
**	"and eax,0FFFFh" is commented out in the original). Every caller adds the result to a
**	short ("x += calcx(...)"), so only the low 16 bits ever matter. The C++ versions return
**	those 16 bits sign-extended. Unsigned arithmetic avoids signed-overflow UB for
**	-32768 * -32768.
*/
inline int calcx(signed short value, short distance)
{
	unsigned int product = (unsigned int)((int)value * (int)distance);
	return (int)(short)(unsigned short)(product >> 7);
}

inline int calcy(signed short value, short distance)
{
	unsigned int product = (unsigned int)((int)value * (int)distance);
	unsigned short ax = (unsigned short)(product >> 7);
	return (int)(short)(unsigned short)(0u - ax);		// "neg eax": low 16 bits are -ax mod 2^16
}

#endif /* RA_COORD_AUX_H */
