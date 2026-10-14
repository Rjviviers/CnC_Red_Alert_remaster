// S08: key translation of the platform layer (platform/src/keymap.cpp).
// SDL key event -> Windows VK -> WW key code (as WWKeyboardClass::Put_Key_Message built it) -> ASCII.
#include "ra_test.h"
#include "keymap.h"
#include "platform.h"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

using namespace ra_platform;

RA_TEST(letters_and_digits_follow_the_keycode) {
    RA_CHECK_EQ(0x41, Map_Key(SDLK_A, SDL_SCANCODE_A, 0));          // VK_A
    RA_CHECK_EQ(0x5A, Map_Key(SDLK_Z, SDL_SCANCODE_Z, 0));
    // AZERTY: the physical Q key produces 'a' -> VK_A, like Windows.
    RA_CHECK_EQ(0x41, Map_Key(SDLK_A, SDL_SCANCODE_Q, 0));
    RA_CHECK_EQ(0x30, Map_Key(SDLK_0, SDL_SCANCODE_0, 0));
    RA_CHECK_EQ(0x39, Map_Key(SDLK_9, SDL_SCANCODE_9, 0));
    // A digit key whose layout character is not a digit still maps by position.
    RA_CHECK_EQ(0x31, Map_Key(SDLK_AMPERSAND, SDL_SCANCODE_1, 0));
}

RA_TEST(special_keys) {
    RA_CHECK_EQ(0x1B, Map_Key(SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, 0));   // VK_ESCAPE
    RA_CHECK_EQ(0x0D, Map_Key(SDLK_RETURN, SDL_SCANCODE_RETURN, 0));   // VK_RETURN
    RA_CHECK_EQ(0x0D, Map_Key(SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER, 0));
    RA_CHECK_EQ(0x08, Map_Key(SDLK_BACKSPACE, SDL_SCANCODE_BACKSPACE, 0));
    RA_CHECK_EQ(0x09, Map_Key(SDLK_TAB, SDL_SCANCODE_TAB, 0));
    RA_CHECK_EQ(0x20, Map_Key(SDLK_SPACE, SDL_SCANCODE_SPACE, 0));
    RA_CHECK_EQ(0x70, Map_Key(SDLK_F1, SDL_SCANCODE_F1, 0));           // VK_F1
    RA_CHECK_EQ(0x7B, Map_Key(SDLK_F12, SDL_SCANCODE_F12, 0));         // VK_F12
    RA_CHECK_EQ(0x25, Map_Key(SDLK_LEFT, SDL_SCANCODE_LEFT, 0));       // VK_LEFT
    RA_CHECK_EQ(0x26, Map_Key(SDLK_UP, SDL_SCANCODE_UP, 0));
    RA_CHECK_EQ(0x27, Map_Key(SDLK_RIGHT, SDL_SCANCODE_RIGHT, 0));
    RA_CHECK_EQ(0x28, Map_Key(SDLK_DOWN, SDL_SCANCODE_DOWN, 0));
    RA_CHECK_EQ(0x24, Map_Key(SDLK_HOME, SDL_SCANCODE_HOME, 0));
    RA_CHECK_EQ(0x23, Map_Key(SDLK_END, SDL_SCANCODE_END, 0));
    RA_CHECK_EQ(0x21, Map_Key(SDLK_PAGEUP, SDL_SCANCODE_PAGEUP, 0));
    RA_CHECK_EQ(0x22, Map_Key(SDLK_PAGEDOWN, SDL_SCANCODE_PAGEDOWN, 0));
    RA_CHECK_EQ(0x2D, Map_Key(SDLK_INSERT, SDL_SCANCODE_INSERT, 0));
    RA_CHECK_EQ(0x2E, Map_Key(SDLK_DELETE, SDL_SCANCODE_DELETE, 0));
    RA_CHECK_EQ(0x10, Map_Key(SDLK_LSHIFT, SDL_SCANCODE_LSHIFT, 0));   // VK_SHIFT
    RA_CHECK_EQ(0x10, Map_Key(SDLK_RSHIFT, SDL_SCANCODE_RSHIFT, 0));
    RA_CHECK_EQ(0x11, Map_Key(SDLK_LCTRL, SDL_SCANCODE_LCTRL, 0));     // VK_CONTROL
    RA_CHECK_EQ(0x12, Map_Key(SDLK_RALT, SDL_SCANCODE_RALT, 0));       // VK_MENU
    RA_CHECK_EQ(0x13, Map_Key(SDLK_PAUSE, SDL_SCANCODE_PAUSE, 0));
    RA_CHECK_EQ(0x91, Map_Key(SDLK_SCROLLLOCK, SDL_SCANCODE_SCROLLLOCK, 0));
    RA_CHECK_EQ(0xBD, Map_Key(SDLK_MINUS, SDL_SCANCODE_MINUS, 0));     // VK_OEM_MINUS
    RA_CHECK_EQ(0xBB, Map_Key(SDLK_EQUALS, SDL_SCANCODE_EQUALS, 0));   // VK_OEM_PLUS
    RA_CHECK_EQ(0xDB, Map_Key(SDLK_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET, 0));
    RA_CHECK_EQ(0xC0, Map_Key(SDLK_GRAVE, SDL_SCANCODE_GRAVE, 0));
    RA_CHECK_EQ(0, Map_Key(SDLK_UNKNOWN, SDL_SCANCODE_UNKNOWN, 0));
}

