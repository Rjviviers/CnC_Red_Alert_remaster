// S07-D: LCW compressor (CODE/LCWCOMP.CPP, port of CODE/LCWCOMP.ASM) and decoder (CODE/LCW.CPP, which
// also replaces WIN32LIB/IFF/LCWUNCMP.ASM). No game assets are used.
//
// What is checked:
//   1. Hand-built streams with known output, one per command (format: docs/codebase/file-formats.md).
//   2. Hand-derived compressor output for small inputs (each derivation is in a comment).
//   3. Round trips compress -> LCW_Uncomp / LCW_Uncompress over many generated inputs.
//   4. "Game path unchanged": the new LCW_Uncomp against a verbatim copy of the 1995 C code it replaces.
//   5. On 32-bit x86 GCC (CI job linux-gcc-x86): both ports against the ORIGINAL asm instructions
//      (tests/s07d_asm_oracle.cpp), byte for byte.
#include "ra_test.h"
#include "s07d_asm_oracle.h"

#include <stdint.h>
#include <string.h>
#include <vector>

extern "C" int LCW_Comp(void const *source, void *dest, int length);
extern "C" unsigned long LCW_Uncompress(void *source, void *dest, unsigned long length);
int LCW_Uncomp(void const *source, void *dest, unsigned long length = 0);   // as in CODE/LCW.H

typedef std::vector<unsigned char> Bytes;

static Bytes B(std::initializer_list<int> list)
{
    Bytes b;
    for (int v : list) b.push_back((unsigned char)v);
    return b;
}

static Bytes S(const char *text) { return Bytes(text, text + strlen(text)); }

// Small deterministic generator (xorshift32), so every run tests the same inputs.
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    unsigned below(unsigned n) { return next() % n; }
};

// Inputs with different structure: random, few symbols, runs, repeats near and far.
static Bytes Make_Input(Rng &rng, size_t size, int kind)
{
    Bytes b(size);
    switch (kind) {
        case 0: for (size_t i = 0; i < size; i++) b[i] = (unsigned char)rng.next(); break;
        case 1: for (size_t i = 0; i < size; i++) b[i] = (unsigned char)(rng.below(4) + 'a'); break;
        case 2: {   // runs of random length, some longer than 64 (0xFE) and some long copies
            size_t i = 0;
            while (i < size) {
                size_t len = 1 + rng.below(rng.below(4) == 0 ? 300 : 20);
                unsigned char v = (unsigned char)rng.below(3);
                for (size_t k = 0; k < len && i < size; k++) b[i++] = v;
            }
            break;
        }
        default: {  // text-like: copies of earlier pieces, near (short copy) and far (medium/long copy)
            size_t i = 0;
            while (i < size) {
                if (i > 16 && rng.below(3) != 0) {
                    size_t len = 3 + rng.below(rng.below(5) == 0 ? 200 : 12);
                    size_t from = rng.below(2) ? i - 1 - rng.below(i < 4000 ? (unsigned)i : 4000u)
                                               : rng.below((unsigned)i);
                    for (size_t k = 0; k < len && i < size; k++) b[i++] = b[from + k];
                } else {
                    b[i++] = (unsigned char)(' ' + rng.below(90));
                }
            }
            break;
        }
    }
    return b;
}

static Bytes Compress(const Bytes &in)
{
    Bytes out(in.size() * 2 + 16);
    int len = LCW_Comp(in.empty() ? (const void *)"" : in.data(), out.data(), (int)in.size());
    out.resize(len);
    return out;
}

static Bytes Uncomp(const Bytes &stream, size_t room)
{
    Bytes out(room + 16, 0xCD);
    int len = LCW_Uncomp(stream.data(), out.data());
    out.resize(len);
    return out;
}

// ---------------------------------------------------------------------------------------------
// 1. Hand-built streams. Commands (n = command byte):
//    0cccpppp pppppppp   copy c+3 bytes from p bytes back in the output
//    10cccccc ...        copy the next c bytes of the stream (c=0 means end of data)
//    11cccccc w          copy c+3 bytes from output offset w (w little endian)
//    11111110 w b        w times byte b
//    11111111 w1 w2      copy w1 bytes from output offset w2
// ---------------------------------------------------------------------------------------------

