/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer public API (session S08). See docs/codebase/platform-layer.md.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This is the ONLY header the game and WIN32LIB include to reach the window,
**	the screen, the keyboard/mouse and the timer (ADR-0004: game code never calls
**	SDL directly). It contains no SDL types and no Windows types.
**
**	Packing rule: the game is compiled with 1-byte struct packing (/Zp1,
**	-fpack-struct=1) and this library is not. Every struct in this header is made
**	only of 32-bit integers so its layout is identical either way (checked by the
**	static_asserts at the bottom). Do not add pointers, 64-bit or smaller fields.
*/
#ifndef RA_PLATFORM_H
#define RA_PLATFORM_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
** Lifetime
** ------------------------------------------------------------------------- */

/* Initialise the platform layer (timer base, event system). Safe to call twice.
** Returns false if SDL could not be initialised. */
bool Platform_Init(void);

/* Close the window, stop everything. Safe to call when not initialised. */
void Platform_Shutdown(void);

/* Last error text from the platform layer (never NULL). */
const char *Platform_Last_Error(void);

/* ---------------------------------------------------------------------------
** Video: one window showing one 8-bit paletted frame buffer.
**
** The game keeps drawing into its own 8-bit buffers (GraphicBufferClass). The
** buffer that used to be the DirectDraw primary surface is registered here with
** Platform_Set_Frame_Buffer; every present converts it through the palette and
** shows it scaled in the window. Presents happen when the game calls
** Platform_Present, and automatically from Platform_Pump_Events at most once per
** Platform_Set_Present_Interval milliseconds (the game writes to the visible
** page directly and never says "flip").
** ------------------------------------------------------------------------- */

enum PlatformScaleMode {
	PLATFORM_SCALE_INTEGER = 0,	/* largest whole multiple that fits; black borders */
	PLATFORM_SCALE_FIT     = 1,	/* largest size that keeps the aspect ratio; black bars */
	PLATFORM_SCALE_STRETCH = 2		/* fill the whole window */
};

struct PlatformVideoConfig {
	int32_t Width;					/* game resolution: 640 x 400 (or 640 x 480 with Resolution=1) */
	int32_t Height;
	int32_t WindowScale;			/* initial window size = game size * this (0 = pick from desktop) */
	int32_t Fullscreen;			/* non-zero: start in desktop fullscreen */
	int32_t ScaleMode;			/* PlatformScaleMode */
	int32_t AspectCorrect;		/* non-zero: show 640x400 at 4:3 like a 1996 CRT (pixels 1.2x tall) */
};

/* Fill in the defaults (640x400, window scale 0, windowed, FIT, aspect correction on). */
void Platform_Default_Video_Config(PlatformVideoConfig *config);

/* Create (or re-create with a new size) the window and renderer. `title` may be NULL. */
bool Platform_Video_Init(const PlatformVideoConfig *config, const char *title);
void Platform_Video_Shutdown(void);
bool Platform_Video_Is_Open(void);

/* Register the visible 8-bit frame buffer (`pitch` = bytes per row). Pass NULL to
** unregister. The memory stays owned by the caller and must outlive the
** registration. */
void Platform_Set_Frame_Buffer(const unsigned char *pixels, int width, int height, int pitch);

/* Present the registered frame buffer now. */
void Platform_Present(void);

/* Stand-in for waiting for the vertical blank (WIN32LIB Wait_Vert_Blank): present
** the frame now, then sleep until the next 1/60 s boundary of the platform clock,
** so loops that paced themselves on the 60/70 Hz retrace (palette fades) keep
** their speed. */
void Platform_Wait_Vertical_Blank(void);

/* Auto-present interval used by Platform_Pump_Events, in milliseconds
** (default 16; 0 disables auto-present). */
void Platform_Set_Present_Interval(int milliseconds);

/* Palette: 256 RGB triplets. `bits` is 6 for VGA/DirectDraw-style 0..63 values
** (expanded with << 2, exactly like WIN32LIB Set_DD_Palette), or 8 for 0..255. */
void Platform_Set_Palette(const unsigned char *rgb768, int bits);
/* Copy the current palette out as 8-bit values (768 bytes). */
void Platform_Get_Palette(unsigned char *rgb768);

void Platform_Set_Fullscreen(bool fullscreen);
bool Platform_Is_Fullscreen(void);
void Platform_Toggle_Fullscreen(void);
void Platform_Set_Scale_Mode(int scale_mode, bool aspect_correct);

/* Show or hide the operating-system cursor over the window (the game draws its own). */
void Platform_Show_System_Cursor(bool show);
/* Keep the OS cursor inside the window (the original clipped it to the screen). */
void Platform_Set_Mouse_Clip(bool clip);

/* Simple blocking message box (replaces Win32 MessageBox for errors). */
void Platform_Message_Box(const char *title, const char *text);

