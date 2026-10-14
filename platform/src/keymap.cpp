/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer: key translation (session S08). See keymap.h.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#include "keymap.h"
#include "../platform.h"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

namespace ra_platform {

namespace {

struct ScanToVK {
	uint32_t Scan;
	unsigned VK;
};

/* Physical keys, by scancode. Letters and digits are handled from the keycode first. */
const ScanToVK ScanTable[] = {
	{ SDL_SCANCODE_RETURN, VK_RETURN_ },	{ SDL_SCANCODE_ESCAPE, VK_ESCAPE_ },
	{ SDL_SCANCODE_BACKSPACE, VK_BACK_ },	{ SDL_SCANCODE_TAB, VK_TAB_ },
	{ SDL_SCANCODE_SPACE, VK_SPACE_ },

	{ SDL_SCANCODE_MINUS, VK_OEM_MINUS_ },	{ SDL_SCANCODE_EQUALS, VK_OEM_PLUS_ },
	{ SDL_SCANCODE_LEFTBRACKET, VK_OEM_4_ },	{ SDL_SCANCODE_RIGHTBRACKET, VK_OEM_6_ },
	{ SDL_SCANCODE_BACKSLASH, VK_OEM_5_ },	{ SDL_SCANCODE_NONUSHASH, VK_OEM_5_ },
	{ SDL_SCANCODE_SEMICOLON, VK_OEM_1_ },	{ SDL_SCANCODE_APOSTROPHE, VK_OEM_7_ },
	{ SDL_SCANCODE_GRAVE, VK_OEM_3_ },		{ SDL_SCANCODE_COMMA, VK_OEM_COMMA_ },
	{ SDL_SCANCODE_PERIOD, VK_OEM_PERIOD_ },	{ SDL_SCANCODE_SLASH, VK_OEM_2_ },
	{ SDL_SCANCODE_NONUSBACKSLASH, VK_OEM_5_ },

	{ SDL_SCANCODE_CAPSLOCK, VK_CAPITAL_ },

	{ SDL_SCANCODE_F1, VK_F1_ + 0 },  { SDL_SCANCODE_F2, VK_F1_ + 1 },  { SDL_SCANCODE_F3, VK_F1_ + 2 },
	{ SDL_SCANCODE_F4, VK_F1_ + 3 },  { SDL_SCANCODE_F5, VK_F1_ + 4 },  { SDL_SCANCODE_F6, VK_F1_ + 5 },
	{ SDL_SCANCODE_F7, VK_F1_ + 6 },  { SDL_SCANCODE_F8, VK_F1_ + 7 },  { SDL_SCANCODE_F9, VK_F1_ + 8 },
	{ SDL_SCANCODE_F10, VK_F1_ + 9 }, { SDL_SCANCODE_F11, VK_F1_ + 10 }, { SDL_SCANCODE_F12, VK_F1_ + 11 },

	{ SDL_SCANCODE_PRINTSCREEN, VK_SNAPSHOT_ },	{ SDL_SCANCODE_SCROLLLOCK, VK_SCROLL_ },
	{ SDL_SCANCODE_PAUSE, VK_PAUSE_ },
	{ SDL_SCANCODE_INSERT, VK_INSERT_ },	{ SDL_SCANCODE_HOME, VK_HOME_ },
	{ SDL_SCANCODE_PAGEUP, VK_PRIOR_ },	{ SDL_SCANCODE_DELETE, VK_DELETE_ },
	{ SDL_SCANCODE_END, VK_END_ },			{ SDL_SCANCODE_PAGEDOWN, VK_NEXT_ },
	{ SDL_SCANCODE_RIGHT, VK_RIGHT_ },		{ SDL_SCANCODE_LEFT, VK_LEFT_ },
	{ SDL_SCANCODE_DOWN, VK_DOWN_ },		{ SDL_SCANCODE_UP, VK_UP_ },

	{ SDL_SCANCODE_NUMLOCKCLEAR, VK_NUMLOCK_ },
	{ SDL_SCANCODE_KP_DIVIDE, VK_DIVIDE_ },	{ SDL_SCANCODE_KP_MULTIPLY, VK_MULTIPLY_ },
	{ SDL_SCANCODE_KP_MINUS, VK_SUBTRACT_ },	{ SDL_SCANCODE_KP_PLUS, VK_ADD_ },
	{ SDL_SCANCODE_KP_ENTER, VK_RETURN_ },

	{ SDL_SCANCODE_LCTRL, VK_CONTROL_ },	{ SDL_SCANCODE_RCTRL, VK_CONTROL_ },
	{ SDL_SCANCODE_LSHIFT, VK_SHIFT_ },	{ SDL_SCANCODE_RSHIFT, VK_SHIFT_ },
	{ SDL_SCANCODE_LALT, VK_MENU_ },		{ SDL_SCANCODE_RALT, VK_MENU_ },
};

/* Keypad digits and period: [num lock on, num lock off] like Windows. */
struct KeypadVK {
	uint32_t Scan;
	unsigned On;
	unsigned Off;
};

const KeypadVK KeypadTable[] = {
	{ SDL_SCANCODE_KP_0, VK_NUMPAD0_ + 0, VK_INSERT_ },
	{ SDL_SCANCODE_KP_1, VK_NUMPAD0_ + 1, VK_END_ },
	{ SDL_SCANCODE_KP_2, VK_NUMPAD0_ + 2, VK_DOWN_ },
	{ SDL_SCANCODE_KP_3, VK_NUMPAD0_ + 3, VK_NEXT_ },
	{ SDL_SCANCODE_KP_4, VK_NUMPAD0_ + 4, VK_LEFT_ },
	{ SDL_SCANCODE_KP_5, VK_NUMPAD0_ + 5, VK_CLEAR_ },
	{ SDL_SCANCODE_KP_6, VK_NUMPAD0_ + 6, VK_RIGHT_ },
	{ SDL_SCANCODE_KP_7, VK_NUMPAD0_ + 7, VK_HOME_ },
	{ SDL_SCANCODE_KP_8, VK_NUMPAD0_ + 8, VK_UP_ },
	{ SDL_SCANCODE_KP_9, VK_NUMPAD0_ + 9, VK_PRIOR_ },
	{ SDL_SCANCODE_KP_PERIOD, VK_DECIMAL_, VK_DELETE_ },
};

} // namespace

unsigned Map_Key(uint32_t sdl_keycode, uint32_t sdl_scancode, unsigned mods)
{
	/*
	** Keypad first: SDL may report keypad keys with layout-dependent keycodes,
	** but Windows decides purely by the physical key and the num lock state.
	*/
	for (unsigned i = 0; i < sizeof(KeypadTable) / sizeof(KeypadTable[0]); i++) {
		if (KeypadTable[i].Scan == sdl_scancode) {
			return (mods & MOD_NUM) ? KeypadTable[i].On : KeypadTable[i].Off;
		}
	}

	/*
	** Letters and digits: Windows VKs follow the keyboard layout (AZERTY 'A' is VK_A),
	** and so does the SDL keycode.
	*/
	if (sdl_keycode >= SDLK_A && sdl_keycode <= SDLK_Z) {
		return 'A' + (sdl_keycode - SDLK_A);
	}
	if (sdl_keycode >= SDLK_0 && sdl_keycode <= SDLK_9) {
		return '0' + (sdl_keycode - SDLK_0);
	}

	/*
	** Letter and digit keys that produce something else in this layout still
	** have a fixed physical position: fall back to it.
	*/
	if (sdl_scancode >= SDL_SCANCODE_A && sdl_scancode <= SDL_SCANCODE_Z) {
		return 'A' + (sdl_scancode - SDL_SCANCODE_A);
	}
	if (sdl_scancode >= SDL_SCANCODE_1 && sdl_scancode <= SDL_SCANCODE_9) {
		return '1' + (sdl_scancode - SDL_SCANCODE_1);
	}
	if (sdl_scancode == SDL_SCANCODE_0) {
		return '0';
	}

	for (unsigned i = 0; i < sizeof(ScanTable) / sizeof(ScanTable[0]); i++) {
		if (ScanTable[i].Scan == sdl_scancode) {
			return ScanTable[i].VK;
		}
	}
	return 0;
}


unsigned Make_WW_Key(unsigned vk, unsigned mods, bool release)
{
	unsigned key = vk & 0xFF;
	bool mouse = (key == VK_LBUTTON_ || key == VK_RBUTTON_ || key == VK_MBUTTON_);

	if (!mouse) {
		if (mods & MOD_SHIFT) key |= PLATFORM_WWKEY_SHIFT_BIT;
		if (mods & MOD_CTRL)  key |= PLATFORM_WWKEY_CTRL_BIT;
		if (mods & MOD_ALT)   key |= PLATFORM_WWKEY_ALT_BIT;
	}
	if (release) {
		key |= PLATFORM_WWKEY_RLS_BIT;
	}
	return key;
}


char WW_Key_To_ASCII(unsigned wwkey)
{
	if (wwkey & PLATFORM_WWKEY_RLS_BIT) {
		return '\0';
	}

	unsigned vk = wwkey & 0xFF;
	bool shift = (wwkey & PLATFORM_WWKEY_SHIFT_BIT) != 0;
	bool ctrl  = (wwkey & PLATFORM_WWKEY_CTRL_BIT) != 0;
	bool alt   = (wwkey & PLATFORM_WWKEY_ALT_BIT) != 0;

	/*
	** Ctrl+Alt is AltGr; the US layout has no AltGr characters.
	*/
	if (ctrl && alt) {
		return '\0';
	}

	if (ctrl) {
		if (vk >= 'A' && vk <= 'Z') return (char)(vk - 'A' + 1);
		switch (vk) {
			case VK_OEM_4_:   return 0x1B;	/* Ctrl+[ */
			case VK_OEM_5_:   return 0x1C;	/* Ctrl+\ */
			case VK_OEM_6_:   return 0x1D;	/* Ctrl+] */
			case VK_BACK_:    return 0x7F;
			case VK_RETURN_:  return 0x0A;
			case VK_SPACE_:   return ' ';
			case VK_ESCAPE_:  return 0x1B;
			case VK_TAB_:     return '\0';
			default:          return '\0';
		}
	}

	if (vk >= 'A' && vk <= 'Z') {
		return (char)(shift ? vk : vk - 'A' + 'a');
	}
	if (vk >= '0' && vk <= '9') {
		if (!shift) return (char)vk;
		static const char shifted_digits[] = ")!@#$%^&*(";
		return shifted_digits[vk - '0'];
	}
	if (vk >= VK_NUMPAD0_ && vk <= VK_NUMPAD0_ + 9) {
		return (char)('0' + (vk - VK_NUMPAD0_));
	}

	switch (vk) {
		case VK_SPACE_:      return ' ';
		case VK_RETURN_:     return '\r';
		case VK_BACK_:       return '\b';
		case VK_TAB_:        return '\t';
		case VK_ESCAPE_:     return 0x1B;
		case VK_MULTIPLY_:   return '*';
		case VK_ADD_:        return '+';
		case VK_SUBTRACT_:   return '-';
		case VK_DECIMAL_:    return '.';
		case VK_DIVIDE_:     return '/';
		case VK_OEM_1_:      return shift ? ':' : ';';
		case VK_OEM_PLUS_:   return shift ? '+' : '=';
		case VK_OEM_COMMA_:  return shift ? '<' : ',';
		case VK_OEM_MINUS_:  return shift ? '_' : '-';
		case VK_OEM_PERIOD_: return shift ? '>' : '.';
		case VK_OEM_2_:      return shift ? '?' : '/';
		case VK_OEM_3_:      return shift ? '~' : '`';
		case VK_OEM_4_:      return shift ? '{' : '[';
		case VK_OEM_5_:      return shift ? '|' : '\\';
		case VK_OEM_6_:      return shift ? '}' : ']';
		case VK_OEM_7_:      return shift ? '"' : '\'';
		default:             return '\0';
	}
}

} // namespace ra_platform