RA_TEST(lcw_decode_literal)
{
    // 83 = literal of 3 bytes, 80 = end.
    Bytes out = Uncomp(B({0x83, 'C', 'n', 'C', 0x80}), 3);
    RA_CHECK(out == S("CnC"));
}

RA_TEST(lcw_decode_short_copy_overlapping)
{
    // "AB", then 0x10 0x02: c = 1 -> 4 bytes, p = 0x002 -> from 2 back. Overlap repeats "AB".
    Bytes out = Uncomp(B({0x82, 'A', 'B', 0x10, 0x02, 0x80}), 6);
    RA_CHECK(out == S("ABABAB"));
}

RA_TEST(lcw_decode_short_copy_12_bit_distance)
{
    // 300 literal-ish bytes would be long to write; instead: a run of 0x200 'x' (FE 00 02 'x'),
    // then 'y', then a short copy of 3 bytes from 0x201 back (op 0x02, 0x01): bytes 0..2 = "xxx".
    Bytes s = B({0xFE, 0x00, 0x02, 'x', 0x81, 'y', 0x02, 0x01, 0x80});
    Bytes out = Uncomp(s, 0x204);
    RA_CHECK_EQ(0x204, (int)out.size());
    RA_CHECK_EQ('y', out[0x200]);
    RA_CHECK(out[0x201] == 'x' && out[0x202] == 'x' && out[0x203] == 'x');
}

RA_TEST(lcw_decode_medium_copy_absolute)
{
    // "XYZ", then C0 00 00: c = 0 -> 3 bytes from offset 0.
    Bytes out = Uncomp(B({0x83, 'X', 'Y', 'Z', 0xC0, 0x00, 0x00, 0x80}), 6);
    RA_CHECK(out == S("XYZXYZ"));
    // C3 01 00: c = 3 -> 6 bytes from offset 1 ("YZ" then the copy reads its own output).
    out = Uncomp(B({0x83, 'X', 'Y', 'Z', 0xC3, 0x01, 0x00, 0x80}), 9);
    RA_CHECK(out == S("XYZYZYZYZ"));
}

RA_TEST(lcw_decode_long_copy)
{
    // "Q", then FF 05 00 00 00: 5 bytes from offset 0 -> "QQQQQQ".
    Bytes out = Uncomp(B({0x81, 'Q', 0xFF, 0x05, 0x00, 0x00, 0x00, 0x80}), 6);
    RA_CHECK(out == S("QQQQQQ"));
}

RA_TEST(lcw_decode_long_run)
{
    // FE 0A 00 7A: 10 times 'z'. FE 2C 01 00: 300 times 0.
    Bytes out = Uncomp(B({0xFE, 0x0A, 0x00, 'z', 0xFE, 0x2C, 0x01, 0x00, 0x80}), 310);
    RA_CHECK_EQ(310, (int)out.size());
    for (int i = 0; i < 10; i++) RA_CHECK_EQ('z', out[i]);
    for (int i = 10; i < 310; i++) RA_CHECK_EQ(0, out[i]);
}

RA_TEST(lcw_decode_run_exact_length_every_alignment)
{
    // The 1995 C code filled runs with aligned 8-byte stores and could write past the run.
    // The run must now end exactly where it says, whatever the alignment of the output.
    for (int offset = 0; offset < 8; offset++) {
        for (int count = 1; count <= 40; count++) {
            Bytes s;
            if (offset) { s.push_back((unsigned char)(0x80 + offset)); for (int i = 0; i < offset; i++) s.push_back('a'); }
            s.push_back(0xFE); s.push_back((unsigned char)count); s.push_back(0); s.push_back('r');
            s.push_back(0x80);
            Bytes out(offset + count + 16, 0xCD);
            int len = LCW_Uncomp(s.data(), out.data());
            RA_CHECK_EQ(offset + count, len);
            for (int i = 0; i < count; i++) RA_CHECK_EQ('r', out[offset + i]);
            for (int i = offset + count; i < (int)out.size(); i++) RA_CHECK_EQ(0xCD, out[i]);
        }
    }
}

