// Unit test for the C++ replacements of the Watcom "#pragma aux" inline assembly in CODE/JSHELL.H
// (session S06a). Fixed_To_Cardinal is used by the simulation, so it must be bit-exact.
//
// On GCC/Clang 32-bit x86 the original instruction sequences are run as inline assembly and
// compared with the C++ versions over many inputs. Elsewhere only the fixed expectations run.

#include "ra_test.h"
#include "watcom_aux.h"

#include <string.h>

#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
#define RA_HAVE_X86_ASM 1

static unsigned asm_fixed_to_cardinal(unsigned base, unsigned fixed)
{
	unsigned eax = base, edx = fixed;
	__asm__("mull %%edx\n\t"
	        "addl $0x80, %%eax\n\t"
	        "testl $0xFF000000, %%eax\n\t"
	        "jz 1f\n\t"
	        "movl $0x00FFFFFF, %%eax\n"
	        "1:\n\t"
	        "shrl $8, %%eax"
	        : "+a"(eax), "+d"(edx) : : "cc");
	return eax;
}

static unsigned asm_cardinal_to_fixed(unsigned base, unsigned cardinal)
{
	unsigned eax = cardinal, ebx = base, edx = 0;
	__asm__("orl %%ebx, %%ebx\n\t"
	        "jz 1f\n\t"
	        "shll $8, %%eax\n\t"
	        "xorl %%edx, %%edx\n\t"
	        "divl %%ebx\n"
	        "1:"
	        : "+a"(eax), "+d"(edx) : "b"(ebx) : "cc");
	return eax;
}

static int asm_get_bit(void const * array, int bit)
{
	int eax = bit;
	__asm__("movl %%eax, %%ebx\n\t"
	        "shrl $5, %%ebx\n\t"
	        "andl $0x1F, %%eax\n\t"
	        "btl %%eax, (%%esi,%%ebx,4)\n\t"
	        "setc %%al"
	        : "+a"(eax) : "S"(array) : "ebx", "cc", "memory");
	return eax;
}

static void asm_set_bit(void * array, int bit, int value)
{
	__asm__ volatile("movl %%ecx, %%ebx\n\t"
	                 "shrl $5, %%ebx\n\t"
	                 "andl $0x1F, %%ecx\n\t"
	                 "btrl %%ecx, (%%esi,%%ebx,4)\n\t"
	                 "orl %%eax, %%eax\n\t"
	                 "jz 1f\n\t"
	                 "btsl %%ecx, (%%esi,%%ebx,4)\n"
	                 "1:"
	                 : "+c"(bit) : "S"(array), "a"(value) : "ebx", "cc", "memory");
}
#endif

// Small deterministic generator for test inputs (not the game's RNG).
static unsigned next_value(unsigned & state)
{
	state = state * 1664525u + 1013904223u;
	return state;
}

RA_TEST(fixed_to_cardinal_known_values)
{
	RA_CHECK_EQ(Fixed_To_Cardinal(100, 0x0100), 100u);          // x 1.0
	RA_CHECK_EQ(Fixed_To_Cardinal(100, 0x0080), 50u);           // x 0.5
	RA_CHECK_EQ(Fixed_To_Cardinal(3, 0x0080), 2u);              // 1.5 rounds up
	RA_CHECK_EQ(Fixed_To_Cardinal(0, 0xFFFF), 0u);
	RA_CHECK_EQ(Fixed_To_Cardinal(0x1000, 0x1000), 0xFFFFu);    // 0x1000080: top byte set, saturates
	RA_CHECK_EQ(Fixed_To_Cardinal(0x00FFFFFF, 0x0100), 0xFFFFu); // 0xFFFFFF00+0x80 saturates
	// Only the low 32 bits of the product count: 0x10000 * 0x10000 = 2^32 -> 0, +0x80 -> 0.
	RA_CHECK_EQ(Fixed_To_Cardinal(0x10000u * 0x100u, 0x100u), 0u);
}

RA_TEST(cardinal_to_fixed_known_values)
{
	RA_CHECK_EQ(Cardinal_To_Fixed(100, 50), 0x80u);
	RA_CHECK_EQ(Cardinal_To_Fixed(0, 1234), 1234u);             // base 0: returns cardinal unchanged
	RA_CHECK_EQ(Cardinal_To_Fixed(1, 0x01000000), 0u);          // the <<8 is 32-bit
	RA_CHECK_EQ(Cardinal_To_Fixed(3, 1), 85u);
}

RA_TEST(bit_functions_known_values)
{
	unsigned char bits[16];
	memset(bits, 0, sizeof(bits));
	Set_Bit(bits, 0, 1);
	Set_Bit(bits, 9, 1);
	Set_Bit(bits, 33, 1);
	RA_CHECK_EQ(bits[0], 0x01);
	RA_CHECK_EQ(bits[1], 0x02);
	RA_CHECK_EQ(bits[4], 0x02);
	RA_CHECK_EQ(Get_Bit(bits, 9), 1);
	RA_CHECK_EQ(Get_Bit(bits, 10), 0);
	RA_CHECK_EQ(First_True_Bit(bits), 0);
	Set_Bit(bits, 0, 0);
	RA_CHECK_EQ(bits[0], 0x00);
	RA_CHECK_EQ(First_True_Bit(bits), 9);
	memset(bits, 0xFF, sizeof(bits));
	Set_Bit(bits, 70, 0);
	RA_CHECK_EQ(First_False_Bit(bits), 70);
}

#ifdef RA_HAVE_X86_ASM
RA_TEST(fixed_point_matches_original_asm)
{
	unsigned state = 12345;
	for (int i = 0; i < 200000; i++) {
		unsigned a = next_value(state);
		unsigned b = next_value(state);
		// Mix full-range values with the small ranges the game uses (8.8 fixed, leptons).
		if (i & 1) {a &= 0xFFFF; b &= 0x3FF;}
		RA_CHECK_EQ(Fixed_To_Cardinal(a, b), asm_fixed_to_cardinal(a, b));
		RA_CHECK_EQ(Cardinal_To_Fixed(a | 1, b & 0xFFFF), asm_cardinal_to_fixed(a | 1, b & 0xFFFF));
	}
	RA_CHECK_EQ(Cardinal_To_Fixed(0, 77), asm_cardinal_to_fixed(0, 77));
}

RA_TEST(bit_functions_match_original_asm)
{
	unsigned state = 999;
	unsigned char a[64], b[64];
	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	for (int i = 0; i < 20000; i++) {
		int bit = (int)(next_value(state) % 512);
		int value = (int)(next_value(state) % 3);   // 0, 1 or 2 (any non-zero sets)
		Set_Bit(a, bit, value);
		asm_set_bit(b, bit, value);
		RA_CHECK_MEM_EQ(a, b, sizeof(a));
		int probe = (int)(next_value(state) % 512);
		RA_CHECK_EQ(Get_Bit(a, probe), asm_get_bit(b, probe));
	}
}
#endif

RA_TEST_MAIN()
