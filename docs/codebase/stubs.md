# Stubs and Win32 replacements

Every function the game links that does **not** do what the original did, why, and which session removes it. Made in
session S10b ([log](../logs/2026-10-15-S10b-link-platform.md), [ADR-0022](../decisions/0022-portable-file-io-and-win32-replacements.md)).
When you replace a stub, delete it here and in the code in the same commit.

Each stub file starts with a comment saying the same thing as its row below.

## Stubs (`CODE/stubs/`)

| File | Hosts | Symbols | Behaviour | Why | Removed by |
|------|-------|---------|-----------|-----|------------|
| `movie_stubs.cpp` | all | `VQA_Alloc` (returns NULL), `VQA_Open` (`VQAERR_OPEN`), `VQA_Play` (`VQAERR_EOF`), `VQA_Init`, `VQA_Close`, `VQA_Free`, `VQA_PauseAudio`, `VQA_ResumeAudio`, `VQA_DefaultConfig` (zeroes the config) | Every movie is skipped at once, as if it had finished (`Play_Movie`, `Open_Movie`, the intro, briefings) | Movies are out of scope ([ADR-0005](../decisions/0005-multiplayer-first-scope.md)). The VQA player library (`vqa32wp`) is no longer linked: its decoder (`UnVQ_4x2`, asm), audio codec (`VQA_sosCODEC*`, `AudioUnzap`: objects missing from the release), DirectSound, multimedia timers, `MCGA_*`/`WaitVB` and `Mono_*` debug output are all gone with it | Permanent, unless a later ADR brings movies back |
| `winsock_stubs.cpp` | non-Windows | `WSAStartup` (fails with `WSASYSNOTREADY`), `WSACleanup`, `WSAGetLastError`, `WSAAsyncSelect` (`SOCKET_ERROR`), `WSACancelAsyncRequest`; `closesocket` is real (`close`) | The game reports that the network is not available. **LAN does not work until S12** | The old transport (`WSPROTO.CPP`, `WSPUDP.CPP`) is driven by `WSAAsyncSelect` window messages, and there is no Win32 window | **S12** (polled UDP transport) |
| `win32_stubs.cpp` | non-Windows | `RegOpenKeyEx` (`ERROR_FILE_NOT_FOUND`), `RegQueryValueEx`, `RegCloseKey` | Every registry key is missing: `Is_Counterstrike_Installed`, `Is_Aftermath_Installed` (`CONQUER.CPP`) and `Is_DVD_Installed` (`INIT.CPP`) return false | No installer, no registry | **S10c** replaces the expansion checks with MIX-file detection ([ADR-0011](../decisions/0011-expansions-in-multiplayer.md)) |
| | | `GetVolumeInformation` (fails, `ERROR_PATH_NOT_FOUND`) | `Get_CD_Index` never finds a CD | No drive letters | Permanent (CD checks die when S10c adds the data-folder setting) |
| | | `SendMessage` (no-op) | `WSPROTO.CPP`/`WSPUDP.CPP` FD_WRITE kicks do nothing | No window | **S12** |
| | | `ShowCursor` (no-op) | | The platform layer hides the OS cursor over the window | Permanent |
| `asm_stubs.cpp` | all | `CPUType` (0), `VendorID` (one NUL), `Detect_MMX_Availability` (0) | `Get_CPU_Type` would report "no CPUID" | `CPUID.ASM` was not ported (S07): `Get_CPU_Type` has no callers | Permanent |
| | | `Force_VM_Page_In` (no-op) | | Windows 95 paging hint before the `EGOS.CPP` slide show (`VMPAGEIN.ASM`) | Permanent |

## Win32 API on non-Windows hosts (`CODE/compat/win32_api.cpp`)

Real, portable implementations of exactly what each caller needs (not general emulations). Compiled to nothing on Windows,
where the real API is linked (`ws2_32 winmm advapi32 user32`, `CMakeLists.txt`).

