// Helpers for the S07-A surface primitive tests (WIN32LIB/DRAWBUFF ports).
//
// A TestSurface is a small byte buffer with a viewport (GVPLayout, the C++ mirror of
// GraphicViewPortClass) on a sub-rectangle of it. Expected images are written as strings,
// one per row of the WHOLE buffer: '.' is colour 0, '1'-'9' and 'A'-'Z' are colours 1-35.
#ifndef S07A_SURFACE_H
#define S07A_SURFACE_H

#include "ra_test.h"
#include "GVPORT.H"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

struct TestSurface {
    int stride;                     // bytes per row of the whole buffer
    int rows;                       // rows in the whole buffer
    std::vector<unsigned char> buf;
    GVPLayout vp;

    // Viewport of size w x h at (x, y) in a stride x rows buffer. `pitch` moves part of the row
    // padding from XAdd to Pitch (the DirectDraw extra pitch); the routines must add both.
    TestSurface(int stride_, int rows_, int x, int y, int w, int h, int pitch = 0)
        : stride(stride_), rows(rows_), buf(stride_ * rows_, 0)
    {
        std::memset(&vp, 0, sizeof(vp));
        vp.Offset = (long)(size_t)(&buf[0] + y * stride + x);
        vp.Width = w;
        vp.Height = h;
        vp.XAdd = stride - w - pitch;
        vp.Pitch = pitch;
        vp.XPos = x;
        vp.YPos = y;
    }

    TestSurface(const TestSurface &) = delete;              // vp.Offset points into buf
    TestSurface &operator=(const TestSurface &) = delete;

    // Whole-buffer viewport.
    TestSurface(int w, int h) : TestSurface(w, h, 0, 0, w, h) {}

    unsigned char &at(int x, int y) { return buf[y * stride + x]; }

    void fill(unsigned char c) { std::memset(&buf[0], c, buf.size()); }

    // Load the buffer from a picture in the same format as expect().
    void load(const char *const *picture)
    {
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < stride; x++)
                at(x, y) = decode(picture[y][x]);
    }

    static unsigned char decode(char c)
    {
        if (c == '.') return 0;
        if (c >= '1' && c <= '9') return (unsigned char)(c - '0');
        if (c >= 'A' && c <= 'Z') return (unsigned char)(c - 'A' + 10);
        return 0xFF;
    }

    static char encode(unsigned char v)
    {
        if (v == 0) return '.';
        if (v <= 9) return (char)('0' + v);
        if (v <= 35) return (char)('A' + v - 10);
        return '?';
    }

    std::string dump() const
    {
        std::string s;
        for (int y = 0; y < rows; y++) {
            s += "    ";
            for (int x = 0; x < stride; x++) s += encode(buf[y * stride + x]);
            s += "\n";
        }
        return s;
    }

    // Compare with an expected picture; on mismatch print both.
    bool matches(const char *const *picture) const
    {
        for (int y = 0; y < rows; y++) {
            if ((int)std::strlen(picture[y]) != stride) return false;
            for (int x = 0; x < stride; x++)
                if (buf[y * stride + x] != decode(picture[y][x])) return false;
        }
        return true;
    }
};

#define S07A_EXPECT_PICTURE(surface, ...)                                         \
    do {                                                                          \
        static const char *const s07a_pic_[] = { __VA_ARGS__ };                   \
        if (!(surface).matches(s07a_pic_)) {                                      \
            std::printf("  got:\n%s  expected:\n", (surface).dump().c_str());     \
            for (unsigned i_ = 0; i_ < sizeof(s07a_pic_) / sizeof(*s07a_pic_); i_++) \
                std::printf("    %s\n", s07a_pic_[i_]);                           \
            ra_test::fail(__FILE__, __LINE__, "picture differs");                 \
        }                                                                         \
    } while (0)

#define S07A_LOAD_PICTURE(surface, ...)                                           \
    do {                                                                          \
        static const char *const s07a_pic_[] = { __VA_ARGS__ };                   \
        (surface).load(s07a_pic_);                                                \
    } while (0)

#endif // S07A_SURFACE_H
