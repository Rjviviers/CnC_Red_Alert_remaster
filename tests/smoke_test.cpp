// Smoke test: proves the test harness and the shared build settings work.
#include "ra_test.h"

RA_TEST(harness_runs) {
    RA_CHECK(1 + 1 == 2);
    RA_CHECK_EQ(4, (int)sizeof(int));
}

RA_TEST(target_is_32bit_when_requested) {
#if defined(RA_EXPECT_32BIT)
    RA_CHECK_EQ(4, (int)sizeof(void *));   // ADR-0003: 32-bit x86 first
#endif
}

RA_TEST_MAIN()
