# =============================================================================
#  RED ALERT BUILD CONFIGURATION
#
#  This is THE ONE PLACE for target architecture, language standard, defines
#  and compiler flags. ADR-0003 ("CMake, modern compilers, 32-bit x86 first")
#  is accepted; if it is ever superseded, change this file only.
#
#  Included twice from the root CMakeLists.txt:
#    1. before project()  -> sets architecture flags (they must be known before
#                            CMake probes the compiler)
#    2. after project()   -> ra_apply_build_settings(<target>) is then usable
#
#  Mapping from the original Watcom flags (CODE/MAKEFILE, WIN32LIB/PROJECT.CFG,
#  WINVQ/*/MAKEFILE) to what we do here:
#    /DWIN32=1 /D_WIN32 /D<LANG>=1  -> <LANG> kept; WIN32/_WIN32 only on Windows hosts (S06a)
#    /DWOLAPI_INTEGRATION /DWINSOCK_IPX  -> dropped in S05: the sources were collapsed
#                                          with unifdef as if they were defined
#    /zp1   pack structs on 1 byte  -> RA_PACK_STRUCTS (/Zp1, -fpack-struct=1)
#    /j     char is signed          -> -fsigned-char (MSVC char is already signed)
#    /5r /4 /otxan /oaeilrt /of+ /s /d0 /d1 /ri /bt=NT /mf  -> dropped (Watcom codegen/ABI options)
#    /fh /fhq (precompiled headers), /zq (quiet)               -> dropped
#    /we /w8 /W3  (warnings, warnings-as-errors)               -> dropped for now (S06 decides)
#    /i=..\watcom\h ..\dxsdk\inc ..\gcl510\H                   -> dropped: Watcom headers,
#          DirectX 5 SDK and Greenleaf are not in the repo. MSVC's Windows SDK
#          ships ddraw.h/dsound.h; Greenleaf (modem) code is removed in S05.
# =============================================================================

if(NOT RA_BUILD_CONFIG_STAGE2)
  # ---------------------------------------------------------------------------
  # Stage 1: before project()
  # ---------------------------------------------------------------------------
  set(RA_TARGET_ARCH "x86" CACHE STRING
      "Target architecture: x86 (32-bit, ADR-0003) or native (whatever the compiler defaults to)")
  set_property(CACHE RA_TARGET_ARCH PROPERTY STRINGS x86 native)

  # GCC/Clang: 32-bit is selected with -m32 (needs gcc-multilib/g++-multilib).
  # MSVC: 32-bit is selected by the environment (x86 Developer Command Prompt,
  # or `-A Win32` with the Visual Studio generator); we only verify it in stage 2.
  # The *_INIT variables only apply when a build directory is first configured:
  # changing RA_TARGET_ARCH later needs a fresh build directory.
  if(RA_TARGET_ARCH STREQUAL "x86" AND NOT CMAKE_HOST_WIN32)
    string(APPEND CMAKE_C_FLAGS_INIT " -m32")
    string(APPEND CMAKE_CXX_FLAGS_INIT " -m32")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -m32")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " -m32")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_INIT " -m32")
  endif()

  set(RA_BUILD_CONFIG_STAGE2 TRUE)
  return()
endif()

# -----------------------------------------------------------------------------
# Stage 2: after project()
# -----------------------------------------------------------------------------

# Verify the architecture actually took effect.
if(RA_TARGET_ARCH STREQUAL "x86" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  if(MSVC)
    message(FATAL_ERROR
      "RA_TARGET_ARCH=x86 but the compiler targets ${CMAKE_SIZEOF_VOID_P}-byte pointers.\n"
      "Use an x86 Developer Command Prompt (vcvarsall.bat x86) with -G Ninja, "
      "or -G \"Visual Studio 17 2022\" -A Win32. Or pass -DRA_TARGET_ARCH=native.")
  else()
    message(FATAL_ERROR
      "RA_TARGET_ARCH=x86 but the compiler targets ${CMAKE_SIZEOF_VOID_P}-byte pointers.\n"
      "Install 32-bit support (Debian/Ubuntu: sudo apt install gcc-multilib g++-multilib) "
      "and use a fresh build directory, or pass -DRA_TARGET_ARCH=native.")
  endif()
endif()

# Verify the C/C++ runtime headers exist for this architecture. Without
# gcc-multilib, `-m32` passes CMake's own checks but every <stdio.h> fails.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <stdio.h>\n#include <string.h>\nint main(){return 0;}"
                          RA_HAVE_RUNTIME_HEADERS)
if(NOT RA_HAVE_RUNTIME_HEADERS)
  message(FATAL_ERROR
    "Cannot compile a trivial program that includes <stdio.h> for RA_TARGET_ARCH=${RA_TARGET_ARCH}.\n"
    "For 32-bit builds on Debian/Ubuntu: sudo apt install gcc-multilib g++-multilib "
    "(then delete the build directory and configure again).")
endif()

# Language standard. ADR-0003 says "the most permissive modern standard setting
# that works". In S03, gnu++98, gnu++14 and gnu++17 gave identical results
# (missing headers dominate), so we use 14, the oldest standard MSVC supports.
# Revisit in S06 once files get further than their #includes.
set(RA_CXX_STANDARD "14" CACHE STRING "C++ standard for all targets (GNU extensions are enabled)")
set_property(CACHE RA_CXX_STANDARD PROPERTY STRINGS 98 11 14 17 20)

