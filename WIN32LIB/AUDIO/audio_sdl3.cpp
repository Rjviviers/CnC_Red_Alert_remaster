/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
** SDL3 audio backend (session S09). See audio_backend.h.
**
** Compiled only when RA_HAVE_SDL3 is defined (CMake option RA_HAVE_SDL3, OFF
** by default until session S08 adds SDL3 to the build). Written against the
** SDL 3.2 API: an SDL_AudioStream opened on the default playback device, in
** the mixer's format; SDL converts to the device format. SDL calls the
** stream callback on its audio thread, and the callback pulls from the mixer.
*/
#ifdef RA_HAVE_SDL3

#include "audio_backend.h"

/*
** SDL's headers assert their natural struct layout (SDL_alignment_test in SDL_stdinc.h),
** but the game libraries are built with 1-byte packing (/Zp1, -fpack-struct=1; ADR-0014).
** Include SDL with natural packing on every compiler. On GCC, ra_compat.h has already
** pre-included it the same way; MSVC needs it here, because SDL_begin_code.h's own packing
** push comes after that assert.
*/
#if defined(RA_PACK_STRUCTS)
#  pragma pack(push, 8)
#endif
#include <SDL3/SDL.h>
#if defined(RA_PACK_STRUCTS)
#  pragma pack(pop)
#endif

namespace {

class SDL3AudioBackendClass : public AudioBackendClass {
	public:
		SDL3AudioBackendClass(void) : Stream(0), Callback(0), User(0), Channels(1), SubsystemUp(false) {}
		// No Close() here: this is a static object, destroyed after main() returns,
		// possibly after SDL_Quit(). Sound_End() closes it.
		virtual ~SDL3AudioBackendClass(void) {}

		virtual char const *Name(void) const {return("sdl3");}

		virtual bool Open(AudioDeviceFormat const &want, AudioMixCallback callback, void *user)
		{
			Close();
			if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
				SDL_Log("Audio: SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
				return(false);
			}
			SubsystemUp = true;

			Callback = callback;
			User = user;
			Channels = (want.Channels == 2) ? 2 : 1;

			SDL_AudioSpec spec;
			spec.format = SDL_AUDIO_S16;
			spec.channels = Channels;
			spec.freq = (want.Rate > 0) ? want.Rate : 22050;

			Stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &Stream_Callback, this);
			if (!Stream) {
				SDL_Log("Audio: SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
				Close();
				return(false);
			}
			return(true);		// The device starts paused.
		}

		virtual void Close(void)
		{
			if (Stream) {
				SDL_DestroyAudioStream(Stream);		// Also stops the callback.
				Stream = 0;
			}
			if (SubsystemUp) {
				SDL_QuitSubSystem(SDL_INIT_AUDIO);
				SubsystemUp = false;
			}
			Callback = 0;
			User = 0;
		}

		virtual void Pause(bool paused)
		{
			if (!Stream) return;
			if (paused) {
				SDL_PauseAudioStreamDevice(Stream);
			} else {
				SDL_ResumeAudioStreamDevice(Stream);
			}
		}

		virtual void Set_Gain(float gain)
		{
			if (Stream) SDL_SetAudioStreamGain(Stream, gain);
		}

	private:
		enum {CHUNK_FRAMES = 1024};

		/*
		** SDL wants 'additional_amount' more bytes. Mix them in chunks.
		*/
		static void SDLCALL Stream_Callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int)
		{
			SDL3AudioBackendClass *self = (SDL3AudioBackendClass *)userdata;
			int const frame_bytes = (int)sizeof(short) * self->Channels;
			int frames = (additional_amount + frame_bytes - 1) / frame_bytes;
			short buffer[CHUNK_FRAMES * 2];

			while (frames > 0) {
				int chunk = (frames < CHUNK_FRAMES) ? frames : CHUNK_FRAMES;
				if (self->Callback) {
					self->Callback(self->User, buffer, chunk);
				} else {
					SDL_memset(buffer, 0, (size_t)chunk * (size_t)frame_bytes);
				}
				SDL_PutAudioStreamData(stream, buffer, chunk * frame_bytes);
				frames -= chunk;
			}
		}

		SDL_AudioStream *Stream;
		AudioMixCallback Callback;
		void *User;
		int Channels;
		bool SubsystemUp;
};

}	// namespace


AudioBackendClass *Audio_SDL3_Backend(void)
{
	static SDL3AudioBackendClass backend;
	return(&backend);
}

#endif	// RA_HAVE_SDL3
