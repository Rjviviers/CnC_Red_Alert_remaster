# =============================================================================
#  SDL3 dependency (session S08, ADR-0004 accepted, ADR-0020 proposed)
#
#  SDL3 is built from source with FetchContent, from the official release
#  tarball pinned by version AND SHA-256, as a static library. The result is
#  the imported-style target SDL3::SDL3 (an alias of SDL3::SDL3-static here).
#
#  Why FetchContent and not vcpkg: see docs/decisions/0020-sdl3-via-fetchcontent.md.
#
#  Options:
#    RA_SDL3_FROM_SYSTEM   OFF  Use find_package(SDL3) instead of building it. The
#                               system SDL3 must match the target architecture
#                               (32-bit for RA_TARGET_ARCH=x86).
#    FETCHCONTENT_SOURCE_DIR_SDL3 (standard CMake) - point at an unpacked SDL
#                               source tree to build offline.
#    RA_SDL3_URL           the pinned release tarball (change together with the hash)
#
#  Linux note: SDL picks its video back ends (X11, Wayland, KMSDRM, ...) from the
#  development headers it finds. For a 32-bit build the X11/Wayland libraries are
#  loaded at run time (dlopen), so only headers are needed at build time; if none
#  are found SDL still builds with its "offscreen" and "dummy" drivers, which is
#  what CI uses (docs/building.md, "SDL3").
# =============================================================================

set(RA_SDL3_VERSION "3.4.16")
set(RA_SDL3_URL
    "https://github.com/libsdl-org/SDL/releases/download/release-${RA_SDL3_VERSION}/SDL3-${RA_SDL3_VERSION}.tar.gz"
    CACHE STRING "SDL3 release tarball (pinned; change RA_SDL3_SHA256 with it)")
set(RA_SDL3_SHA256 "7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68"
    CACHE STRING "SHA-256 of RA_SDL3_URL")
option(RA_SDL3_FROM_SYSTEM "Use an installed SDL3 (find_package) instead of building the pinned release" OFF)

if(RA_SDL3_FROM_SYSTEM)
  find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)
  message(STATUS "SDL3: using installed SDL3 ${SDL3_VERSION}")
  return()
endif()

include(FetchContent)

# SDL's own build options. Set as cache variables before SDL's CMakeLists runs.
# Static only: one self-contained executable, no SDL3.dll to copy next to the
# tests and the demo on Windows.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
# Subsystems the game will never use. Video, render, events and audio (S09) stay on.
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
# X11 extensions the game does not need. Once SDL finds X11 it insists on the
# development files of every enabled extension, so fewer means fewer 32-bit
# packages to install (CI installs libx11/xext/xrandr/xcursor/xi/xfixes -dev:i386).
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
set(SDL_X11_XSHAPE OFF CACHE BOOL "" FORCE)
set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
set(SDL_X11_XDBE OFF CACHE BOOL "" FORCE)
set(SDL_X11_XSYNC OFF CACHE BOOL "" FORCE)
# SDL's own sources are not ours: never warnings-as-errors.
set(SDL_WERROR OFF CACHE BOOL "" FORCE)

# Linux without (32-bit) X11/Wayland development files: SDL refuses to configure
# unless told that a window-less build is intended. RA_SDL3_HEADLESS=ON builds SDL
# with only its "offscreen" and "dummy" video drivers: everything links and the
# tests and platform_demo run (SDL_VIDEO_DRIVER=offscreen), but no real window opens.
option(RA_SDL3_HEADLESS "Linux: allow building SDL3 without X11/Wayland (offscreen/dummy video only)" OFF)
if(RA_SDL3_HEADLESS)
  set(SDL_UNIX_CONSOLE_BUILD ON CACHE BOOL "" FORCE)
  set(SDL_X11 OFF CACHE BOOL "" FORCE)
  set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
  cmake_policy(SET CMP0135 NEW)   # extracted files get the extraction time stamp
endif()

FetchContent_Declare(SDL3
  URL      "${RA_SDL3_URL}"
  URL_HASH SHA256=${RA_SDL3_SHA256})
FetchContent_MakeAvailable(SDL3)

if(NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 ALIAS SDL3-static)
endif()

message(STATUS "SDL3: building pinned release ${RA_SDL3_VERSION} (static) from ${RA_SDL3_URL}")
