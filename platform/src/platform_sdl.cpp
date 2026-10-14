/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer: SDL3 back end for window, frame presentation, palette, input,
**	event pump and poll hooks (session S08). See docs/codebase/platform-layer.md.
**
**	This is the only file that talks to SDL video/events. Everything is single-
**	threaded: nothing here runs on a timer thread (the original ran the mouse
**	cursor and the tick counter from Windows multimedia-timer callbacks).
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#include "../platform.h"
#include "keymap.h"
#include "viewport.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

using namespace ra_platform;

namespace {

enum { INPUT_QUEUE_SIZE = 256 };

struct PollHookEntry {
	PlatformPollHook Hook;
	void *User;
};

struct PlatformState {
	bool Initialised;

	/* Video */
	SDL_Window *Window;
	SDL_Renderer *Renderer;
	SDL_Texture *Texture;
	int TextureW;
	int TextureH;
	PlatformVideoConfig Config;
	bool ConfigSet;
	bool PaletteSet;
	Uint32 Palette32[256];			/* XRGB8888 for the texture */
	unsigned char Palette8[768];	/* 8-bit RGB, for Platform_Get_Palette */
	const unsigned char *Frame;
	int FrameW;
	int FrameH;
	int FramePitch;
	int PresentInterval;			/* 0 = default (16 ms), negative = auto-present off */
	uint32_t LastPresent;
	Rect Viewport;					/* in output (render) pixels */
	float PixelScaleX;				/* output pixels per window coordinate */
	float PixelScaleY;

	/* Input */
	PlatformInputEvent Queue[INPUT_QUEUE_SIZE];
	int QueueHead;
	int QueueTail;
	int MouseX;
	int MouseY;
	bool ButtonDown[3];				/* left, right, middle */
	bool HasFocus;
	bool QuitRequested;
	PlatformFocusCallback FocusCallback;

	PollHookEntry Hooks[PLATFORM_MAX_POLL_HOOKS];
	int HookCount;
	bool InPump;