set(RA_LANGUAGE "ENGLISH" CACHE STRING "Game language define (CODE/MAKEFILE LANGUAGE)")
set_property(CACHE RA_LANGUAGE PROPERTY STRINGS ENGLISH GERMAN FRENCH)

option(RA_PACK_STRUCTS
  "Pack all structs on 1-byte boundaries, like Watcom /zp1. Matches the original memory layout \
(save games, network packets) but also applies to system headers; S06 decides the final policy." ON)

# Defines shared by every target. The original makefiles defined WIN32=1 and
# _WIN32 for every build. Since S05 no source tests WIN32, _WIN32,
# WINSOCK_IPX or WOLAPI_INTEGRATION (a few WIN32LIB/WINVQ component files
# #define WIN32 themselves). Both names tell system and third-party headers
# (glibc, SDL3) that the HOST is Windows, which breaks non-Windows builds (SDL3
# took its Windows path and wanted <process.h>). So since S06a they are only
# defined on Windows hosts, where the compilers predefine _WIN32 anyway
# (ADR-0014).
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(RA_PLATFORM_DEFINES WIN32=1 _WIN32)
else()
  set(RA_PLATFORM_DEFINES "")
endif()

# Compatibility layer (session S06a, ADR-0014, docs/codebase/compat-notes.md):
#   CODE/compat/ra_compat.h   force-included into every translation unit
#   CODE/compat/              also on the include path (watcom_aux.h)
#   CODE/compat/dos/          Watcom/DOS headers no current compiler has
#                             (dos.h, mem.h, bios.h, pharlap.h, ...): every host
#   CODE/compat/posix/        Microsoft C runtime headers glibc lacks (io.h,
#                             direct.h, conio.h, share.h, new.h, process.h):
#                             non-Windows hosts, searched after the system headers
#   CODE/compat/win32/        minimal Win32/DirectX/Winsock declarations
#                             (windows.h, ddraw.h, ...): non-Windows hosts only,
#                             until S08 replaces the Win32 code with SDL
set(RA_COMPAT_DIR ${CMAKE_CURRENT_LIST_DIR}/../CODE/compat)
get_filename_component(RA_COMPAT_DIR ${RA_COMPAT_DIR} ABSOLUTE)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(RA_COMPAT_HOST_HEADERS FALSE)
else()
  set(RA_COMPAT_HOST_HEADERS TRUE)
endif()

# Game-only defines, straight from CODE/MAKEFILE (!ifdef WIN32 branch).
# WOLAPI_INTEGRATION and WINSOCK_IPX were dropped in S05: no source tests them
# any more (see docs/codebase/removed-code.md).
set(RA_GAME_DEFINES ${RA_LANGUAGE}=1)

function(ra_apply_build_settings target)
  set_target_properties(${target} PROPERTIES
    CXX_STANDARD ${RA_CXX_STANDARD}
    CXX_STANDARD_REQUIRED OFF
    CXX_EXTENSIONS ON)
  target_compile_definitions(${target} PRIVATE ${RA_PLATFORM_DEFINES})

  # Compatibility layer (see the comment above RA_COMPAT_DIR).
  target_include_directories(${target} PRIVATE ${RA_COMPAT_DIR}/dos ${RA_COMPAT_DIR})
  if(RA_PACK_STRUCTS)
    target_compile_definitions(${target} PRIVATE RA_PACK_STRUCTS=1)
  endif()
  if(MSVC)
    target_compile_options(${target} PRIVATE "/FI${RA_COMPAT_DIR}/ra_compat.h")
  else()
    target_compile_options(${target} PRIVATE "SHELL:-include ${RA_COMPAT_DIR}/ra_compat.h")
  endif()
  if(RA_COMPAT_HOST_HEADERS)
    target_compile_options(${target} PRIVATE
      "-idirafter${RA_COMPAT_DIR}/posix"
      "-idirafter${RA_COMPAT_DIR}/win32")
  endif()

  if(MSVC)
    target_compile_options(${target} PRIVATE
      /permissive        # accept pre-standard code (opposite of /permissive-)
      /W3)
    # Silence "use the _s version" deprecation noise from the modern CRT.
    target_compile_definitions(${target} PRIVATE
      _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
    if(RA_PACK_STRUCTS)
      # Recent Windows SDKs refuse non-default packing (winnt.h C_ASSERT,
      # "Windows headers require the default packing option") unless told to
      # ignore it. Without this define every file that includes windows.h would
      # fail on that one assert and hide its real errors. This is exactly the
      # ABI hazard S06 must resolve (pack only our own structs via #pragma pack).
      target_compile_options(${target} PRIVATE /Zp1)
      target_compile_definitions(${target} PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -fsigned-char)                         # Watcom /j
    # No -fpermissive: it hid code that MSVC rejects (S10c). GCC must fail where MSVC fails.
    if(RA_PACK_STRUCTS)
      target_compile_options(${target} PRIVATE -fpack-struct=1)
    endif()
  endif()
endfunction()
