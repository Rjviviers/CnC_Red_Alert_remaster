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
** Audio output backend interface (session S09). Replaces DirectSound.
**
** The game's sound API (audio.h: Play_Sample, File_Stream_Sample_Vol, ...)
** is implemented in SOUNDIO.CPP on top of a software mixer (audio_mixer.h).
** A backend only moves the mixer's output to a device: the backend calls the
** mix callback whenever it needs more PCM (signed 16-bit, interleaved).
**
** Backends:
**   - null (audio_null.cpp, always built): no device. Output is discarded,
**     but time advances from Update() (called by Sound_Callback), so samples
**     start and finish like they would on real hardware. Lets the game run
**     with audio disabled, and lets tests drive the mixer with Pump().
**   - SDL3 (audio_sdl3.cpp, only with RA_HAVE_SDL3): an SDL audio stream
**     whose callback pulls from the mixer on SDL's audio thread.
**
** Threading: Mix callbacks may run on another thread. The mixer serializes
** them with the game-side API through its own lock (see audio_mixer.h).
**
** See docs/codebase/audio.md.
*/
#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

/*
** Output format: signed 16-bit native-endian PCM, 'channels' interleaved.
*/
struct AudioDeviceFormat {
	int Rate;			// Frames per second.
	int Channels;		// 1 or 2.
};

/*
** Called by the backend to fill 'frames' frames (frames*channels samples).
*/
typedef void (*AudioMixCallback)(void *user, short *out, int frames);


class AudioBackendClass {
	public:
		virtual ~AudioBackendClass(void) {}

		/*
		** Short name for logs ("null", "sdl3").
		*/
		virtual char const *Name(void) const = 0;

		/*
		** Open the device. 'want' is the format the mixer will produce; the
		** backend must accept it (converting if the device differs).
		** The device starts paused; call Pause(false) to start pulling.
		** Returns false if no device could be opened.
		*/
		virtual bool Open(AudioDeviceFormat const &want, AudioMixCallback callback, void *user) = 0;
		virtual void Close(void) = 0;

		/*
		** Stop / resume pulling from the mixer (focus loss, Stop_Primary_Sound_Buffer).
		*/
		virtual void Pause(bool paused) = 0;

		/*
		** Device-level gain, 0.0 (silent) to 1.0 (unchanged).
		*/
		virtual void Set_Gain(float gain) = 0;

		/*
		** Called from the game thread on every Sound_Callback(). Backends with
		** their own audio thread ignore it; the null backend advances time here.
		*/
		virtual void Update(void) {}
};


/*
** The null backend. Pump() is public so tests can drive the mixer by hand.
*/
class NullAudioBackendClass : public AudioBackendClass {
	public:
		NullAudioBackendClass(void);
		virtual ~NullAudioBackendClass(void);

		virtual char const *Name(void) const;
		virtual bool Open(AudioDeviceFormat const &want, AudioMixCallback callback, void *user);
		virtual void Close(void);
		virtual void Pause(bool paused);
		virtual void Set_Gain(float gain);
		virtual void Update(void);

		/*
		** Mix 'frames' frames now. If 'out' is null the output is discarded;
		** otherwise it must hold frames*channels samples. Does nothing (and
		** clears 'out') while closed or paused. Returns the frames mixed.
		*/
		int Pump(int frames, short *out = 0);

		/*
		** When false, Update() does nothing, so only Pump() advances time.
		** Tests use this to stay independent of the wall clock. Default true.
		*/
		void Set_Real_Time(bool on) {RealTime = on;}

		bool Is_Open(void) const {return(IsOpen);}
		bool Is_Paused(void) const {return(IsPaused);}

	private:
		bool IsOpen;
		bool IsPaused;
		bool RealTime;
		AudioDeviceFormat Format;
		AudioMixCallback Callback;
		void *User;
		double LastTime;		// Seconds, from a monotonic clock.
		double Carry;			// Fraction of a frame not yet mixed.
};


/*
** Backend selection. Call Audio_Set_Backend() before Audio_Init() to choose
** one; the caller keeps ownership. Without a call, Audio_Init() uses
** Audio_Default_Backend(): SDL3 when built with RA_HAVE_SDL3, else null.
*/
void Audio_Set_Backend(AudioBackendClass *backend);
AudioBackendClass *Audio_Get_Backend(void);
AudioBackendClass *Audio_Default_Backend(void);

#ifdef RA_HAVE_SDL3
AudioBackendClass *Audio_SDL3_Backend(void);
#endif

#endif