	char Error[256];
};

PlatformState State;


void Set_Error(const char *what)
{
	snprintf(State.Error, sizeof(State.Error), "%s: %s", what, SDL_GetError());
}


unsigned Neutral_Mods(SDL_Keymod mod)
{
	unsigned m = 0;
	if (mod & SDL_KMOD_SHIFT) m |= MOD_SHIFT;
	if (mod & SDL_KMOD_CTRL)  m |= MOD_CTRL;
	if (mod & SDL_KMOD_ALT)   m |= MOD_ALT;
	if (mod & SDL_KMOD_NUM)   m |= MOD_NUM;
	if (mod & SDL_KMOD_CAPS)  m |= MOD_CAPS;
	return m;
}


void Queue_Input(int type, unsigned key, int x, int y)
{
	int next = (State.QueueTail + 1) % INPUT_QUEUE_SIZE;
	if (next == State.QueueHead) {
		return;		/* full: drop, like a full WWKeyboardClass buffer */
	}
	PlatformInputEvent &e = State.Queue[State.QueueTail];
	e.Type = type;
	e.Key = key;
	e.X = x;
	e.Y = y;
	State.QueueTail = next;
}


int Queue_Count(void)
{
	return (State.QueueTail - State.QueueHead + INPUT_QUEUE_SIZE) % INPUT_QUEUE_SIZE;
}


void Update_Viewport(void)
{
	if (!State.Renderer) return;
	int out_w = 0, out_h = 0, win_w = 0, win_h = 0;
	SDL_GetCurrentRenderOutputSize(State.Renderer, &out_w, &out_h);
	SDL_GetWindowSize(State.Window, &win_w, &win_h);
	State.PixelScaleX = (win_w > 0) ? (float)out_w / (float)win_w : 1.0f;
	State.PixelScaleY = (win_h > 0) ? (float)out_h / (float)win_h : 1.0f;
	int gw = State.Frame ? State.FrameW : State.Config.Width;
	int gh = State.Frame ? State.FrameH : State.Config.Height;
	State.Viewport = Compute_Viewport(out_w, out_h, gw, gh, State.Config.ScaleMode, State.Config.AspectCorrect != 0);
}


/* Window coordinates (SDL mouse events) -> game coordinates. */
void Window_To_Game(float wx, float wy, int *gx, int *gy)
{
	int gw = State.Frame ? State.FrameW : State.Config.Width;
	int gh = State.Frame ? State.FrameH : State.Config.Height;
	Output_To_Game(State.Viewport, gw, gh, wx * State.PixelScaleX, wy * State.PixelScaleY, gx, gy);
}


bool Ensure_Texture(int w, int h)
{
	if (State.Texture && State.TextureW == w && State.TextureH == h) return true;
	if (State.Texture) {
		SDL_DestroyTexture(State.Texture);
		State.Texture = NULL;
	}
	State.Texture = SDL_CreateTexture(State.Renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (!State.Texture) {
		Set_Error("SDL_CreateTexture");
		return false;
	}
	/* Whole-multiple scaling stays sharp with NEAREST; other sizes would get uneven
	** pixel columns, so PIXELART (SDL 3.4) smooths only the pixel edges. */
	SDL_SetTextureScaleMode(State.Texture,
		State.Config.ScaleMode == PLATFORM_SCALE_INTEGER && !State.Config.AspectCorrect ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_PIXELART);
	State.TextureW = w;
	State.TextureH = h;
	return true;
}


void Handle_Key(const SDL_KeyboardEvent &key)
{
	unsigned mods = Neutral_Mods(key.mod);

	/* Alt+Enter toggles fullscreen and is not passed to the game. */
	if (key.down && (key.key == SDLK_RETURN || key.key == SDLK_KP_ENTER) && (mods & MOD_ALT) && !(mods & MOD_CTRL)) {
		if (!key.repeat) Platform_Toggle_Fullscreen();
		return;
	}

	unsigned vk = Map_Key((uint32_t)key.key, (uint32_t)key.scancode, mods);
	if (vk == 0) return;

	/* Auto-repeat produces more key-down codes, like repeated WM_KEYDOWN messages. */
	Queue_Input(PLATFORM_INPUT_KEY, Make_WW_Key(vk, mods, !key.down), 0, 0);
}


void Handle_Button(const SDL_MouseButtonEvent &button)
{
	unsigned vk;
	int index;
	switch (button.button) {
		case SDL_BUTTON_LEFT:   vk = VK_LBUTTON_; index = 0; break;
		case SDL_BUTTON_RIGHT:  vk = VK_RBUTTON_; index = 1; break;
		case SDL_BUTTON_MIDDLE: vk = VK_MBUTTON_; index = 2; break;
		default: return;
	}
	State.ButtonDown[index] = button.down;
	int gx, gy;
	Window_To_Game(button.x, button.y, &gx, &gy);
	State.MouseX = gx;
	State.MouseY = gy;
	/* No double-click translation: the original window class had no CS_DBLCLKS. */
	Queue_Input(PLATFORM_INPUT_MOUSE, Make_WW_Key(vk, 0, !button.down), gx, gy);
}


void Set_Focus(bool focus)
{
	if (State.HasFocus == focus) return;
	State.HasFocus = focus;
	if (State.FocusCallback) State.FocusCallback(focus);
}

} // namespace


/* ======================================================================== */
/* Lifetime                                                                  */
/* ======================================================================== */

bool Platform_Init(void)
{
	if (State.Initialised) return true;

	/*
	** State is a zero-initialised global with no constructor, so it is valid even
	** when a static constructor elsewhere calls in before this file's constructors
	** would have run. Hooks, the focus callback and a palette set before
	** Platform_Init are kept.
	*/
	if (!State.ConfigSet) {
		Platform_Default_Video_Config(&State.Config);
		State.ConfigSet = true;
	}
	if (!State.PaletteSet) {
		/* Grey ramp until the game sets a palette, so a test pattern is visible. */
		unsigned char ramp[768];
		for (int i = 0; i < 256; i++) {
			ramp[i * 3 + 0] = ramp[i * 3 + 1] = ramp[i * 3 + 2] = (unsigned char)i;
		}
		Platform_Set_Palette(ramp, 8);
	}
	State.QueueHead = State.QueueTail = 0;
	State.QuitRequested = false;

	SDL_SetAppMetadata("Red Alert", NULL, "org.redalert.port");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		Set_Error("SDL_Init");
		return false;
	}
	(void)Platform_Milliseconds();	/* start the clock */
	State.Initialised = true;
	return true;
}


void Platform_Shutdown(void)
{
	if (!State.Initialised) return;
	Platform_Video_Shutdown();
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	State.Initialised = false;
}


const char *Platform_Last_Error(void)
{
	return State.Error;
}


/* ======================================================================== */
/* Video                                                                     */
/* ======================================================================== */

void Platform_Default_Video_Config(PlatformVideoConfig *config)
{
	if (!config) return;
	config->Width = 640;
	config->Height = 400;
	config->WindowScale = 0;
	config->Fullscreen = 0;
	config->ScaleMode = PLATFORM_SCALE_FIT;
	config->AspectCorrect = 1;
}