/* ---------------------------------------------------------------------------
** Input. Events are translated to the game's own key codes: Windows virtual-key
** codes (VK_*, CODE/KEY.H) plus the WWKEY_* modifier bits, exactly as
** WWKeyboardClass::Put_Key_Message built them. Mouse buttons are VK_LBUTTON,
** VK_RBUTTON, VK_MBUTTON with the position in game (frame-buffer) coordinates.
** ------------------------------------------------------------------------- */

enum {
	PLATFORM_WWKEY_SHIFT_BIT = 0x100,	/* same values as WWKey_Type in CODE/KEY.H */
	PLATFORM_WWKEY_CTRL_BIT  = 0x200,
	PLATFORM_WWKEY_ALT_BIT   = 0x400,
	PLATFORM_WWKEY_RLS_BIT   = 0x800
};

enum PlatformInputType {
	PLATFORM_INPUT_KEY   = 1,	/* Key = full WW key code (VK | WWKEY bits) */
	PLATFORM_INPUT_MOUSE = 2	/* Key = VK_xBUTTON (| WWKEY_RLS_BIT); X, Y = game coordinates */
};

struct PlatformInputEvent {
	int32_t  Type;		/* PlatformInputType */
	uint32_t Key;
	int32_t  X;
	int32_t  Y;
};

/* Process all pending OS events: translate input into the queue, track focus and
** quit requests, run the registered poll hooks, and auto-present. Call it where
** the original pumped Windows messages (WWKeyboardClass::Fill_Buffer_From_System).
** Returns the number of input events waiting in the queue. */
int Platform_Pump_Events(void);

/* Pop the oldest translated input event. Returns false when the queue is empty. */
bool Platform_Get_Input(PlatformInputEvent *event);
/* Look at the oldest event without removing it. */
bool Platform_Peek_Input(PlatformInputEvent *event);
/* Throw away all queued input events. */
void Platform_Clear_Input(void);

/* Current mouse position in game coordinates (clamped to the frame buffer). */
void Platform_Get_Mouse_Position(int *x, int *y);

/* Is this key (VK code, low 8 bits used; VK_LBUTTON etc. for buttons) held down now?
** Replaces GetAsyncKeyState/GetKeyState. */
bool Platform_Key_Down(unsigned vk);

/* WWKEY_SHIFT/CTRL/ALT bits for the modifier keys held now. */
unsigned Platform_Modifier_Bits(void);

/* Translate a WW key code to ASCII like Win32 ToAscii did with a US keyboard
** layout (shift/ctrl from the key's own bits; caps lock ignored like the original).
** Returns 0 for released keys and keys without a character. */
char Platform_Key_To_ASCII(unsigned wwkey);

bool Platform_Has_Focus(void);
/* The user closed the window (or the OS asked the program to quit). */
bool Platform_Quit_Requested(void);

/* Called once whenever the window gains (true) or loses (false) input focus.
** S10 connects this to what WM_ACTIVATEAPP did (GameInFocus, Focus_Loss/Restore). */
typedef void (*PlatformFocusCallback)(bool in_focus);
void Platform_Set_Focus_Callback(PlatformFocusCallback callback);

/* ---------------------------------------------------------------------------
** Poll hooks: functions run on every Platform_Pump_Events, after input has been
** translated and before the auto-present. The network layer (S12) registers its
** socket Poll() here; WWMouseClass registers the cursor update. At most
** PLATFORM_MAX_POLL_HOOKS hooks; the same (function, user) pair is only added once.
** ------------------------------------------------------------------------- */

enum { PLATFORM_MAX_POLL_HOOKS = 16 };
typedef void (*PlatformPollHook)(void *user);
bool Platform_Add_Poll_Hook(PlatformPollHook hook, void *user);
bool Platform_Remove_Poll_Hook(PlatformPollHook hook, void *user);

/* ---------------------------------------------------------------------------
** Time. std::chrono steady clock; works without Platform_Init and without SDL.
** ------------------------------------------------------------------------- */

/* Milliseconds since the first call into the timer (monotonic). The 32-bit form
** wraps after 49 days; use the 64-bit one for long-running tick counts. */
uint32_t Platform_Milliseconds(void);
uint64_t Platform_Milliseconds64(void);

/* Sleep the calling thread. */
void Platform_Sleep(int milliseconds);

/* Tick period used for a timer of `frequency` ticks per second, in whole
** milliseconds, exactly as WIN32LIB's WinTimerClass asked timeSetEvent for:
** 1000 / frequency, integer division (60 Hz -> 16 ms -> 62.5 ticks per second). */
uint32_t Platform_Tick_Period_Ms(uint32_t frequency);

/* Number of whole periods in `elapsed_ms` (the tick count of a timer started
** `elapsed_ms` ago). */
uint32_t Platform_Ticks_In(uint64_t elapsed_ms, uint32_t frequency);

#ifdef __cplusplus
static_assert(sizeof(PlatformVideoConfig) == 24, "PlatformVideoConfig must not depend on packing");
static_assert(sizeof(PlatformInputEvent) == 16, "PlatformInputEvent must not depend on packing");
#endif

#endif /* RA_PLATFORM_H */
