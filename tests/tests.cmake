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
# --- S07-B ---
ra_add_test(NAME s07b_shape_renderer_test
            SOURCES tests/s07b_shape_renderer_test.cpp CODE/SHAPEDRW.CPP
            INCLUDES CODE)
ra_add_test(NAME s07b_shape_buffer_test
            SOURCES tests/s07b_shape_buffer_test.cpp WIN32LIB/SHAPE/SHAPE.CPP)
# --- S07-C ---
ra_add_test(NAME s07c_text_test
            SOURCES tests/s07c_text_test.cpp CODE/2TXTPRNT.CPP WIN32LIB/FONT/SETFPAL.CPP)
ra_add_test(NAME s07c_palette_test
            SOURCES tests/s07c_palette_test.cpp WIN32LIB/PALETTE/PAL.CPP WIN32LIB/MISC/FADING.CPP)
ra_add_test(NAME s07c_mouse_test
            SOURCES tests/s07c_mouse_test.cpp WIN32LIB/KEYBOARD/WWMOUSE.CPP)
ra_add_test(NAME s07c_interp_test
            SOURCES tests/s07c_interp_test.cpp CODE/WINASM.CPP)
# --- S07-D ---
# s07d_asm_oracle.cpp holds the original asm (GNU syntax) as a reference; it is empty except on 32-bit x86 GCC/Clang.
ra_add_test(NAME s07d_lcw_test
            SOURCES tests/s07d_lcw_test.cpp tests/s07d_asm_oracle.cpp CODE/LCW.CPP CODE/LCWCOMP.CPP
            INCLUDES CODE)
ra_add_test(NAME s07d_xordelta_test
            SOURCES tests/s07d_xordelta_test.cpp tests/s07d_asm_oracle.cpp WIN32LIB/WSA/XORDELTA.CPP)
ra_add_test(NAME s07d_misc_test
            SOURCES tests/s07d_misc_test.cpp tests/s07d_asm_oracle.cpp
                    WIN32LIB/MEM/MEM_COPY.CPP WIN32LIB/MISC/REVERSE.CPP WIN32LIB/MISC/RANDOM.CPP)
# --- S08 ---
# --- S09 ---
