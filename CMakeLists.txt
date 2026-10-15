# Command & Conquer: Red Alert - multiplayer port
# Root build file (session S03). See docs/building.md for how to use it.
#
# Targets:
#   redalert   the game (CODE/), output name ra95 like the original
#   win32lib   Westwood Win32 support library (WIN32LIB/)
#   vqa32wp    VQA movie player library (WINVQ/VQA32)
#   vqm32wp    VQ misc library (WINVQ/VQM32)
#   compile-progress   compile every source on its own and write build/compile-progress.md
#
# The code does not compile yet (it was written for Watcom C++ 10.6). Building
# the targets normally is expected to fail until sessions S06 (compiler
# compatibility) and S07 (assembly -> C++) are done. Use compile-progress.

cmake_minimum_required(VERSION 3.20)

# Architecture/compiler settings live in ONE place (ADR-0003, accepted).
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/RABuildConfig.cmake)   # stage 1: before project()

project(RedAlert LANGUAGES C CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/RABuildConfig.cmake)   # stage 2: after project()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # the compile-progress tool reads compile_commands.json

# Hand-maintained source lists (ADR-0012; originally generated from the makefiles).
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/RASources.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RASources.cmake)

if(CMAKE_HOST_WIN32)
  find_package(Python3 COMPONENTS Interpreter QUIET)
else()
  # Needed at configure time for the case-insensitive include shim.
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Platform dependency (SDL3) and the platform layer (session S08).
# ADR-0004: SDL3 behind a thin platform/ layer; ADR-0020: SDL3 is built from the
# pinned release tarball with FetchContent (cmake/RASDL3.cmake).
#   SDL3::SDL3     static SDL3 (also used by audio, S09)
#   raplatform     platform/ library (window, palette, input, timer, poll hooks)
#   platform_demo  test program for the platform layer
# -----------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/RASDL3.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/platform/platform.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/RACaseShim.cmake)

# Shared include directories, in the order of the original /i= options.
set(RA_WIN32LIB_INCLUDE WIN32LIB/INCLUDE)
set(RA_WINVQ_INCLUDE WINVQ/INCLUDE)

# Helper: add sources (repo-relative paths) to a target. Every file is compiled
# as C++ (Watcom's makefile used wpp386 for .c files too).
function(ra_target_sources target)
  set(abs_sources "")
  foreach(src IN LISTS ARGN)
    list(APPEND abs_sources ${CMAKE_CURRENT_SOURCE_DIR}/${src})
  endforeach()
  target_sources(${target} PRIVATE ${abs_sources})
  set_source_files_properties(${abs_sources} PROPERTIES LANGUAGE CXX)
  ra_case_shim_sources(${ARGN})
endfunction()

# Helper: add repo-relative include directories (plus their case shims).
function(ra_target_includes target)
  foreach(dir IN LISTS ARGN)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
    ra_case_shim_include(${target} ${dir})
  endforeach()
  ra_case_shim_system(${target})
endfunction()

# -----------------------------------------------------------------------------
# win32lib - WIN32LIB/MAKEFILE: 18 component libraries merged into win32lib.lib
# -----------------------------------------------------------------------------
add_library(win32lib STATIC)
ra_target_sources(win32lib ${RA_WIN32LIB_CXX_SOURCES})
ra_target_includes(win32lib ${RA_WIN32LIB_INCLUDE})   # PROJECT.CFG: /i=$(WIN32LIB)\include
ra_apply_build_settings(win32lib)
target_link_libraries(win32lib PUBLIC raplatform)     # S08: video/input/timer back end (platform/platform.h)

# SDL3 audio backend (S09, WIN32LIB/AUDIO/audio_sdl3.cpp). ON since S08 added SDL3 and S06a
# stopped defining _WIN32 off Windows. OFF selects the null backend only.
option(RA_HAVE_SDL3 "Build the SDL3 audio backend (requires the SDL3::SDL3 target)" ON)
if(RA_HAVE_SDL3)
  target_compile_definitions(win32lib PUBLIC RA_HAVE_SDL3=1)
  target_link_libraries(win32lib PUBLIC SDL3::SDL3)
