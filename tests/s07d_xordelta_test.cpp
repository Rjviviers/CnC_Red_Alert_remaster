// S07-D: XOR-delta decoder (WIN32LIB/WSA/XORDELTA.CPP, port of WIN32LIB/WSA/XORDELTA.ASM).
// Format: docs/codebase/file-formats.md. No game assets are used.
//
//   00000000 c v     SHORTRUN   c times byte v         0ccccccc d..   SHORTDUMP  next c bytes
//   1sssssss         SHORTSKIP  skip s (1..127)        10000000 w     word w (little endian):
//     w = 0 STOP;  w = 0x0001..0x7FFF LONGSKIP w;  w = 10cc.. LONGDUMP c = w & 0x3FFF;
//     w = 11cc.. v LONGRUN c = w & 0x3FFF times byte v
// "Apply" XORs each byte onto the target; the viewport version can also store (copy) instead.
#include "ra_test.h"
#include "s07d_asm_oracle.h"

#include <stdint.h>
#include <string.h>
#include <vector>

extern "C" unsigned int Apply_XOR_Delta(char *target, char *delta);
extern "C" void Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int width, int nextrow, int copy);

static const int DO_XOR = 0;
static const int DO_COPY = 1;

typedef std::vector<unsigned char> Bytes;

static Bytes B(std::initializer_list<int> list)
{
    Bytes b;
    for (int v : list) b.push_back((unsigned char)v);
    return b;
}

struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    unsigned below(unsigned n) { return next() % n; }
};

RA_TEST(xor_delta_short_commands)
{
    // 03 11 22 33: dump 3 bytes. 82: skip 2. 00 04 FF: run of 4 x FF. 80 00 00: stop.
    Bytes delta = B({0x03, 0x11, 0x22, 0x33, 0x82, 0x00, 0x04, 0xFF, 0x80, 0x00, 0x00});
    Bytes target = B({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A});
    RA_CHECK_EQ(0u, Apply_XOR_Delta((char *)target.data(), (char *)delta.data()));
    Bytes expect = B({0x01 ^ 0x11, 0x02 ^ 0x22, 0x03 ^ 0x33, 0x04, 0x05,
                      0x06 ^ 0xFF, 0x07 ^ 0xFF, 0x08 ^ 0xFF, 0x09 ^ 0xFF, 0x0A});
    RA_CHECK(target == expect);
}

RA_TEST(xor_delta_long_commands)
{
    // 80 05 00: long skip 5. 80 03 80 AA BB CC: long dump (0x8003 -> 3 bytes).
    // 80 04 C0 77: long run (0xC004 -> 4 x 77). 80 00 00: stop.
    Bytes delta = B({0x80, 0x05, 0x00, 0x80, 0x03, 0x80, 0xAA, 0xBB, 0xCC,
                     0x80, 0x04, 0xC0, 0x77, 0x80, 0x00, 0x00});
    Bytes target(14, 0x0F);
    Apply_XOR_Delta((char *)target.data(), (char *)delta.data());
    Bytes expect = B({0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F ^ 0xAA, 0x0F ^ 0xBB, 0x0F ^ 0xCC,
                      0x0F ^ 0x77, 0x0F ^ 0x77, 0x0F ^ 0x77, 0x0F ^ 0x77, 0x0F, 0x0F});
    RA_CHECK(target == expect);

    // A long skip of 0x7FFF is still a skip (the highest positive 16-bit value).
    Bytes far = B({0x80, 0xFF, 0x7F, 0x01, 0x42, 0x80, 0x00, 0x00});
    Bytes big(0x8001, 0);
    Apply_XOR_Delta((char *)big.data(), (char *)far.data());
    RA_CHECK_EQ(0x42, big[0x7FFF]);
    RA_CHECK_EQ(0, big[0x7FFE]);
}