RA_TEST(lcw_uncompress_is_bounded_like_the_asm)
{
    // LCW_Uncompress never writes more than 'length' bytes (WIN32LIB/IFF/LCWUNCMP.ASM did the same).
    Bytes s = B({0xFE, 0x0A, 0x00, 'z', 0x80});
    unsigned char out[16];
    memset(out, 0xCD, sizeof(out));
    RA_CHECK_EQ(4, (int)LCW_Uncompress(s.data(), out, 4));
    RA_CHECK_EQ('z', out[3]);
    RA_CHECK_EQ(0xCD, out[4]);

    // It stops when the output is full, without needing the end code.
    Bytes no_end = B({0x82, 'h', 'i'});
    memset(out, 0xCD, sizeof(out));
    RA_CHECK_EQ(2, (int)LCW_Uncompress(no_end.data(), out, 2));
    RA_CHECK(out[0] == 'h' && out[1] == 'i' && out[2] == 0xCD);

    // Length 0 writes nothing (the asm returned before reading the first command).
    RA_CHECK_EQ(0, (int)LCW_Uncompress(s.data(), out, 0));

    // A larger length than the data is fine: the end code stops it.
    memset(out, 0xCD, sizeof(out));
    RA_CHECK_EQ(10, (int)LCW_Uncompress(s.data(), out, sizeof(out)));
    RA_CHECK_EQ(0xCD, out[10]);
}

RA_TEST(lcw_uncomp_ignores_length)
{
    // The game's entry point decodes to the end code whatever length it is given (as before S07-D).
    Bytes s = B({0xFE, 0x0A, 0x00, 'z', 0x80});
    unsigned char out[16];
    RA_CHECK_EQ(10, LCW_Uncomp(s.data(), out, 3));
}

// ---------------------------------------------------------------------------------------------
// 2. Compressor output derived by hand from the asm's algorithm.
// ---------------------------------------------------------------------------------------------

RA_TEST(lcw_compress_known_output)
{
    // Empty input: just the end code. (Port-defined; the asm read past the input.)
    RA_CHECK(Compress(Bytes()) == B({0x80}));
    // One byte: the asm always starts with a 1-byte literal (81 b). (Port-defined for length 1.)
    RA_CHECK(Compress(S("A")) == B({0x81, 'A', 0x80}));
    // "AB": no earlier 'B', so the literal grows to 82.
    RA_CHECK(Compress(S("AB")) == B({0x82, 'A', 'B', 0x80}));
    // "ABABABAB": literal "AB"; at position 2 the match at 0 runs to the end (6 bytes), distance 2
    // <= 0xFFF and 6 <= 10 -> short copy ((6-3)<<4 | 0, 2) = 30 02.
    RA_CHECK(Compress(S("ABABABAB")) == B({0x82, 'A', 'B', 0x30, 0x02, 0x80}));
    // 100 x 'z': literal 'z'; at position 1 the byte 64 ahead is equal, so the run scan starts.
    // It reaches the end, and the asm then counts one byte less: 98 = 0x62 -> FE 62 00 'z'.
    // The last byte only has 1-byte matches, so it is a new literal: 81 'z'.
    Bytes z(100, 'z');
    RA_CHECK(Compress(z) == B({0x81, 'z', 0xFE, 0x62, 0x00, 'z', 0x81, 'z', 0x80}));
    // "abcdefghij" + "abcdefghij" + "abcdefghij": literal of 10, then the match at 0 is 20 long
    // (> 10, so not short) -> medium copy C0|(20-3) = D1, offset 00 00.
    RA_CHECK(Compress(S("abcdefghijabcdefghijabcdefghij")) ==
             B({0x8A, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 0xD1, 0x00, 0x00, 0x80}));
    // 63 different bytes fill one literal command (BF); the 64th starts a new one (81).
    Bytes lit(64);
    for (int i = 0; i < 64; i++) lit[i] = (unsigned char)(i * 3 + 1);
    Bytes c = Compress(lit);
    RA_CHECK_EQ(64 + 2 + 1, (int)c.size());
    RA_CHECK_EQ(0xBF, c[0]);
    RA_CHECK_EQ(0x81, c[64]);
    RA_CHECK_EQ(0x80, c[66]);
}

