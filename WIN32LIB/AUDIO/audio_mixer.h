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
** Portable software mixer (session S09). Replaces the DirectSound secondary
** buffers and DirectSound's own mixing.
**
** - A fixed number of voices. Each voice plays PCM in its own format
**   (any rate, 8-bit unsigned or 16-bit signed, mono or stereo) and is
**   resampled (linear interpolation) to the output rate.
** - Voices PULL their data: when a voice's staging buffer is empty, the mixer
**   calls the fill callback for up to STAGE_BYTES bytes. A return of 0 means
**   the voice's data is finished; the voice stops after playing what it has.
**   (SOUNDIO.CPP's fill callback decodes AUD data with Sample_Copy, exactly
**   like the old DirectSound code filled a quarter of a secondary buffer.)
** - Per-voice gain and a master gain, as 16.16 fixed point (65536 = 1.0).
** - A tick callback at a fixed rate in OUTPUT time (40 Hz in the game, the
**   rate of the old maintenance timer), used for fades.
** - Output is signed 16-bit, 1 or 2 channels, clipped.
**
** Threading: one recursive lock. Mix() holds it, so the fill and tick
** callbacks run with it held. Game-side code takes it with Lock()/Unlock()
** (or AudioLockClass) around anything that touches state the callbacks use.
**
** Integer-only processing, so the output is the same on every platform.
** Audio never feeds back into the game simulation.
*/
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <mutex>

class AudioMixerClass {
	public:
		enum {
			MAX_VOICES = 8,
			STAGE_BYTES = 8192,		// = SECONDARY_BUFFER_SIZE/4, the old fill size.
			UNITY_GAIN = 65536
		};

		/*
		** Fill callback: write up to 'size' bytes of PCM for 'voice' to 'dest'.
		** Return the bytes written; 0 = no more data.
		*/
		typedef long (*FillFunc)(void *user, int voice, void *dest, long size);
		typedef void (*TickFunc)(void *user);

		AudioMixerClass(void);
		~AudioMixerClass(void);

		/*
		** Output format. Stops every voice. 'reverse' swaps left and right.
		*/
		void Configure(int rate, int channels, bool reverse = false);
		int Output_Rate(void) const {return(OutRate);}
		int Output_Channels(void) const {return(OutChannels);}

		void Set_Fill_Callback(FillFunc func, void *user);
		void Set_Tick_Callback(TickFunc func, void *user, int hz);

		/*
		** Start 'voice' (0..MAX_VOICES-1) with source data in the given format
		** (bits 8 = unsigned, 16 = signed little-endian). Any sound already on
		** the voice is dropped. No data is fetched until the next Mix().
		*/
		bool Start_Voice(int voice, int rate, int channels, int bits, int gain);
		void Stop_Voice(int voice);
		bool Is_Voice_Playing(int voice) const;
		void Set_Voice_Gain(int voice, int gain);
		int Voice_Gain(int voice) const;

		void Set_Master_Gain(int gain);

		/*
		** Mix 'frames' output frames into 'out' (frames * Output_Channels()
		** samples). Silent voices still produce zeros.
		*/
		void Mix(short *out, int frames);

		/*
		** Total output frames mixed since Configure().
		*/
		unsigned long Frames_Mixed(void) const {return(FramesMixed);}

		void Lock(void) const {Mutex->lock();}
		void Unlock(void) const {Mutex->unlock();}

	private:
		AudioMixerClass(AudioMixerClass const &);
		AudioMixerClass &operator=(AudioMixerClass const &);

		struct Voice {
			bool Active;			// Playing (or about to).
			bool Primed;			// Cur/Next loaded.
			bool SourceEnded;		// Fill returned 0; Next is a copy of Cur.
			int Rate;
			int Channels;
			int Bits;
			int FrameBytes;
			int Gain;
			unsigned long Step;		// Source frames per output frame, 16.16.
			unsigned long Frac;		// Position between Cur and Next, 16.16.
			int Cur[2];
			int Next[2];
			long StageLen;
			long StagePos;
			unsigned char Stage[STAGE_BYTES];
		};

		bool Fetch_Frame(int index, Voice &v, int frame[2]);
		void Mix_Voice(int index, Voice &v, int *acc, int frames);

		/*
		** Heap-allocated on purpose: the build packs structs to 1 byte (Watcom
		** /zp1, RA_PACK_STRUCTS), which would let a member mutex sit at an odd
		** address; futexes and interlocked operations need it aligned. 'new'
		** always returns suitably aligned memory.
		*/
		std::recursive_mutex *Mutex;
		int OutRate;
		int OutChannels;
		bool Reverse;
		int MasterGain;
		FillFunc Fill;
		void *FillUser;
		TickFunc Tick;
		void *TickUser;
		int TickHz;
		long FramesToTick;
		unsigned long FramesMixed;
		Voice Voices[MAX_VOICES];
};


/*
** Scoped lock for a mixer.
*/
class AudioLockClass {
	public:
		explicit AudioLockClass(AudioMixerClass const &mixer) : Mixer(mixer) {Mixer.Lock();}
		~AudioLockClass(void) {Mixer.Unlock();}
	private:
		AudioLockClass(AudioLockClass const &);
		AudioLockClass &operator=(AudioLockClass const &);
		AudioMixerClass const &Mixer;
};

#endif
