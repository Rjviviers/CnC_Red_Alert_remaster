// Minimal unit-test framework for the Red Alert port (no external dependencies).
// See docs/building.md "Unit tests". Usage:
//
//   #include "ra_test.h"
//   RA_TEST(lcw_round_trip) {
//       RA_CHECK(result != nullptr);
//       RA_CHECK_EQ(expected_size, actual_size);
//       RA_CHECK_MEM_EQ(expected, actual, size);
//   }
//   RA_TEST_MAIN()   // exactly once per test executable
//
// A test executable returns 0 when every check passed, 1 otherwise. CTest runs it.
#ifndef RA_TEST_H
#define RA_TEST_H

#include <cstdio>
#include <cstring>

namespace ra_test {

typedef void (*TestFn)();

struct TestCase {
    const char *name;
    TestFn fn;
    TestCase *next;
};

inline TestCase *&registry() { static TestCase *head = 0; return head; }
inline int &failures() { static int count = 0; return count; }

struct Registrar {
    TestCase node;
    Registrar(const char *name, TestFn fn) {
        node.name = name;
        node.fn = fn;
        node.next = 0;
        TestCase **tail = &registry();          // keep declaration order
        while (*tail) tail = &(*tail)->next;
        *tail = &node;
    }
};

inline void fail(const char *file, int line, const char *what) {
    std::printf("  FAIL %s:%d: %s\n", file, line, what);
    ++failures();
}

inline int run_all() {
    int tests = 0, failed_tests = 0;
    for (TestCase *t = registry(); t; t = t->next) {
        int before = failures();
        std::printf("[ RUN  ] %s\n", t->name);
        t->fn();
        bool ok = failures() == before;
        std::printf("[ %s ] %s\n", ok ? " OK " : "FAIL", t->name);
        ++tests;
        if (!ok) ++failed_tests;
    }
    std::printf("%d test(s), %d failed\n", tests, failed_tests);
    return failed_tests == 0 ? 0 : 1;
}

} // namespace ra_test

#define RA_TEST(name)                                                    \
    static void ra_test_fn_##name();                                     \
    static ra_test::Registrar ra_test_reg_##name(#name, ra_test_fn_##name); \
    static void ra_test_fn_##name()

#define RA_CHECK(cond)                                                   \
    do { if (!(cond)) ra_test::fail(__FILE__, __LINE__, #cond); } while (0)

#define RA_CHECK_EQ(expected, actual)                                    \
    do {                                                                 \
        if (!((expected) == (actual))) {                                 \
            char ra_msg_[256];                                           \
            std::snprintf(ra_msg_, sizeof(ra_msg_), "%s == %s (got %lld vs %lld)", \
                          #expected, #actual, (long long)(expected), (long long)(actual)); \
            ra_test::fail(__FILE__, __LINE__, ra_msg_);                  \
        }                                                                \
    } while (0)

#define RA_CHECK_MEM_EQ(expected, actual, size)                          \
    do {                                                                 \
        if (std::memcmp((expected), (actual), (size)) != 0)              \
            ra_test::fail(__FILE__, __LINE__, "memory differs: " #expected " vs " #actual); \
    } while (0)

#define RA_TEST_MAIN() int main() { return ra_test::run_all(); }

#endif // RA_TEST_H