bool Platform_Video_Init(const PlatformVideoConfig *config, const char *title)
{
	if (!State.Initialised && !Platform_Init()) return false;

	if (config) {
		State.Config = *config;
		State.ConfigSet = true;
	}
	if (State.Config.Width <= 0 || State.Config.Height <= 0) {
		snprintf(State.Error, sizeof(State.Error), "Platform_Video_Init: bad size %dx%d",
			(int)State.Config.Width, (int)State.Config.Height);
		return false;
	}

	/* Window size: the (aspect-corrected) game size times a whole factor. */
	int disp_w = State.Config.Width;
	int disp_h = State.Config.AspectCorrect ? (State.Config.Width * 3) / 4 : State.Config.Height;
	int scale = State.Config.WindowScale;
	if (scale <= 0) {
		scale = 1;
		SDL_DisplayID display = SDL_GetPrimaryDisplay();
		SDL_Rect usable;
		if (display && SDL_GetDisplayUsableBounds(display, &usable)) {
			while (disp_w * (scale + 1) <= usable.w * 9 / 10 && disp_h * (scale + 1) <= usable.h * 9 / 10) {
				scale++;
			}
		}
	}

	if (State.Window) {
		SDL_SetWindowSize(State.Window, disp_w * scale, disp_h * scale);
		if (title) SDL_SetWindowTitle(State.Window, title);
	} else {
		SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
		State.Window = SDL_CreateWindow(title ? title : "Red Alert", disp_w * scale, disp_h * scale, flags);
		if (!State.Window) {
			Set_Error("SDL_CreateWindow");
			return false;
		}
		SDL_SetWindowMinimumSize(State.Window, 320, 200);
		State.Renderer = SDL_CreateRenderer(State.Window, NULL);
		if (!State.Renderer) {
			Set_Error("SDL_CreateRenderer");
			SDL_DestroyWindow(State.Window);
			State.Window = NULL;
			return false;
		}
		/* No vsync: presents happen from inside the game's busy loops and must not block. */
		SDL_SetRenderVSync(State.Renderer, 0);
	}

	SDL_SetWindowFullscreen(State.Window, State.Config.Fullscreen != 0);
	SDL_ShowWindow(State.Window);
	SDL_RaiseWindow(State.Window);

	if (!Ensure_Texture(State.Config.Width, State.Config.Height)) {
		return false;
	}
	State.HasFocus = (SDL_GetWindowFlags(State.Window) & SDL_WINDOW_INPUT_FOCUS) != 0;
	Update_Viewport();

	/* Start with a black window. */
	SDL_SetRenderDrawColor(State.Renderer, 0, 0, 0, 255);
	SDL_RenderClear(State.Renderer);
	SDL_RenderPresent(State.Renderer);
	return true;
}


void Platform_Video_Shutdown(void)
{
	if (State.Texture)  SDL_DestroyTexture(State.Texture);
	if (State.Renderer) SDL_DestroyRenderer(State.Renderer);
	if (State.Window)   SDL_DestroyWindow(State.Window);
	State.Texture = NULL;
	State.Renderer = NULL;
	State.Window = NULL;
	State.TextureW = State.TextureH = 0;
}


bool Platform_Video_Is_Open(void)
{
	return State.Window != NULL;
}


void Platform_Set_Frame_Buffer(const unsigned char *pixels, int width, int height, int pitch)
{
	if (!pixels || width <= 0 || height <= 0) {
		State.Frame = NULL;
		State.FrameW = State.FrameH = State.FramePitch = 0;
		return;
	}
	State.Frame = pixels;
	State.FrameW = width;
	State.FrameH = height;
	State.FramePitch = pitch > 0 ? pitch : width;
	Update_Viewport();
}


void Platform_Present(void)
{
	if (!State.Renderer) return;
	State.LastPresent = Platform_Milliseconds();

	SDL_SetRenderDrawColor(State.Renderer, 0, 0, 0, 255);
	SDL_RenderClear(State.Renderer);

	if (State.Frame && Ensure_Texture(State.FrameW, State.FrameH)) {
		void *pixels = NULL;
		int pitch = 0;
		if (SDL_LockTexture(State.Texture, NULL, &pixels, &pitch)) {
			for (int y = 0; y < State.FrameH; y++) {
				const unsigned char *src = State.Frame + (size_t)y * State.FramePitch;
				Uint32 *dst = (Uint32 *)((unsigned char *)pixels + (size_t)y * pitch);
				for (int x = 0; x < State.FrameW; x++) {
					dst[x] = State.Palette32[src[x]];
				}
			}
			SDL_UnlockTexture(State.Texture);
		}
		SDL_FRect dst;
		dst.x = (float)State.Viewport.X;
		dst.y = (float)State.Viewport.Y;
		dst.w = (float)State.Viewport.W;
		dst.h = (float)State.Viewport.H;
		SDL_RenderTexture(State.Renderer, State.Texture, NULL, &dst);
	}
	SDL_RenderPresent(State.Renderer);
}