RA_TEST(keypad_depends_on_num_lock_like_windows) {
    RA_CHECK_EQ(0x67, Map_Key(SDLK_KP_7, SDL_SCANCODE_KP_7, MOD_NUM));   // VK_NUMPAD7
    RA_CHECK_EQ(0x24, Map_Key(SDLK_KP_7, SDL_SCANCODE_KP_7, 0));         // VK_HOME
    RA_CHECK_EQ(0x0C, Map_Key(SDLK_KP_5, SDL_SCANCODE_KP_5, 0));         // VK_CLEAR
    RA_CHECK_EQ(0x60, Map_Key(SDLK_KP_0, SDL_SCANCODE_KP_0, MOD_NUM));   // VK_NUMPAD0
    RA_CHECK_EQ(0x2D, Map_Key(SDLK_INSERT, SDL_SCANCODE_KP_0, 0));       // VK_INSERT
    RA_CHECK_EQ(0x6E, Map_Key(SDLK_KP_PERIOD, SDL_SCANCODE_KP_PERIOD, MOD_NUM)); // VK_DECIMAL
    RA_CHECK_EQ(0x2E, Map_Key(SDLK_KP_PERIOD, SDL_SCANCODE_KP_PERIOD, 0));       // VK_DELETE
    RA_CHECK_EQ(0x6B, Map_Key(SDLK_KP_PLUS, SDL_SCANCODE_KP_PLUS, 0));           // VK_ADD
}

RA_TEST(ww_key_bits_match_put_key_message) {
    RA_CHECK_EQ(0x41, Make_WW_Key(0x41, 0, false));
    RA_CHECK_EQ(0x141, Make_WW_Key(0x41, MOD_SHIFT, false));                  // WWKEY_SHIFT_BIT
    RA_CHECK_EQ(0x241, Make_WW_Key(0x41, MOD_CTRL, false));                   // WWKEY_CTRL_BIT
    RA_CHECK_EQ(0x441, Make_WW_Key(0x41, MOD_ALT, false));                    // WWKEY_ALT_BIT
    RA_CHECK_EQ(0x841, Make_WW_Key(0x41, 0, true));                           // WWKEY_RLS_BIT
    RA_CHECK_EQ(0xF41, Make_WW_Key(0x41, MOD_SHIFT | MOD_CTRL | MOD_ALT, true));
    // Caps lock and num lock never set the shift bit (the original's test was dead code).
    RA_CHECK_EQ(0x41, Make_WW_Key(0x41, MOD_CAPS | MOD_NUM, false));
    // Mouse buttons never carry modifier bits ("incompatible with the dos version").
    RA_CHECK_EQ(0x01, Make_WW_Key(0x01, MOD_SHIFT | MOD_CTRL, false));        // VK_LBUTTON
    RA_CHECK_EQ(0x802, Make_WW_Key(0x02, MOD_ALT, true));                     // VK_RBUTTON released
    RA_CHECK_EQ(0x04, Make_WW_Key(0x04, MOD_SHIFT, false));                   // VK_MBUTTON
    // The bits are the ones CODE/KEY.H uses.
    RA_CHECK_EQ(0x100, PLATFORM_WWKEY_SHIFT_BIT);
    RA_CHECK_EQ(0x200, PLATFORM_WWKEY_CTRL_BIT);
    RA_CHECK_EQ(0x400, PLATFORM_WWKEY_ALT_BIT);
    RA_CHECK_EQ(0x800, PLATFORM_WWKEY_RLS_BIT);
}

RA_TEST(to_ascii_us_layout) {
    RA_CHECK_EQ('a', WW_Key_To_ASCII(0x41));
    RA_CHECK_EQ('A', WW_Key_To_ASCII(0x141));
    RA_CHECK_EQ('1', WW_Key_To_ASCII(0x31));
    RA_CHECK_EQ('!', WW_Key_To_ASCII(0x131));
    RA_CHECK_EQ(')', WW_Key_To_ASCII(0x130));
    RA_CHECK_EQ(' ', WW_Key_To_ASCII(0x20));
    RA_CHECK_EQ('\r', WW_Key_To_ASCII(0x0D));
    RA_CHECK_EQ('\b', WW_Key_To_ASCII(0x08));
    RA_CHECK_EQ(27, WW_Key_To_ASCII(0x1B));
    RA_CHECK_EQ('7', WW_Key_To_ASCII(0x67));          // VK_NUMPAD7
    RA_CHECK_EQ('-', WW_Key_To_ASCII(0xBD));
    RA_CHECK_EQ('_', WW_Key_To_ASCII(0x1BD));
    RA_CHECK_EQ('/', WW_Key_To_ASCII(0xBF));
    RA_CHECK_EQ('?', WW_Key_To_ASCII(0x1BF));
    RA_CHECK_EQ(1, WW_Key_To_ASCII(0x241));           // Ctrl+A
    RA_CHECK_EQ('a', WW_Key_To_ASCII(0x441));         // Alt alone does not change the character
    RA_CHECK_EQ(0, WW_Key_To_ASCII(0x641));           // Ctrl+Alt (AltGr): nothing on US
    RA_CHECK_EQ(0, WW_Key_To_ASCII(0x841));           // released keys never give ASCII
    RA_CHECK_EQ(0, WW_Key_To_ASCII(0x70));            // F1
    RA_CHECK_EQ(0, WW_Key_To_ASCII(0x25));            // VK_LEFT
}

RA_TEST(public_to_ascii_is_the_same_function) {
    for (unsigned key = 0; key < 0x1000; key++) {
        if (Platform_Key_To_ASCII(key) != WW_Key_To_ASCII(key)) {
            RA_CHECK(Platform_Key_To_ASCII(key) == WW_Key_To_ASCII(key));
            break;
        }
    }
}

RA_TEST_MAIN()
