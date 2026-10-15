# Boot smoke test (session S10c): run the real game executable headless, without game data,
# and check that it gets as far as it can and exits cleanly with a clear message (no crash).
#
#   cmake -DRA95=<path to ra95> -DWORK=<scratch dir> -P tests/s10c_boot_smoke.cmake
#
# Case 1, empty data folder: the start-up data check (CODE/STARTUP.CPP Boot_Setup_Data) must
#   report the missing REDALERT.MIX and MAIN.MIX, name the folder it searched, create
#   REDALERT.INI next to the executable, and exit with code 1.
# Case 2, empty (0-byte) REDALERT.MIX and MAIN.MIX: the start-up check passes, the game opens
#   its (offscreen) window, initialises timer, audio, keyboard and mouse, and starts loading
#   (INIT.CPP Init_Bootstrap_Mixfiles). It must reject the damaged files with a message and
#   exit with code 1. These placeholders are NOT game data; they exercise the error path only.
#
# The executable is copied into the scratch folder so REDALERT.INI is created there.

if(NOT RA95 OR NOT WORK)
  message(FATAL_ERROR "usage: cmake -DRA95=<ra95> -DWORK=<dir> -P s10c_boot_smoke.cmake")
endif()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/game" "${WORK}/empty-data" "${WORK}/bad-data")
get_filename_component(exe_name "${RA95}" NAME)
file(COPY "${RA95}" DESTINATION "${WORK}/game")
set(exe "${WORK}/game/${exe_name}")
file(WRITE "${WORK}/bad-data/redalert.mix" "")
file(WRITE "${WORK}/bad-data/MAIN.MIX" "")

function(run_game data_dir out_rc out_text)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy
            "${exe}" "-data=${data_dir}"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 60)
  set(${out_rc} "${rc}" PARENT_SCOPE)
  set(${out_text} "${out}${err}" PARENT_SCOPE)
endfunction()

function(expect cond_text what text)
  string(FIND "${text}" "${cond_text}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "${what}: expected to find \"${cond_text}\" in the output:\n${text}")
  endif()
endfunction()

# --- Case 1: no data -------------------------------------------------------------------------
run_game("${WORK}/empty-data" rc text)
message(STATUS "case 1 (empty data folder): exit code ${rc}\n${text}")
if(NOT rc STREQUAL "1")
  message(FATAL_ERROR "case 1: expected exit code 1 (clean exit after the data message), got '${rc}'")
endif()
expect("The Red Alert game data was not found" "case 1" "${text}")
expect("REDALERT.MIX" "case 1" "${text}")
expect("MAIN.MIX" "case 1" "${text}")
expect("empty-data (data folder, from -data)" "case 1" "${text}")
if(NOT EXISTS "${WORK}/game/REDALERT.INI")
  message(FATAL_ERROR "case 1: REDALERT.INI was not created next to the executable")
endif()

# --- Case 2: damaged data --------------------------------------------------------------------
run_game("${WORK}/bad-data" rc text)
message(STATUS "case 2 (0-byte MIX files): exit code ${rc}\n${text}")
if(NOT rc STREQUAL "1")
  message(FATAL_ERROR "case 2: expected exit code 1 (clean exit after the data message), got '${rc}'")
endif()
expect("could not be read" "case 2" "${text}")

message(STATUS "boot smoke test passed")