endif()

# -----------------------------------------------------------------------------
# vqa32wp / vqm32wp - WINVQ/VQA32/MAKEFILE and WINVQ/VQM32/MAKEFILE
# -----------------------------------------------------------------------------
add_library(vqa32wp STATIC)
ra_target_sources(vqa32wp ${RA_VQA32_CXX_SOURCES})
ra_target_includes(vqa32wp ${RA_WINVQ_INCLUDE})       # INCLUDEPATH = $(PRJ)\INCLUDE
ra_apply_build_settings(vqa32wp)

add_library(vqm32wp STATIC)
ra_target_sources(vqm32wp ${RA_VQM32_CXX_SOURCES})
ra_target_includes(vqm32wp ${RA_WINVQ_INCLUDE})
ra_apply_build_settings(vqm32wp)

# -----------------------------------------------------------------------------
# redalert - CODE/MAKEFILE (Win32: ra95.exe). OBJECTS + jshell.lib + tech.lib.
# -----------------------------------------------------------------------------
add_executable(redalert)
set_target_properties(redalert PROPERTIES OUTPUT_NAME ra95 WIN32_EXECUTABLE TRUE)
ra_target_sources(redalert ${RA_GAME_CXX_SOURCES})
ra_target_includes(redalert ${RA_WIN32LIB_INCLUDE} ${RA_WINVQ_INCLUDE})
ra_apply_build_settings(redalert)
target_compile_definitions(redalert PRIVATE ${RA_GAME_DEFINES})
# S10b (ADR-0022): vqa32wp (the VQA movie player) is not linked: movies are stubbed
# (ADR-0005, CODE/stubs/movie_stubs.cpp). It still builds as its own target.
target_link_libraries(redalert PRIVATE win32lib vqm32wp)
if(WIN32)
  # The Win32 API the game still calls on Windows (registry, Winsock until S12, winmm).
  target_link_libraries(redalert PRIVATE ws2_32 winmm advapi32 user32)
  if(MSVC)
    # WIN32_EXECUTABLE (no console) normally wants WinMain; the game has a portable main.
    target_link_options(redalert PRIVATE /ENTRY:mainCRTStartup)
  endif()
endif()
# The original also linked ipx\wwipx32.lib (native IPX, removed in S05), mpgdll.lib,
# DirectX (ddraw, dsound, dxguid), DirectShow (amstrmid, strmbasd) and uuid.

# Assembly sources are listed (RA_*_ASM_SOURCES) but not compiled: they are
# TASM syntax and are being rewritten in C++ in session S07.

# -----------------------------------------------------------------------------
# Unit tests (tests/tests.cmake). Each test is a small executable; run with ctest.
# -----------------------------------------------------------------------------
option(RA_BUILD_TESTS "Build the unit tests" ON)
if(RA_BUILD_TESTS)
  enable_testing()
  include(${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.cmake)
endif()

# -----------------------------------------------------------------------------
# compile-progress: compile each translation unit independently, keep going on
# errors, write ${CMAKE_BINARY_DIR}/compile-progress.md (+ .json).
# -----------------------------------------------------------------------------
if(Python3_Interpreter_FOUND)
  add_custom_target(compile-progress
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_progress.py
            --build-dir ${CMAKE_BINARY_DIR} --no-configure
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Compiling every source file independently (compile-progress report)")
endif()

message(STATUS "Red Alert: arch=${RA_TARGET_ARCH} (pointer size ${CMAKE_SIZEOF_VOID_P}), "
               "C++${RA_CXX_STANDARD}, language=${RA_LANGUAGE}, pack-structs=${RA_PACK_STRUCTS}")