RA_TEST(xor_delta_viewport_wraps_rows)
{
    // A 4-pixel-wide image in a buffer with a 6-byte pitch (nextrow = 6).
    // 06 a b c d e f: dump 6 -> row 0 cols 0-3, row 1 cols 0-1.
    // 83: skip 3 -> column 2+3 = 5 -> row 2, column 1. 00 02 55: run of 2 -> row 2 cols 1-2.
    Bytes delta = B({0x06, 'a', 'b', 'c', 'd', 'e', 'f', 0x83, 0x00, 0x02, 0x55, 0x80, 0x00, 0x00});
    Bytes buffer(6 * 3, '.');
    Apply_XOR_Delta_To_Page_Or_Viewport(buffer.data(), delta.data(), 4, 6, DO_COPY);
    const char *expect = "abcd.."
                         "ef...."
                         ".UU...";
    RA_CHECK_MEM_EQ(expect, buffer.data(), 18);

    // The same delta in XOR mode XORs instead of storing.
    Bytes zero(6 * 3, 0);
    Apply_XOR_Delta_To_Page_Or_Viewport(zero.data(), delta.data(), 4, 6, DO_XOR);
    for (int i = 0; i < 18; i++) RA_CHECK_EQ(expect[i] == '.' ? 0 : expect[i], zero[i]);

    // Long skip across several rows: 80 0A 00 skips 10 pixels = 2 rows + 2 columns.
    Bytes jump = B({0x80, 0x0A, 0x00, 0x01, 'X', 0x80, 0x00, 0x00});
    Bytes buf2(6 * 3, '.');
    Apply_XOR_Delta_To_Page_Or_Viewport(buf2.data(), jump.data(), 4, 6, DO_COPY);
    RA_CHECK_EQ('X', buf2[2 * 6 + 2]);
}

// A simple encoder, written from the format, to show apply(A, encode(A ^ B)) == B.
static Bytes Encode_Delta(const Bytes &from, const Bytes &to)
{
    Bytes out;
    size_t n = from.size(), i = 0;
    while (i < n) {
        unsigned char x = from[i] ^ to[i];
        size_t j = i;
        if (x == 0) {
            while (j < n && (from[j] ^ to[j]) == 0 && j - i < 0x7FFF) j++;
            size_t skip = j - i;
            if (skip < 128) out.push_back((unsigned char)(0x80 | skip));
            else { out.push_back(0x80); out.push_back(skip & 0xFF); out.push_back((unsigned char)(skip >> 8)); }
        } else {
            while (j < n && (from[j] ^ to[j]) == x && j - i < 0x3FFF) j++;
            size_t run = j - i;
            if (run >= 3) {
                if (run < 256) { out.push_back(0); out.push_back((unsigned char)run); }
                else { out.push_back(0x80); out.push_back(run & 0xFF); out.push_back((unsigned char)(0xC0 | (run >> 8))); }
                out.push_back(x);
            } else {
                j = i;
                while (j < n && (from[j] ^ to[j]) != 0 && j - i < 0x3FFF) j++;
                size_t dump = j - i;
                if (dump < 128) out.push_back((unsigned char)dump);
                else { out.push_back(0x80); out.push_back(dump & 0xFF); out.push_back((unsigned char)(0x80 | (dump >> 8))); }
                for (size_t k = i; k < j; k++) out.push_back(from[k] ^ to[k]);
            }
        }
        i = j;
    }
    out.push_back(0x80); out.push_back(0); out.push_back(0);
    return out;
}

static Bytes Frame(Rng &rng, size_t n, const Bytes *base)
{
    Bytes f(n);
    for (size_t i = 0; i < n; i++) {
        if (base && rng.below(4) != 0) f[i] = (*base)[i];          // mostly unchanged
        else f[i] = (unsigned char)(rng.below(3) == 0 ? 0x33 : rng.next());
    }
    return f;
}

