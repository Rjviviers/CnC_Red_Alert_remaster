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
