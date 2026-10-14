// S07-D test oracle: the ORIGINAL Westwood assembly for the data codecs, transcribed instruction for
// instruction into GNU assembler (Intel syntax) so the C++ ports can be compared against it at run
// time. See tests/s07d_asm_oracle.cpp. Only available on 32-bit x86 with GCC/Clang on ELF (the CI
// job linux-gcc-x86); elsewhere S07D_HAVE_ASM_ORACLE is 0 and the tests fall back to golden vectors.
//
// The oracle is test-only code. Nothing in the game links it.
#ifndef S07D_ASM_ORACLE_H
#define S07D_ASM_ORACLE_H

#if defined(__GNUC__) && defined(__i386__) && defined(__ELF__)
#define S07D_HAVE_ASM_ORACLE 1
#else
#define S07D_HAVE_ASM_ORACLE 0
#endif

#if S07D_HAVE_ASM_ORACLE
extern "C" {
// CODE/LCWCOMP.ASM, LCW_Comp. Reads up to 64 bytes past the end of the input: pad the input buffer.
int ref_LCW_Comp(void const *source, void *dest, int length);
// WIN32LIB/IFF/LCWUNCMP.ASM, LCW_Uncompress.
unsigned long ref_LCW_Uncompress(void const *source, void *dest, unsigned long length);
// WIN32LIB/WSA/XORDELTA.ASM.
unsigned int ref_Apply_XOR_Delta(char *target, char *delta);
void ref_Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int width, int nextrow, int copy);
// WIN32LIB/MISC/RANDOM.ASM, Random(), working on a 4-byte state instead of the global RandNumb.
unsigned char ref_Random(void *randnumb);
// WIN32LIB/MISC/REVERSE.ASM.
long ref_Reverse_Long(long number);
short ref_Reverse_Short(short number);
long ref_Swap_Long(long number);
}
#endif

#endif
