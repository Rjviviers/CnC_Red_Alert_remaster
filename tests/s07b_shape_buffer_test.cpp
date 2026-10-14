// Test for WIN32LIB/SHAPE/SHAPE.CPP (S07-B), the port of WIN32LIB/SHAPE/SHAPE.ASM.
// SHAPE.ASM (lines 50-95): _ShapeBuffer and _ShapeBufferSize start at 0 ("dd 0");
// Set_Shape_Buffer(buff, size) stores size in _ShapeBufferSize and buff in _ShapeBuffer.
#include "ra_test.h"

extern "C" {
extern char *_ShapeBuffer;
extern long _ShapeBufferSize;
void Set_Shape_Buffer(void const *buffer, int size);
}

RA_TEST(shape_buffer_starts_empty) {
    RA_CHECK(_ShapeBuffer == 0);
    RA_CHECK_EQ(0, _ShapeBufferSize);
}

RA_TEST(set_shape_buffer_stores_pointer_and_size) {
    static char buffer[4000];
    Set_Shape_Buffer(buffer, 4000);
    RA_CHECK(_ShapeBuffer == buffer);
    RA_CHECK_EQ(4000, _ShapeBufferSize);
    Set_Shape_Buffer(0, 0);
    RA_CHECK(_ShapeBuffer == 0);
    RA_CHECK_EQ(0, _ShapeBufferSize);
}

RA_TEST_MAIN()
