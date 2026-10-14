// S08: the platform timer reproduces WinTimerClass(60): timeSetEvent(1000/60) = 16 ms per tick.
#include "ra_test.h"
#include "platform.h"

RA_TEST(tick_period_is_integer_division_like_timeSetEvent) {
    RA_CHECK_EQ(16u, Platform_Tick_Period_Ms(60));    // 1000/60: 62.5 ticks per real second
    RA_CHECK_EQ(10u, Platform_Tick_Period_Ms(100));
    RA_CHECK_EQ(1u, Platform_Tick_Period_Ms(1000));
    RA_CHECK_EQ(1u, Platform_Tick_Period_Ms(0));      // guarded, never divides by zero
    RA_CHECK_EQ(1u, Platform_Tick_Period_Ms(5000));
}

RA_TEST(ticks_in_elapsed_time) {
    RA_CHECK_EQ(0u, Platform_Ticks_In(0, 60));
    RA_CHECK_EQ(0u, Platform_Ticks_In(15, 60));
    RA_CHECK_EQ(1u, Platform_Ticks_In(16, 60));
    RA_CHECK_EQ(62u, Platform_Ticks_In(1000, 60));    // one second
    RA_CHECK_EQ(3750u, Platform_Ticks_In(60000, 60)); // one minute = 3750 ticks, not 3600
    // Past the 32-bit millisecond wrap (49.7 days) the count keeps going.
    RA_CHECK_EQ(268435456u, Platform_Ticks_In(4294967296ULL, 60));
}

RA_TEST(clock_is_monotonic_and_sleeps) {
    uint64_t a = Platform_Milliseconds64();
    Platform_Sleep(20);
    uint64_t b = Platform_Milliseconds64();
    RA_CHECK(b >= a + 19);
    RA_CHECK(b - a < 2000);
    RA_CHECK((uint32_t)b == Platform_Milliseconds() || (uint32_t)b + 1 == Platform_Milliseconds() ||
             Platform_Milliseconds() - (uint32_t)b < 50);
}

RA_TEST(poll_hooks_add_remove) {
    struct Counter { static void Hook(void *user) { ++*(int *)user; } };
    int a = 0, b = 0;
    RA_CHECK(Platform_Add_Poll_Hook(Counter::Hook, &a));
    RA_CHECK(Platform_Add_Poll_Hook(Counter::Hook, &a));      // same pair: not added twice
    RA_CHECK(Platform_Add_Poll_Hook(Counter::Hook, &b));
    RA_CHECK(Platform_Remove_Poll_Hook(Counter::Hook, &a));
    RA_CHECK(!Platform_Remove_Poll_Hook(Counter::Hook, &a)); // already removed
    RA_CHECK(Platform_Remove_Poll_Hook(Counter::Hook, &b));
    RA_CHECK(!Platform_Add_Poll_Hook(0, 0));
    // Fill the table: the limit is enforced.
    int slots[PLATFORM_MAX_POLL_HOOKS + 1];
    int added = 0;
    for (int i = 0; i <= PLATFORM_MAX_POLL_HOOKS; i++) {
        if (Platform_Add_Poll_Hook(Counter::Hook, &slots[i])) added++;
    }
    RA_CHECK_EQ(PLATFORM_MAX_POLL_HOOKS, added);
    for (int i = 0; i <= PLATFORM_MAX_POLL_HOOKS; i++) Platform_Remove_Poll_Hook(Counter::Hook, &slots[i]);
}

RA_TEST(input_queue_is_empty_without_events) {
    PlatformInputEvent e;
    RA_CHECK(!Platform_Get_Input(&e));
    RA_CHECK(!Platform_Peek_Input(&e));
    RA_CHECK_EQ(0, Platform_Pump_Events());     // not initialised: does nothing
    RA_CHECK_EQ(16, (int)sizeof(PlatformInputEvent));
    RA_CHECK_EQ(24, (int)sizeof(PlatformVideoConfig));
}

RA_TEST_MAIN()
