# Platform layer (session S08): the raplatform library and the platform_demo program.
# Included from the root CMakeLists.txt after cmake/RASDL3.cmake.
# See docs/codebase/platform-layer.md.
#
# raplatform is NEW code and must always build: it is a normal target (not in
# PROGRESS_TARGETS), so `cmake --build` and CI fail when it breaks.
#
# Deliberately NOT built with ra_apply_build_settings(): no 1-byte struct packing
# (SDL's headers and ABI need normal packing) and no WIN32 defines. platform.h is
# written so its structs have the same layout with and without packing.

set(RA_PLATFORM_SOURCES
  platform/platform.h
  platform/src/keymap.h
  platform/src/keymap.cpp
  platform/src/viewport.h
  platform/src/viewport.cpp
  platform/src/platform_timer.cpp
  platform/src/platform_sdl.cpp)
list(TRANSFORM RA_PLATFORM_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

add_library(raplatform STATIC ${RA_PLATFORM_SOURCES})
target_include_directories(raplatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform)
target_link_libraries(raplatform PRIVATE SDL3::SDL3)
set_target_properties(raplatform PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
if(MSVC)
  target_compile_options(raplatform PRIVATE /W4)
else()
  target_compile_options(raplatform PRIVATE -Wall -Wextra)
endif()

# Test program: test pattern, palette cycling, input echo (docs/codebase/platform-layer.md).
add_executable(platform_demo ${CMAKE_CURRENT_SOURCE_DIR}/platform/demo/platform_demo.cpp)
target_link_libraries(platform_demo PRIVATE raplatform)
set_target_properties(platform_demo PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
