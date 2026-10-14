// S07-A: what is left of the video-memory icon cache (WIN32LIB/DRAWBUFF/STMPCACH.CPP,
// replacing STMPCACH.ASM; ADR-0016).
#include "ra_test.h"

#include <cstring>

extern "C" void Cache_Copy_Icon(void const *icon_ptr, void *surface, int pitch);
extern "C" int Is_Icon_Cached(void const *icon_data, int icon);

// The cache is removed: every icon reports "not cached", so GBUFFER.H's Draw_Stamp
// falls back to Buffer_Draw_Stamp_Clip.
RA_TEST(no_icon_is_ever_cached) {
    unsigned char dummy[64] = {0};
    RA_CHECK_EQ(-1, Is_Icon_Cached(dummy, 0));
    RA_CHECK_EQ(-1, Is_Icon_Cached(dummy, 5));
    RA_CHECK_EQ(-1, Is_Icon_Cached(NULL, 0));
}

// Cache_Copy_Icon: 24 rows of 24 bytes, destination rows `pitch` bytes apart.
RA_TEST(copy_icon_uses_pitch) {
    unsigned char icon[24 * 24];
    for (int i = 0; i < 24 * 24; i++) icon[i] = (unsigned char)(i % 251 + 1);
    const int pitch = 30;
    unsigned char surface[24 * 30 + 8];
    std::memset(surface, 0xEE, sizeof(surface));
    Cache_Copy_Icon(icon, surface, pitch);
    for (int y = 0; y < 24; y++) {
        RA_CHECK_MEM_EQ(&icon[y * 24], &surface[y * pitch], 24);
        for (int x = 24; x < pitch && y * pitch + x < (int)sizeof(surface); x++)
            RA_CHECK_EQ(0xEE, surface[y * pitch + x]);
    }
}

RA_TEST_MAIN()
