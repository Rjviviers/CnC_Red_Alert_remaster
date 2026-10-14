/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer: key translation (session S08). Pure functions, no SDL types,
**	so they can be unit-tested (tests/platform_keymap_test.cpp).
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#ifndef RA_PLATFORM_KEYMAP_H
#define RA_PLATFORM_KEYMAP_H

#include <stdint.h>

namespace ra_platform {

/* Windows virtual-key codes the translation produces (same values as CODE/KEY.H). */
enum {
	VK_LBUTTON_ = 0x01, VK_RBUTTON_ = 0x02, VK_MBUTTON_ = 0x04,
	VK_BACK_ = 0x08, VK_TAB_ = 0x09, VK_CLEAR_ = 0x0C, VK_RETURN_ = 0x0D,
	VK_SHIFT_ = 0x10, VK_CONTROL_ = 0x11, VK_MENU_ = 0x12, VK_PAUSE_ = 0x13, VK_CAPITAL_ = 0x14,
	VK_ESCAPE_ = 0x1B, VK_SPACE_ = 0x20,
	VK_PRIOR_ = 0x21, VK_NEXT_ = 0x22, VK_END_ = 0x23, VK_HOME_ = 0x24,
	VK_LEFT_ = 0x25, VK_UP_ = 0x26, VK_RIGHT_ = 0x27, VK_DOWN_ = 0x28,
	VK_SNAPSHOT_ = 0x2C, VK_INSERT_ = 0x2D, VK_DELETE_ = 0x2E,
	VK_NUMPAD0_ = 0x60, VK_MULTIPLY_ = 0x6A, VK_ADD_ = 0x6B, VK_SUBTRACT_ = 0x6D,
	VK_DECIMAL_ = 0x6E, VK_DIVIDE_ = 0x6F, VK_F1_ = 0x70,
	VK_NUMLOCK_ = 0x90, VK_SCROLL_ = 0x91,
	VK_OEM_1_ = 0xBA,       /* ;: */
	VK_OEM_PLUS_ = 0xBB,    /* =+ */
	VK_OEM_COMMA_ = 0xBC,   /* ,< */
	VK_OEM_MINUS_ = 0xBD,   /* -_ */
	VK_OEM_PERIOD_ = 0xBE,  /* .> */
	VK_OEM_2_ = 0xBF,       /* /? */
	VK_OEM_3_ = 0xC0,       /* `~ */
	VK_OEM_4_ = 0xDB,       /* [{ */
	VK_OEM_5_ = 0xDC,       /* \| */
	VK_OEM_6_ = 0xDD,       /* ]} */
	VK_OEM_7_ = 0xDE        /* '" */
};

/* Modifier state in a neutral form (translated from SDL_Keymod by the caller). */
enum {
	MOD_SHIFT = 0x01,
	MOD_CTRL  = 0x02,
	MOD_ALT   = 0x04,
	MOD_NUM   = 0x08,	/* num lock is on */
	MOD_CAPS  = 0x10	/* caps lock is on */
};

/* SDL keycode (SDL_Keycode value) + SDL scancode (SDL_Scancode value) -> Windows VK code.
** Letters and digits follow the keycode (keyboard layout, as Windows VKs do);
** everything else follows the physical key (scancode). Keypad keys depend on num
** lock like on Windows (num lock off: KP 7 = VK_HOME, KP 5 = VK_CLEAR, ...).
** Returns 0 for keys the game has no code for. */
unsigned Map_Key(uint32_t sdl_keycode, uint32_t sdl_scancode, unsigned mods);

/* Build the WW key code that WWKeyboardClass::Put_Key_Message produced:
**   vk | SHIFT_BIT (shift held) | CTRL_BIT | ALT_BIT | RLS_BIT (release).
** Mouse buttons never get modifier bits. Caps lock and num lock never set the
** shift bit: the original tested GetKeyState(...) & 0x0008, a bit Windows never
** sets (the toggle state is bit 0), so it was dead code; we keep that behaviour. */
unsigned Make_WW_Key(unsigned vk, unsigned mods, bool release);

/* ToAscii() for a US keyboard layout, with the shift/ctrl state taken from the
** key's own WWKEY bits (the original filled a zeroed KeyState array from those
** bits only, so caps lock was ignored). Ctrl+letter gives the control character.
** Released keys and keys without a character give 0. */
char WW_Key_To_ASCII(unsigned wwkey);

} // namespace ra_platform

#endif
