# Case-insensitive include shim (ADR-0008).
#
# The original sources were written on case-insensitive file systems: they
# write `#include "function.h"` for FUNCTION.H and `#include <vqm32\all.h>`
# with backslashes. On Linux that fails. Rather than renaming files, at
# configure time tools/make_case_shim.py creates ${CMAKE_BINARY_DIR}/case_shim/
# with one directory of symlinks per source/include directory, named exactly as
# the code spells them. This file wires those directories into the targets:
#
#   - each include directory D gets its shim right after it (-I D -I shim(D))
#   - each source file gets -iquote shim(own directory), so its quoted
#     includes find sibling headers first, like they did on DOS/Windows
#   - _system/ (forwarders such as <sys\stat.h> -> <sys/stat.h>) is added
#     with -idirafter, so it is searched last
#
# On Windows hosts the file system is case-insensitive and accepts backslashes,
# so the shim is disabled and these functions do nothing.

if(CMAKE_HOST_WIN32)
  set(_ra_shim_default OFF)
else()
  set(_ra_shim_default ON)
endif()
option(RA_CASE_SHIM "Generate case-insensitive include shims (needed on case-sensitive file systems)"
       ${_ra_shim_default})

set(RA_CASE_SHIM_DIR ${CMAKE_BINARY_DIR}/case_shim)

# Repo-relative directory -> shim directory (same naming as make_case_shim.py).
function(_ra_shim_path out rel_dir)
  string(REPLACE "/" "__" name "${rel_dir}")
  set(${out} ${RA_CASE_SHIM_DIR}/${name} PARENT_SCOPE)
endfunction()

if(RA_CASE_SHIM)
  # Every directory that contains sources or is an include directory.
  set(_ra_shim_dirs WIN32LIB/INCLUDE WINVQ/INCLUDE)
  foreach(src IN LISTS RA_GAME_CXX_SOURCES RA_WIN32LIB_CXX_SOURCES
                       RA_VQA32_CXX_SOURCES RA_VQM32_CXX_SOURCES)
    get_filename_component(_dir ${src} DIRECTORY)
    list(APPEND _ra_shim_dirs ${_dir})
  endforeach()
  list(REMOVE_DUPLICATES _ra_shim_dirs)

  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_case_shim.py
            --source-root ${CMAKE_CURRENT_SOURCE_DIR}
            --out ${RA_CASE_SHIM_DIR}
            --dirs ${_ra_shim_dirs}
            --scan CODE WIN32LIB WINVQ
    RESULT_VARIABLE _ra_shim_result
    OUTPUT_VARIABLE _ra_shim_output
    ERROR_VARIABLE _ra_shim_output
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _ra_shim_result EQUAL 0)
    message(FATAL_ERROR "Case shim generation failed:\n${_ra_shim_output}")
  endif()
  message(STATUS "${_ra_shim_output}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_case_shim.py)
endif()

# -iquote <shim of the source's own directory> on each source file.
function(ra_case_shim_sources)
  if(NOT RA_CASE_SHIM)
    return()
  endif()
  foreach(src IN LISTS ARGN)
    get_filename_component(dir ${src} DIRECTORY)
    _ra_shim_path(shim ${dir})
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                 APPEND PROPERTY COMPILE_OPTIONS "-iquote${shim}")
  endforeach()
endfunction()

# Shim of one include directory, placed right after the real directory.
function(ra_case_shim_include target rel_dir)
  if(NOT RA_CASE_SHIM)
    return()
  endif()
  _ra_shim_path(shim ${rel_dir})
  target_include_directories(${target} PRIVATE ${shim})
endfunction()

# System-header forwarders, searched last.
function(ra_case_shim_system target)
  if(NOT RA_CASE_SHIM)
    return()
  endif()
  target_compile_options(${target} PRIVATE "-idirafter${RA_CASE_SHIM_DIR}/_system")
endfunction()
