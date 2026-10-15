// Unit test for the C++ replacements of the Watcom "#pragma aux" helpers calcx/calcy in
// CODE/COORD.CPP (session S06b). They are used by Move_Point/Normal_Move_Point (simulation code),
// so they must be bit-exact.
//
// On GCC/Clang 32-bit x86 the original instruction sequences are run as inline assembly and
// compared with the C++ versions: exhaustively for every table value (-128..127) against every
// 16-bit distance, and for 2 000 000 random pairs of full 16-bit inputs. Elsewhere only the fixed
// expectations run.
//
// Only AX of the original result is defined (the upper half of EAX is whatever the caller left in
// it; "and eax,0FFFFh" is commented out in COORD.CPP), and every caller adds the result to a short.
// So the comparison is on the low 16 bits, and the asm is run with random garbage in the upper half
// of EAX to show that it does not leak into them.

#include "ra_test.h"
#include "coord_aux.h"

#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
#define RA_HAVE_X86_ASM 1

// parm [ax] [bx], modify [eax dx], value [eax]. Instructions copied from CODE/COORD.CPP.
static unsigned asm_calcx(unsigned eax_in, short distance)
{
	unsigned eax = eax_in;
	unsigned edx = 0xDEADBEEFu;
	__asm__("imulw %%bx\n\t"        // imul bx
	        "shlw $1, %%ax\n\t"     // shl  ax,1
	        "rclw $1, %%dx\n\t"     // rcl  dx,1
	        "movb %%ah, %%al\n\t"   // mov  al,ah
	        "movb %%dl, %%ah\n\t"   // mov  ah,dl
	        "cwtd"                  // cwd
	        : "+a"(eax), "+d"(edx) : "b"((int)distance) : "cc");
	return eax;
}

static unsigned asm_calcy(unsigned eax_in, short distance)
{
	unsigned eax = eax_in;
	unsigned edx = 0xDEADBEEFu;
	__asm__("imulw %%bx\n\t"
	        "shlw $1, %%ax\n\t"
	        "rclw $1, %%dx\n\t"
	        "movb %%ah, %%al\n\t"
	        "movb %%dl, %%ah\n\t"
	        "cwtd\n\t"
	        "negl %%eax"            // neg eax
	        : "+a"(eax), "+d"(edx) : "b"((int)distance) : "cc");
	return eax;
}
#endif

// Small deterministic generator for test inputs (not the game's RNG).
static unsigned next_value(unsigned & state)
{
	state = state * 1664525u + 1013904223u;
	return state;
}

RA_TEST(calc_known_values)
{
	RA_CHECK_EQ(calcx(127, 256), 254);           // 127*256 >> 7
	RA_CHECK_EQ(calcy(127, 256), -254);
	RA_CHECK_EQ(calcx(-3, 256), -6);
	RA_CHECK_EQ(calcy(-3, 256), 6);
	RA_CHECK_EQ(calcx(0, 12345), 0);
	RA_CHECK_EQ(calcy(0, 12345), 0);
	RA_CHECK_EQ(calcx(127, 1), 0);               // 127 >> 7
	RA_CHECK_EQ(calcx(-1, 1), -1);               // arithmetic: bits 7..22 of 0xFFFFFFFF
	// Bits 7..22 of the product; higher bits are dropped: 0x4000 * 0x4000 = 0x10000000, >> 7 = 0x200000, low 16 = 0.
	RA_CHECK_EQ(calcx(0x4000, 0x4000), 0);
	// -32768 * -32768 = 0x40000000 >> 7 = 0x800000 -> low 16 = 0.
	RA_CHECK_EQ(calcx(-32768, -32768), 0);
	// 0x7FFF * 0x7FFF = 0x3FFF0001 >> 7 = 0x7FFE00 -> 0xFE00 = -512.
	RA_CHECK_EQ(calcx(0x7FFF, 0x7FFF), -512);
	RA_CHECK_EQ(calcy(0x7FFF, 0x7FFF), 512);
	// A distance above 32767 (the callers pass unsigned short) is a negative short.
	RA_CHECK_EQ(calcx(127, (short)(unsigned short)40000), (int)(short)(unsigned short)((unsigned)(127 * -25536) >> 7));
}

#ifdef RA_HAVE_X86_ASM
RA_TEST(calc_matches_original_asm_table_range_exhaustive)
{
	unsigned state = 2026;
	long mismatches = 0;
	for (int value = -128; value <= 127; value++) {
		for (int d = 0; d < 0x10000; d++) {
			short distance = (short)(unsigned short)d;
			unsigned garbage = next_value(state) & 0xFFFF0000u;
			unsigned in = garbage | (unsigned short)(short)value;
			short ax = (short)asm_calcx(in, distance);
			short ay = (short)asm_calcy(in, distance);
			if (ax != (short)calcx((signed short)value, distance)) mismatches++;
			if (ay != (short)calcy((signed short)value, distance)) mismatches++;
			// What the callers do with it: x += calcx(...), x a short.
			short x0 = (short)next_value(state), x1 = x0, y0 = x0, y1 = x0;
			x0 += (int)asm_calcx(in, distance);
			x1 += calcx((signed short)value, distance);
			y0 += (int)asm_calcy(in, distance);
			y1 += calcy((signed short)value, distance);
			if (x0 != x1 || y0 != y1) mismatches++;
		}
	}
	RA_CHECK_EQ(mismatches, 0L);
}

RA_TEST(calc_matches_original_asm_random_16bit)
{
	unsigned state = 12345;
	long mismatches = 0;
	for (int i = 0; i < 2000000; i++) {
		unsigned r = next_value(state);
		short value = (short)(r & 0xFFFF);
		short distance = (short)(next_value(state) >> 16);
		unsigned in = (r & 0xFFFF0000u) | (unsigned short)value;
		if ((short)asm_calcx(in, distance) != (short)calcx(value, distance)) mismatches++;
		if ((short)asm_calcy(in, distance) != (short)calcy(value, distance)) mismatches++;
	}
	RA_CHECK_EQ(mismatches, 0L);
}
#endif

RA_TEST_MAIN()