RA_TEST(xor_delta_round_trip)
{
    Rng rng(1994);
    static const size_t sizes[] = {1, 2, 127, 128, 300, 20000};
    for (size_t n : sizes) {
        for (int k = 0; k < 5; k++) {
            Bytes a = Frame(rng, n, 0);
            Bytes b = Frame(rng, n, &a);
            Bytes d = Encode_Delta(a, b);
            Bytes t = a;
            Apply_XOR_Delta((char *)t.data(), (char *)d.data());
            RA_CHECK(t == b);
            // Viewport with nextrow == width is the same as the linear version.
            Bytes v = a;
            Apply_XOR_Delta_To_Page_Or_Viewport(v.data(), d.data(), (int)(n < 40 ? n : 40), (int)(n < 40 ? n : 40), DO_XOR);
            RA_CHECK(v == b);
        }
    }
}

#if S07D_HAVE_ASM_ORACLE

// A random but valid delta covering at most 'pixels' pixels.
static Bytes Random_Delta(Rng &rng, size_t pixels)
{
    Bytes d;
    size_t pos = 0;
    while (pos < pixels) {
        size_t left = pixels - pos;
        unsigned kind = rng.below(6);
        size_t count;
        switch (kind) {
            case 0: count = 1 + rng.below(left < 255 ? (unsigned)left : 255u);
                d.push_back(0); d.push_back((unsigned char)count); d.push_back((unsigned char)rng.next()); break;
            case 1: count = 1 + rng.below(left < 127 ? (unsigned)left : 127u);
                d.push_back((unsigned char)count); for (size_t i = 0; i < count; i++) d.push_back((unsigned char)rng.next()); break;
            case 2: count = 1 + rng.below(left < 127 ? (unsigned)left : 127u);
                d.push_back((unsigned char)(0x80 | count)); break;
            case 3: count = 1 + rng.below(left < 0x7FFF ? (unsigned)left : 0x7FFFu);
                d.push_back(0x80); d.push_back(count & 0xFF); d.push_back((unsigned char)(count >> 8)); break;
            case 4: count = 1 + rng.below(left < 0x3FFF ? (unsigned)left : 0x3FFFu);
                d.push_back(0x80); d.push_back(count & 0xFF); d.push_back((unsigned char)(0x80 | (count >> 8)));
                for (size_t i = 0; i < count; i++) d.push_back((unsigned char)rng.next()); break;
            default: count = 1 + rng.below(left < 0x3FFF ? (unsigned)left : 0x3FFFu);
                d.push_back(0x80); d.push_back(count & 0xFF); d.push_back((unsigned char)(0xC0 | (count >> 8)));
                d.push_back((unsigned char)rng.next()); break;
        }
        pos += count;
    }
    d.push_back(0x80); d.push_back(0); d.push_back(0);
    return d;
}

RA_TEST(xor_delta_matches_asm)
{
    Rng rng(0x0DE17A);
    for (int iter = 0; iter < 500; iter++) {
        size_t n = 1 + rng.below(iter < 400 ? 400 : 40000);
        Bytes d = Random_Delta(rng, n);
        Bytes a(n), b;
        for (size_t i = 0; i < n; i++) a[i] = (unsigned char)rng.next();
        b = a;
        RA_CHECK_EQ(ref_Apply_XOR_Delta((char *)b.data(), (char *)d.data()),
                    Apply_XOR_Delta((char *)a.data(), (char *)d.data()));
        RA_CHECK(a == b);
    }
}

RA_TEST(xor_delta_viewport_matches_asm)
{
    Rng rng(0x5CA1E);
    for (int iter = 0; iter < 600; iter++) {
        int width = 1 + (int)rng.below(48);
        int height = 1 + (int)rng.below(30);
        int nextrow = width + (int)rng.below(20);
        int copy = (int)rng.below(2);
        Bytes d = Random_Delta(rng, (size_t)width * height);
        Bytes a((size_t)nextrow * (height + 1) + width), b;
        for (size_t i = 0; i < a.size(); i++) a[i] = (unsigned char)rng.next();
        b = a;
        ref_Apply_XOR_Delta_To_Page_Or_Viewport(b.data(), d.data(), width, nextrow, copy);
        Apply_XOR_Delta_To_Page_Or_Viewport(a.data(), d.data(), width, nextrow, copy);
        RA_CHECK(a == b);
    }
}

#endif

RA_TEST_MAIN()
