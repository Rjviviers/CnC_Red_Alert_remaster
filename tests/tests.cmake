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
# --- S06a ---
ra_add_test(NAME jshell_asm_test
            SOURCES tests/jshell_asm_test.cpp
            INCLUDES CODE/compat)
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
# Platform layer tests. Not ra_add_test(): raplatform is built without the game's
# settings (no 1-byte packing, no WIN32 defines; see platform/platform.cmake), and the
# tests include SDL's keycode headers, so they are built the same way.
foreach(ra_platform_test platform_keymap_test platform_viewport_test platform_timer_test)
  add_executable(${ra_platform_test} ${CMAKE_CURRENT_SOURCE_DIR}/tests/${ra_platform_test}.cpp)
  target_include_directories(${ra_platform_test} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests
                                                         ${CMAKE_CURRENT_SOURCE_DIR}/platform/src)
  target_link_libraries(${ra_platform_test} PRIVATE raplatform SDL3::SDL3)
  set_target_properties(${ra_platform_test} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
  add_test(NAME ${ra_platform_test} COMMAND ${ra_platform_test})
  add_dependencies(ra_tests ${ra_platform_test})
endforeach()
# platform_demo end to end (window, palette, pump, present) on SDL's offscreen video driver,
# so it runs on CI machines without a display.
add_test(NAME platform_demo_offscreen COMMAND platform_demo --frames 120)
set_tests_properties(platform_demo_offscreen PROPERTIES ENVIRONMENT "SDL_VIDEO_DRIVER=offscreen")
add_dependencies(ra_tests platform_demo)
# --- S09 ---
ra_add_test(NAME s09_decoder_test
            SOURCES tests/s09_decoder_test.cpp
                    WIN32LIB/AUDIO/SOSCODEC.CPP WIN32LIB/AUDIO/OLSOSDEC.CPP WIN32LIB/AUDIO/AUDUNCMP.CPP
            INCLUDES WIN32LIB/AUDIO CODE)
ra_add_test(NAME s09_mixer_test
            SOURCES tests/s09_mixer_test.cpp WIN32LIB/AUDIO/audio_mixer.cpp
            INCLUDES WIN32LIB/AUDIO)
ra_add_test(NAME s09_sound_api_test
            SOURCES tests/s09_sound_api_test.cpp
                    WIN32LIB/AUDIO/SOUNDIO.CPP WIN32LIB/AUDIO/SOUNDINT.CPP WIN32LIB/AUDIO/SOUNDLCK.CPP
                    WIN32LIB/AUDIO/SOSCODEC.CPP WIN32LIB/AUDIO/OLSOSDEC.CPP WIN32LIB/AUDIO/AUDUNCMP.CPP
                    WIN32LIB/AUDIO/audio_mixer.cpp WIN32LIB/AUDIO/audio_null.cpp
            INCLUDES WIN32LIB/AUDIO WIN32LIB/INCLUDE)
# --- S06b ---
# calcx/calcy from CODE/COORD.CPP (simulation code): C++ against the original instructions.
ra_add_test(NAME s06b_coord_test
            SOURCES tests/s06b_coord_test.cpp
            INCLUDES CODE/compat)
# --- S10b ---
# Portable file layer used by RawFileClass (CODE/compat/ra_file.cpp, ADR-0022).
ra_add_test(NAME s10b_file_test
            SOURCES tests/s10b_file_test.cpp CODE/compat/ra_file.cpp
            INCLUDES CODE/compat)
# --- S10c ---
# Start-up helpers: -data option, search list, required-file check, first-run REDALERT.INI (ADR-0023).
ra_add_test(NAME s10c_boot_test
            SOURCES tests/s10c_boot_test.cpp CODE/port/ra_boot.cpp CODE/compat/ra_file.cpp
            INCLUDES CODE/port CODE/compat)
# Boot smoke test: the real game, headless, without game data, must exit cleanly with the
# "data missing" message (tests/s10c_boot_smoke.cmake). Builds the game as part of ra_tests.
if(TARGET redalert)
  add_test(NAME s10c_boot_smoke
           COMMAND ${CMAKE_COMMAND} -DRA95=$<TARGET_FILE:redalert>
                   -DWORK=${CMAKE_CURRENT_BINARY_DIR}/s10c_boot_smoke
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/s10c_boot_smoke.cmake)
  set_tests_properties(s10c_boot_smoke PROPERTIES TIMEOUT 180)
  add_dependencies(ra_tests redalert)
endif()
