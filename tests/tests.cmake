# Unit tests (Wave 3 prep). See docs/building.md "Unit tests".
# Included from the root CMakeLists.txt with include() (not add_subdirectory), so repo-relative paths work.
#
# Add a test with:
#   ra_add_test(NAME lcw_test
#               SOURCES tests/lcw_test.cpp CODE/LCW.CPP
#               INCLUDES CODE WIN32LIB/INCLUDE)
# SOURCES and INCLUDES are repo-relative. Each test is its own executable, built with the same
# settings as the game (architecture, packing, defines), so tests see the same struct layouts.

# All test executables, so CI can build just the tests: cmake --build build --target ra_tests
add_custom_target(ra_tests)

function(ra_add_test)
  cmake_parse_arguments(T "" "NAME" "SOURCES;INCLUDES" ${ARGN})
  if(NOT T_NAME OR NOT T_SOURCES)
    message(FATAL_ERROR "ra_add_test: NAME and SOURCES are required")
  endif()
  add_executable(${T_NAME})
  ra_target_sources(${T_NAME} ${T_SOURCES})
  ra_target_includes(${T_NAME} tests ${T_INCLUDES})
  ra_apply_build_settings(${T_NAME})
  if(RA_TARGET_ARCH STREQUAL "x86")
    target_compile_definitions(${T_NAME} PRIVATE RA_EXPECT_32BIT=1)
  endif()
  add_test(NAME ${T_NAME} COMMAND ${T_NAME})
  add_dependencies(ra_tests ${T_NAME})
endfunction()

ra_add_test(NAME smoke_test SOURCES tests/smoke_test.cpp)

# Session-specific tests are listed below, one block per session or package, to keep merges simple.
# --- S07-A ---
# Surface primitives ported from WIN32LIB/DRAWBUFF and WIN32LIB/MISC asm. One test per asm file.
# tests/s07a_surface.h holds the shared viewport helpers.
ra_add_test(NAME s07a_bitblit_test  SOURCES tests/s07a_bitblit_test.cpp  WIN32LIB/DRAWBUFF/BITBLIT.CPP  INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_clear_test    SOURCES tests/s07a_clear_test.cpp    WIN32LIB/DRAWBUFF/CLEAR.CPP    INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_drawline_test SOURCES tests/s07a_drawline_test.cpp WIN32LIB/DRAWBUFF/DRAWLINE.CPP INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_fillrect_test SOURCES tests/s07a_fillrect_test.cpp WIN32LIB/DRAWBUFF/FILLRECT.CPP INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_getpix_test   SOURCES tests/s07a_getpix_test.cpp   WIN32LIB/DRAWBUFF/GETPIX.CPP   INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_putpix_test   SOURCES tests/s07a_putpix_test.cpp   WIN32LIB/DRAWBUFF/PUTPIX.CPP   INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_remap_test    SOURCES tests/s07a_remap_test.cpp    WIN32LIB/DRAWBUFF/REMAP.CPP    INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_scale_test    SOURCES tests/s07a_scale_test.cpp    WIN32LIB/DRAWBUFF/SCALE.CPP    INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_stamp_test    SOURCES tests/s07a_stamp_test.cpp    WIN32LIB/DRAWBUFF/STAMP.CPP    INCLUDES WIN32LIB/DRAWBUFF WIN32LIB/INCLUDE)
ra_add_test(NAME s07a_stmpcach_test SOURCES tests/s07a_stmpcach_test.cpp WIN32LIB/DRAWBUFF/STMPCACH.CPP INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_tobuff_test   SOURCES tests/s07a_tobuff_test.cpp   WIN32LIB/DRAWBUFF/TOBUFF.CPP   INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_topage_test   SOURCES tests/s07a_topage_test.cpp   WIN32LIB/DRAWBUFF/TOPAGE.CPP   INCLUDES WIN32LIB/DRAWBUFF)
ra_add_test(NAME s07a_cliprect_test SOURCES tests/s07a_cliprect_test.cpp WIN32LIB/MISC/CLIPRECT.CPP)
# --- S07-B ---
# --- S07-C ---
# --- S07-D ---
# --- S08 ---
# --- S09 ---