void Platform_Wait_Vertical_Blank(void)
{
	Platform_Present();
	uint64_t now = Platform_Milliseconds64();
	uint64_t next = ((now * 60) / 1000 + 1) * 1000 / 60;
	if (next > now) Platform_Sleep((int)(next - now));
}


void Platform_Set_Present_Interval(int milliseconds)
{
	State.PresentInterval = milliseconds <= 0 ? -1 : milliseconds;
}


void Platform_Set_Palette(const unsigned char *rgb768, int bits)
{
	if (!rgb768) return;
	for (int i = 0; i < 256; i++) {
		unsigned r = rgb768[i * 3 + 0];
		unsigned g = rgb768[i * 3 + 1];
		unsigned b = rgb768[i * 3 + 2];
		if (bits == 6) {
			/* Exactly what Set_DD_Palette did: (value << 2), stored in a byte. */
			r = (r << 2) & 0xFF;
			g = (g << 2) & 0xFF;
			b = (b << 2) & 0xFF;
		}
		State.Palette8[i * 3 + 0] = (unsigned char)r;
		State.Palette8[i * 3 + 1] = (unsigned char)g;
		State.Palette8[i * 3 + 2] = (unsigned char)b;
		State.Palette32[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
	}
	State.PaletteSet = true;
	State.LastPresent = 0;		/* show the new colours at the next pump */
}


void Platform_Get_Palette(unsigned char *rgb768)
{
	if (rgb768) memcpy(rgb768, State.Palette8, sizeof(State.Palette8));
}


void Platform_Set_Fullscreen(bool fullscreen)
{
	State.Config.Fullscreen = fullscreen ? 1 : 0;
	if (State.Window) {
		SDL_SetWindowFullscreen(State.Window, fullscreen);
		SDL_SyncWindow(State.Window);
		Update_Viewport();
	}
}


bool Platform_Is_Fullscreen(void)
{
	if (State.Window) {
		return (SDL_GetWindowFlags(State.Window) & SDL_WINDOW_FULLSCREEN) != 0;
	}
	return State.Config.Fullscreen != 0;
}


void Platform_Toggle_Fullscreen(void)
{
	Platform_Set_Fullscreen(!Platform_Is_Fullscreen());
}


void Platform_Set_Scale_Mode(int scale_mode, bool aspect_correct)
{
	if (scale_mode < PLATFORM_SCALE_INTEGER || scale_mode > PLATFORM_SCALE_STRETCH) return;
	if (!State.ConfigSet) {
		Platform_Default_Video_Config(&State.Config);
		State.ConfigSet = true;
	}
	State.Config.ScaleMode = scale_mode;
	State.Config.AspectCorrect = aspect_correct ? 1 : 0;
	if (State.Texture) {
		SDL_SetTextureScaleMode(State.Texture,
			scale_mode == PLATFORM_SCALE_INTEGER && !aspect_correct ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_PIXELART);
	}
	Update_Viewport();
}


void Platform_Show_System_Cursor(bool show)
{
	if (show) SDL_ShowCursor(); else SDL_HideCursor();
}


void Platform_Set_Mouse_Clip(bool clip)
{
	if (State.Window) SDL_SetWindowMouseGrab(State.Window, clip);
}


void Platform_Message_Box(const char *title, const char *text)
{
	if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title ? title : "Red Alert", text ? text : "", State.Window)) {
		fprintf(stderr, "%s: %s\n", title ? title : "Red Alert", text ? text : "");
	}
}


/* ======================================================================== */
/* Input and the event pump                                                  */
/* ======================================================================== */

