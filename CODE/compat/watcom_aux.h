/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**	Portable replacements for Watcom "#pragma aux" inline assembly (session S06a).
**	See docs/codebase/compat-notes.md.
*/

/*
**	Included by CODE/JSHELL.H (and CODE/RGB.H for the port output functions) when not
**	compiling with Watcom. Self-contained so that tests/jshell_asm_test.cpp can include it
**	without the rest of the game. The original assembly is still in JSHELL.H/RGB.H under
**	#ifdef __WATCOMC__.
*/

#ifndef RA_WATCOM_AUX_H
#define RA_WATCOM_AUX_H

/*
**	Portable C++ versions of the Watcom "#pragma aux" inline assembly in JSHELL.H. They must give exactly the same results as the
**	assembly: Fixed_To_Cardinal is used by the simulation (FLY, INFANTRY, TECHNO, UNIT,
**	VESSEL) and Set_Bit/Get_Bit back BooleanVectorClass. Little-endian is assumed, as
**	everywhere else in the code; the bit arrays are accessed byte-wise, which on a
**	little-endian machine addresses the same bits as the 32-bit "bt/bts/btr" of the asm.
**	Tests: tests/jshell_asm_test.cpp.
*/

/*
**	"bit" is split with an unsigned shift (shr) into a dword index and a bit number
**	0..31, then the bit is cleared and set again if "value" is non-zero.
*/
inline void Set_Bit(void * array, int bit, int value)
{
	unsigned int ubit = (unsigned int)bit;
	unsigned char * byte = (unsigned char *)array + (ubit >> 5) * 4 + ((ubit & 0x1F) >> 3);
	unsigned char mask = (unsigned char)(1u << (ubit & 0x07));
	*byte = (unsigned char)(*byte & ~mask);
	if (value) *byte = (unsigned char)(*byte | mask);
}

/*
**	Returns 0 or 1 ("setc al" with the upper bits of eax already zero).
*/
inline int Get_Bit(void const * array, int bit)
{
	unsigned int ubit = (unsigned int)bit;
	unsigned char const * byte = (unsigned char const *)array + (ubit >> 5) * 4 + ((ubit & 0x1F) >> 3);
	return (*byte >> (ubit & 0x07)) & 1;
}

/*
**	Scans whole 32-bit words from the start of the array for the first set (clear) bit,
**	without a length limit, exactly like the asm ("bsf" on each dword until non-zero).
*/
inline unsigned int _Read_Dword_LE(unsigned char const * p)
{
	return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

inline int _Lowest_Set_Bit(unsigned int value)
{
	int index = 0;
	while (!(value & 1u)) {
		value >>= 1;
		index++;
	}
	return index;
}

inline int First_True_Bit(void const * array)
{
	unsigned char const * ptr = (unsigned char const *)array;
	int base = 0;
	for (;;) {
		unsigned int value = _Read_Dword_LE(ptr);
		if (value != 0) return base + _Lowest_Set_Bit(value);
		ptr += 4;
		base += 32;
	}
}

inline int First_False_Bit(void const * array)
{
	unsigned char const * ptr = (unsigned char const *)array;
	int base = 0;
	for (;;) {
		unsigned int value = ~_Read_Dword_LE(ptr);
		if (value != 0) return base + _Lowest_Set_Bit(value);
		ptr += 4;
		base += 32;
	}
}

/*
**	(base * fixed) keeps only the low 32 bits of the 64-bit product ("mul edx" result in
**	eax), adds 0x80 for rounding (wrapping at 32 bits), saturates to 0x00FFFFFF if any of
**	the top 8 bits are set, and shifts right by 8.
*/
inline unsigned Fixed_To_Cardinal(unsigned base, unsigned fixed)
{
	unsigned int value = (unsigned int)base * (unsigned int)fixed;
	value += 0x80u;
	if (value & 0xFF000000u) value = 0x00FFFFFFu;
	return value >> 8;
}

/*
**	(cardinal << 8) / base with a 32-bit shift and an unsigned 32-bit divide. If base is
**	zero the asm skips everything and returns "cardinal" unchanged.
*/
inline unsigned Cardinal_To_Fixed(unsigned base, unsigned cardinal)
{
	if (base == 0) return cardinal;
	return (unsigned int)((unsigned int)cardinal << 8) / (unsigned int)base;
}

/*
**	VGA port output. User-mode programs cannot do port I/O on any current OS (the Win32
**	build would have faulted on it), so these do nothing.
*/
#ifndef OUTPORTB
#define OUTPORTB
inline void outportb(int port, unsigned char data) {(void)port; (void)data;}
inline void outport(int port, unsigned short data) {(void)port; (void)data;}
#endif


#endif /* RA_WATCOM_AUX_H */
