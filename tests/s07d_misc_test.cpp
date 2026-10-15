// S07-D: Mem_Copy (WIN32LIB/MEM/MEM_COPY.CPP), Reverse_Long/Reverse_Short/Swap_Long
// (WIN32LIB/MISC/REVERSE.CPP) and RandNumb (WIN32LIB/MISC/RANDOM.CPP).
#include "ra_test.h"
#include "s07d_asm_oracle.h"

#include <stdint.h>
#include <string.h>

extern "C" void Mem_Copy(void const *source, void *dest, unsigned long bytes_to_copy);
extern "C" long Reverse_Long(long number);
extern "C" short Reverse_Short(short number);
extern "C" long Swap_Long(long number);
extern "C" long RandNumb;   // same declaration as CODE/INIT.CPP

// ---------------------------------------------------------------------------------------------
// Mem_Copy: memmove with (source, dest, size) argument order; no-op for NULL or source == dest.
// ---------------------------------------------------------------------------------------------

RA_TEST(mem_copy_matches_memmove_with_overlap)
{
    unsigned char base[96];
    for (int i = 0; i < 96; i++) base[i] = (unsigned char)(i * 7 + 3);
    // Every size around the asm's 14-byte switch and its 4-byte alignment paths, both directions.
    for (int size = 0; size <= 40; size++) {
        for (int from = 0; from < 8; from++) {
            for (int to = 0; to < 8; to++) {
                unsigned char a[96], b[96];
                memcpy(a, base, 96);
                memcpy(b, base, 96);
                Mem_Copy(a + 20 + from, a + 20 + to, (unsigned long)size);
                memmove(b + 20 + to, b + 20 + from, (size_t)size);
                RA_CHECK_MEM_EQ(b, a, 96);
            }
        }
    }
}

RA_TEST(mem_copy_ignores_null_and_same_pointer)
{
    unsigned char a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Mem_Copy(0, a, 8);
    Mem_Copy(a, 0, 8);
    Mem_Copy(a, a, 8);
    const unsigned char expect[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    RA_CHECK_MEM_EQ(expect, a, 8);
}

// ---------------------------------------------------------------------------------------------
// Byte order helpers (IFF chunks are big endian).
// ---------------------------------------------------------------------------------------------

RA_TEST(reverse_known_values)
{
    RA_CHECK_EQ(0x44332211L, Reverse_Long(0x11223344L));
    RA_CHECK_EQ((long)(int32_t)0x80FFFFFFu, Reverse_Long((long)0xFFFFFF80L));   // 32-bit wrap as in eax
    RA_CHECK_EQ(0x2211, Reverse_Short(0x1122));
    RA_CHECK_EQ((short)0x80FF, Reverse_Short((short)0xFF80));
    RA_CHECK_EQ(0x33441122L, Swap_Long(0x11223344L));
    RA_CHECK_EQ(0x12345678L, Reverse_Long(Reverse_Long(0x12345678L)));
    RA_CHECK_EQ(0x12345678L, Swap_Long(Swap_Long(0x12345678L)));
}

// ---------------------------------------------------------------------------------------------
// RandNumb: only the seed variable is kept (ADR-0010). Its initial value is the asm's DATASEG one.
// ---------------------------------------------------------------------------------------------

RA_TEST(randnumb_initial_value_and_writable)
{
    RA_CHECK_EQ(0x12349876L, RandNumb);
    long saved = RandNumb;
    RandNumb = 0x1234;          // CODE/INIT.CPP writes the game seed here
    RA_CHECK_EQ(0x1234L, RandNumb);
    RandNumb = saved;
}

// The old library Random() from WIN32LIB/MISC/RANDOM.ASM, transcribed here for the record only. It
// is NOT part of the build: the game never called it, and the lockstep simulation must use the
// game's own C++ RandomClass / Sim_Random (ADR-0010). It updates bytes 0-2 of RandNumb (little
// endian) and returns byte0 ^ byte1:
//     al = b0 >> 2, carry = bit 1 of b0
//     b2 = rcl(b2): (b2 << 1) | carry, carry = old bit 7 of b2
//     b1 = rcl(b1): (b1 << 1) | carry, carry = old bit 7 of b1
//     carry = !carry;  al = al - b0 - carry;  carry = bit 0 of al
//     b0 = (b0 >> 1) | (carry << 7)
static unsigned char Old_Library_Random(unsigned char *b)
{
    unsigned char al = (unsigned char)(b[0] >> 2);
    unsigned carry = (b[0] >> 1) & 1;
    unsigned c2 = b[2] >> 7;
    b[2] = (unsigned char)((b[2] << 1) | carry);
    unsigned c1 = b[1] >> 7;
    b[1] = (unsigned char)((b[1] << 1) | c2);
    carry = !c1;
    al = (unsigned char)(al - b[0] - carry);
    carry = al & 1;
    b[0] = (unsigned char)((b[0] >> 1) | (carry << 7));
    return (unsigned char)(b[0] ^ b[1]);
}

RA_TEST(old_library_random_sequence)
{
    // First 16 values from the initial RandNumb 0x12349876 (bytes 76 98 34 12), produced by the
    // original instructions (see randnumb_sequence_matches_asm below).
    static const unsigned char expect[16] = {0x8B, 0x3D, 0x6F, 0x54, 0x6D, 0x38, 0x80, 0xF9,
                                                0x8F, 0xA0, 0x1F, 0x91, 0xF5, 0x80, 0xB5, 0xB0};
    unsigned char state[4] = {0x76, 0x98, 0x34, 0x12};
    for (int i = 0; i < 16; i++) RA_CHECK_EQ(expect[i], Old_Library_Random(state));
}

#if S07D_HAVE_ASM_ORACLE

RA_TEST(reverse_matches_asm)
{
    uint32_t s = 0x9E3779B9u;
    for (int i = 0; i < 10000; i++) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        long v = (long)(int32_t)s;
        RA_CHECK_EQ(ref_Reverse_Long(v), Reverse_Long(v));
        RA_CHECK_EQ(ref_Swap_Long(v), Swap_Long(v));
        RA_CHECK_EQ(ref_Reverse_Short((short)v), Reverse_Short((short)v));
    }
}

RA_TEST(randnumb_sequence_matches_asm)
{
    unsigned char a[4] = {0x76, 0x98, 0x34, 0x12}, b[4] = {0x76, 0x98, 0x34, 0x12};
    for (int i = 0; i < 100000; i++) {
        unsigned char ra = ref_Random(a);
        unsigned char rb = Old_Library_Random(b);
        if (ra != rb || memcmp(a, b, 4) != 0) {
            RA_CHECK_EQ(ra, rb);
            RA_CHECK_MEM_EQ(a, b, 4);
            break;
        }
    }
}

#endif

RA_TEST_MAIN()