int Platform_Pump_Events(void)
{
	if (!State.Initialised) return 0;
	/* A poll hook may end up back here (for example through Keyboard->Check()). */
	if (State.InPump) return Queue_Count();
	State.InPump = true;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				State.QuitRequested = true;
				break;

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				Handle_Key(event.key);
				break;

			case SDL_EVENT_MOUSE_MOTION: {
				int gx, gy;
				Window_To_Game(event.motion.x, event.motion.y, &gx, &gy);
				State.MouseX = gx;
				State.MouseY = gy;
				break;
			}

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
				Handle_Button(event.button);
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				Set_Focus(true);
				break;

			case SDL_EVENT_WINDOW_FOCUS_LOST:
				/* Buttons released while away would otherwise stay "down". */
				State.ButtonDown[0] = State.ButtonDown[1] = State.ButtonDown[2] = false;
				Set_Focus(false);
				break;

			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
			case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
				Update_Viewport();
				State.LastPresent = 0;
				break;

			case SDL_EVENT_WINDOW_EXPOSED:
				State.LastPresent = 0;	/* redraw at the next opportunity */
				break;

			default:
				break;
		}
	}

	for (int i = 0; i < State.HookCount; i++) {
		State.Hooks[i].Hook(State.Hooks[i].User);
	}

	if (State.Renderer && State.Frame && State.PresentInterval >= 0) {
		uint32_t interval = State.PresentInterval ? (uint32_t)State.PresentInterval : 16;
		uint32_t now = Platform_Milliseconds();
		if (State.LastPresent == 0 || now - State.LastPresent >= interval) {
			Platform_Present();
		}
	}

	State.InPump = false;
	return Queue_Count();
}


bool Platform_Get_Input(PlatformInputEvent *event)
{
	if (State.QueueHead == State.QueueTail) return false;
	if (event) *event = State.Queue[State.QueueHead];
	State.QueueHead = (State.QueueHead + 1) % INPUT_QUEUE_SIZE;
	return true;
}


bool Platform_Peek_Input(PlatformInputEvent *event)
{
	if (State.QueueHead == State.QueueTail) return false;
	if (event) *event = State.Queue[State.QueueHead];
	return true;
}


void Platform_Clear_Input(void)
{
	State.QueueHead = State.QueueTail;
}


void Platform_Get_Mouse_Position(int *x, int *y)
{
	if (x) *x = State.MouseX;
	if (y) *y = State.MouseY;
}


bool Platform_Key_Down(unsigned vk)
{
	vk &= 0xFF;
	switch (vk) {
		case VK_LBUTTON_: return State.ButtonDown[0];
		case VK_RBUTTON_: return State.ButtonDown[1];
		case VK_MBUTTON_: return State.ButtonDown[2];
		default: break;
	}

	if (!State.Initialised) return false;
	SDL_Keymod mod = SDL_GetModState();
	switch (vk) {
		case VK_SHIFT_:   return (mod & SDL_KMOD_SHIFT) != 0;
		case VK_CONTROL_: return (mod & SDL_KMOD_CTRL) != 0;
		case VK_MENU_:    return (mod & SDL_KMOD_ALT) != 0;
		default: break;
	}

	/* Any physical key that maps to this VK with the current lock state. */
	int count = 0;
	const bool *keys = SDL_GetKeyboardState(&count);
	unsigned mods = Neutral_Mods(mod);
	for (int sc = 0; sc < count; sc++) {
		if (keys[sc]) {
			SDL_Keycode key = SDL_GetKeyFromScancode((SDL_Scancode)sc, SDL_KMOD_NONE, false);
			if (Map_Key((uint32_t)key, (uint32_t)sc, mods) == vk) return true;
		}
	}
	return false;
}


unsigned Platform_Modifier_Bits(void)
{
	if (!State.Initialised) return 0;
	return Make_WW_Key(0, Neutral_Mods(SDL_GetModState()), false) & ~0xFFu;
}


char Platform_Key_To_ASCII(unsigned wwkey)
{
	return WW_Key_To_ASCII(wwkey);
}


bool Platform_Has_Focus(void)
{
	return State.HasFocus;
}


bool Platform_Quit_Requested(void)
{
	return State.QuitRequested;
}


void Platform_Set_Focus_Callback(PlatformFocusCallback callback)
{
	State.FocusCallback = callback;
}


bool Platform_Add_Poll_Hook(PlatformPollHook hook, void *user)
{
	if (!hook) return false;
	for (int i = 0; i < State.HookCount; i++) {
		if (State.Hooks[i].Hook == hook && State.Hooks[i].User == user) return true;
	}
	if (State.HookCount >= PLATFORM_MAX_POLL_HOOKS) return false;
	State.Hooks[State.HookCount].Hook = hook;
	State.Hooks[State.HookCount].User = user;
	State.HookCount++;
	return true;
}


bool Platform_Remove_Poll_Hook(PlatformPollHook hook, void *user)
{
	for (int i = 0; i < State.HookCount; i++) {
		if (State.Hooks[i].Hook == hook && State.Hooks[i].User == user) {
			for (int j = i + 1; j < State.HookCount; j++) {
				State.Hooks[j - 1] = State.Hooks[j];
			}
			State.HookCount--;
			return true;
		}
	}
	return false;
}
