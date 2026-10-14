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
# --- S07-C ---
# --- S07-D ---
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