// ---------------------------------------------------------------------------------------------
// 3. Round trips.
// ---------------------------------------------------------------------------------------------

RA_TEST(lcw_round_trip)
{
    Rng rng(0x5EED1996);
    static const size_t sizes[] = {1, 2, 3, 4, 5, 63, 64, 65, 66, 127, 128, 129, 1000, 4096, 8192, 16384};
    for (size_t size : sizes) {
        for (int kind = 0; kind < 4; kind++) {
            Bytes in = Make_Input(rng, size, kind);
            Bytes c = Compress(in);
            RA_CHECK(c.size() <= size + size / 63 + 3);
            RA_CHECK(Uncomp(c, size) == in);

            Bytes out(size + 16, 0xCD);
            RA_CHECK_EQ(size, LCW_Uncompress(c.data(), out.data(), (unsigned long)size));
            RA_CHECK_MEM_EQ(in.data(), out.data(), size);
            RA_CHECK_EQ(0xCD, out[size]);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// 4. "Game path unchanged". Verbatim copy of LCW_Uncomp from CODE/LCW.CPP before S07-D, except
//    '(unsigned) dest_ptr' -> '(uintptr_t) dest_ptr' so it also builds for 64-bit targets.
//    It can write up to 4 bytes past a run, so it is only ever given padded buffers.
// ---------------------------------------------------------------------------------------------

static int Original_LCW_Uncomp(void const * source, void * dest, unsigned long )
{
	unsigned char * source_ptr, * dest_ptr, * copy_ptr, op_code, data;
	unsigned	  count, * word_dest_ptr, word_data;

	source_ptr = (unsigned char*) source;
	dest_ptr   = (unsigned char*) dest;

	while (1) {
		op_code = *source_ptr++;
		if (!(op_code & 0x80)) {
			count	 = (op_code >> 4) + 3;
			copy_ptr = dest_ptr - ((unsigned) *source_ptr++ + (((unsigned) op_code & 0x0f) << 8));
			while (count--) *dest_ptr++ = *copy_ptr++;
		} else {
			if (!(op_code & 0x40)) {
				if (op_code == 0x80) {
					return ((unsigned long) (dest_ptr - (unsigned char*) dest));
				} else {
					count = op_code & 0x3f;
					while (count--) *dest_ptr++ = *source_ptr++;
				}
			} else {
				if (op_code == 0xfe) {
					count = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
					word_data = data = *(source_ptr + 2);
					word_data  = (word_data << 24) + (word_data << 16) + (word_data << 8) + word_data;
					source_ptr += 3;
					copy_ptr = dest_ptr + 4 - ((uintptr_t) dest_ptr & 0x3);
					count -= (copy_ptr - dest_ptr);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;
					word_dest_ptr = (unsigned*) dest_ptr;
					dest_ptr += (count & 0xfffffffc);
					while (word_dest_ptr < (unsigned*) dest_ptr) {
						*word_dest_ptr		= word_data;
						*(word_dest_ptr + 1) = word_data;
						word_dest_ptr += 2;
					}
					copy_ptr = dest_ptr + (count & 0x3);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;
				} else {
					if (op_code == 0xff) {
						count	 = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						copy_ptr = (unsigned char*) dest + *(source_ptr + 2) + ((unsigned) *(source_ptr + 3) << 8);
						source_ptr += 4;
						while (count--) *dest_ptr++ = *copy_ptr++;
					} else {
						count = (op_code & 0x3f) + 3;
						copy_ptr = (unsigned char*) dest + *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						source_ptr += 2;
						while (count--) *dest_ptr++ = *copy_ptr++;
					}
				}
			}
		}
	}
}

RA_TEST(lcw_game_decoder_matches_1995_code)
{
    // Streams from the compressor (as in the game's [MapPack]/[OverlayPack] and LCW pipes/straws),
    // decoded into buffers at every 4-byte alignment.
    Rng rng(0xC0FFEE);
    static const size_t sizes[] = {2, 7, 65, 66, 200, 1000, 4096, 8192};
    for (size_t size : sizes) {
        for (int kind = 0; kind < 4; kind++) {
            Bytes in = Make_Input(rng, size, kind);
            Bytes c = Compress(in);
            for (int align = 0; align < 4; align++) {
                Bytes a(size + 32, 0xCD), b(size + 32, 0xCD);
                int la = LCW_Uncomp(c.data(), a.data() + align);
                int lb = Original_LCW_Uncomp(c.data(), b.data() + align, 0);
                RA_CHECK_EQ(lb, la);
                RA_CHECK_MEM_EQ(b.data() + align, a.data() + align, size);
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// 5. Against the original asm (32-bit x86 GCC only).
// ---------------------------------------------------------------------------------------------
#if S07D_HAVE_ASM_ORACLE

RA_TEST(lcw_compress_matches_asm)
{
    Rng rng(0xA5A5);
    int compared = 0;
    for (int iter = 0; iter < 400; iter++) {
        size_t size = 2 + rng.below(iter < 300 ? 600 : 9000);
        Bytes in = Make_Input(rng, size, iter % 4);
        // The asm reads up to 64 bytes past the input; give it padding, two different ways,
        // to show the padding never changes its output.
        for (int pad = 0; pad < 2; pad++) {
            Bytes padded(in);
            padded.resize(size + 128, pad ? in[size - 1] : (unsigned char)~in[size - 1]);
            Bytes ref(size * 2 + 16), out(size * 2 + 16);
            int lr = ref_LCW_Comp(padded.data(), ref.data(), (int)size);
            int lo = LCW_Comp(in.data(), out.data(), (int)size);
            RA_CHECK_EQ(lr, lo);
            if (lr == lo) RA_CHECK_MEM_EQ(ref.data(), out.data(), (size_t)lr);
            compared++;
        }
    }
    RA_CHECK_EQ(800, compared);
}

RA_TEST(lcw_uncompress_matches_asm)
{
    Rng rng(0xBEEF);
    for (int iter = 0; iter < 300; iter++) {
        size_t size = 1 + rng.below(iter < 200 ? 500 : 9000);
        Bytes in = Make_Input(rng, size, iter % 4);
        Bytes c = Compress(in);
        // Full length, a little more, and cut short (the bounded case).
        unsigned long lengths[3] = {(unsigned long)size, (unsigned long)size + 7,
                                    (unsigned long)rng.below((unsigned)size + 1)};
        for (unsigned long length : lengths) {
            for (int align = 0; align < 4; align++) {
                Bytes a(size + 40, 0xCD), b(size + 40, 0xCD);
                unsigned long la = LCW_Uncompress(c.data(), a.data() + align, length);
                unsigned long lb = ref_LCW_Uncompress(c.data(), b.data() + align, length);
                RA_CHECK_EQ(lb, la);
                RA_CHECK(a == b);
            }
        }
    }
    // Hand-made streams with long runs and long copies that reach the asm's 4-byte paths,
    // at odd output offsets.
    for (int offset = 0; offset < 8; offset++) {
        for (int count = 1; count < 140; count += 3) {
            Bytes s;
            s.push_back((unsigned char)(0x80 + offset + 1));
            for (int i = 0; i <= offset; i++) s.push_back((unsigned char)('a' + i));
            s.push_back(0xFE); s.push_back((unsigned char)count); s.push_back(0); s.push_back('r');
            s.push_back(0xFF); s.push_back((unsigned char)count); s.push_back(0); s.push_back(0); s.push_back(0);
            s.push_back(0xFF); s.push_back((unsigned char)count); s.push_back(0);
            s.push_back((unsigned char)(offset + count / 2)); s.push_back(0);   // overlapping long copy
            s.push_back(0x80);
            unsigned long full = (unsigned long)(offset + 1 + 3 * count);
            unsigned long bounds[3] = {full + 5, full, full - 1 - (unsigned long)(count / 2)};
            for (unsigned long bound : bounds) {
                Bytes a(4 * count + 64, 0xCD), b(4 * count + 64, 0xCD);
                RA_CHECK_EQ(ref_LCW_Uncompress(s.data(), b.data(), bound),
                            LCW_Uncompress(s.data(), a.data(), bound));
                RA_CHECK(a == b);
            }
        }
    }
}

#endif

RA_TEST_MAIN()