| Symbols | Caller | Implementation |
|---------|--------|----------------|
| `FindFirstFile`, `FindNextFile`, `FindClose` | `SESSION.CPP`: multiplayer map packets `*.PKT`, `*.MPR` | `opendir` + `fnmatch(FNM_CASEFOLD)`, results **sorted case-insensitively** (NTFS order; readdir order is arbitrary and this list orders the map list). No 8.3 alias (`cAlternateFileName` empty; the caller falls back to `cFileName`). `SESSION.CPP` never calls `FindClose`, so each scan leaks its small state block, as on Windows |
| `CreateFile` (`GENERIC_READ`, `OPEN_EXISTING` only), `CloseHandle` | `CONQUER.CPP` `Get_CD_Index` (unreachable: `GetVolumeInformation` fails first) | `RA_File_Open` |
| `GetLastError`, `SetLastError` | `CONQUER.CPP`, `WSPROTO.CPP` | A global set by these functions (Win32 codes) |
| `GetSystemTime` | `INIT.CPP`, `MENUS.CPP`: seed bits for `CryptRandom` | `gettimeofday` + `gmtime_r` |
| `GlobalMemoryStatus` | `2KEYFRAM.CPP` `Check_Use_Compressed_Shapes` (> 16 MB → big shape buffer) | `sysconf(_SC_PHYS_PAGES)`, capped below 2 GB |
| `GetDriveType` | `WIN32LIB/PLAYCD/GETCD.CPP` | Always `DRIVE_NO_ROOT_DIR`: no CD drives |
| `OutputDebugString` | `WSPROTO.CPP`, `WSPUDP.CPP` | `stderr` when `RA_DEBUG_OUTPUT` is set |

`RawFileClass` uses `CODE/compat/ra_file.h` on every host instead of the Win32 file API; see
[compat-notes.md, "S10b notes"](compat-notes.md#s10b-notes).

## Removed instead of stubbed

| Symbol(s) | Where | What happened |
|-----------|-------|---------------|
| `WinMain`, `GetModuleFileName`, `MessageBox`, `Sleep`, `PostMessage` | `STARTUP.CPP` | Portable `main` ([platform-layer.md](platform-layer.md#how-main-works)) |
| `RegisterClass`, `CreateWindowEx`, `LoadIcon`, `ShowWindow`, `UpdateWindow`, `SetFocus`, `GetSystemMetrics`, `RegisterWindowMessage`, `DefWindowProc`, `PeekMessage`, `GetMessage`, `TranslateMessage`, `DispatchMessage`, `PostQuitMessage`, `ExitProcess`, `DialogBox`, `Stop_Profiler` | `WINSTUB.CPP` | `Create_Main_Window`, `Windows_Procedure`, `Window_Dialog_Box`, `Colour_Debug` deleted; focus, quit and shutdown go through the platform layer |
| `DefWindowProc` | `KEY.CPP` | `WWKeyboardClass::Message_Handler` deleted (S08 replaced it with `Fill_Buffer_From_System`) |
| `CreateFile`, `ReadFile`, `WriteFile`, `SetFilePointer`, `GetFileSize`, `DeleteFile`, `GetFileInformationByHandle`, `FileTimeToDosDateTime`, `DosDateTimeToFileTime`, `SetFileTime`, `SetErrorMode` | `RAWFILE.CPP` | `ra_file.h` |
| `MpgPlay`, `MpgPause`, `MpgResume`, `MpgSetCallback` | `CONQUER.CPP` | `MPEGMOVIE` no longer defined (`DEFINES.H`); `MPGSET.CPP` compiles to nothing without it |
| `DirectSoundCreate`, `timeSetEvent`, `timeKillEvent`, `GetCurrentProcess`, `GetPriorityClass`, `SetPriorityClass`, `AudioUnzap`, `VQA_sosCODEC*`, `UnVQ_4x2`, `MCGA_Blit`, `MCGA_BufferCopy`, `WaitVB`, `WaitNoVB`, `Mono_*` | `WINVQ/VQA32/*` | Only referenced by the VQA player, which is no longer linked |
