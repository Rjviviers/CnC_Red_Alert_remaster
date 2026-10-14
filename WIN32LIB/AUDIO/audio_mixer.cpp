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
** Portable software mixer (session S09). See audio_mixer.h.
*/
#include "audio_mixer.h"

#include <string.h>

namespace {

enum {
	CHUNK_FRAMES = 256		// Frames mixed per pass (size of the accumulator).
};

inline int Clamp_Gain(int gain)
{
	return (gain < 0) ? 0 : gain;
}

inline short Clip16(long long value)
{
	if (value > 32767) return 32767;
	if (value < -32768) return -32768;
	return (short)value;
}

}	// namespace


AudioMixerClass::AudioMixerClass(void) :
	Mutex(new std::recursive_mutex),
	OutRate(22050),
	OutChannels(1),
	Reverse(false),
	MasterGain(UNITY_GAIN),
	Fill(0),
	FillUser(0),
	Tick(0),
	TickUser(0),
	TickHz(0),
	FramesToTick(0),
	FramesMixed(0)
{
	memset(Voices, 0, sizeof(Voices));
}


AudioMixerClass::~AudioMixerClass(void)
{
	delete Mutex;
}


void AudioMixerClass::Configure(int rate, int channels, bool reverse)
{
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	OutRate = (rate > 0) ? rate : 22050;
	OutChannels = (channels == 2) ? 2 : 1;
	Reverse = reverse;
	FramesMixed = 0;
	FramesToTick = (TickHz > 0) ? OutRate / TickHz : 0;
	for (int i = 0; i < MAX_VOICES; i++) {
		Voices[i].Active = false;
	}
}


void AudioMixerClass::Set_Fill_Callback(FillFunc func, void *user)
{
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	Fill = func;
	FillUser = user;
}


/*
** The tick fires every OutRate/hz output frames (rounded down, at least 1).
*/
void AudioMixerClass::Set_Tick_Callback(TickFunc func, void *user, int hz)
{
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	Tick = func;
	TickUser = user;
	TickHz = (func && hz > 0) ? hz : 0;
	FramesToTick = TickHz ? OutRate / TickHz : 0;
	if (TickHz && FramesToTick < 1) FramesToTick = 1;
}


bool AudioMixerClass::Start_Voice(int voice, int rate, int channels, int bits, int gain)
{
	if ((unsigned)voice >= MAX_VOICES || rate <= 0) return(false);
	if (channels != 1 && channels != 2) return(false);
	if (bits != 8 && bits != 16) return(false);

	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	Voice &v = Voices[voice];
	v.Active = true;
	v.Primed = false;
	v.SourceEnded = false;
	v.Rate = rate;
	v.Channels = channels;
	v.Bits = bits;
	v.FrameBytes = channels * (bits / 8);
	v.Gain = Clamp_Gain(gain);
	v.Step = (unsigned long)(((unsigned long long)rate << 16) / (unsigned long long)OutRate);
	if (v.Step == 0) v.Step = 1;
	v.Frac = 0;
	v.Cur[0] = v.Cur[1] = 0;
	v.Next[0] = v.Next[1] = 0;
	v.StageLen = 0;
	v.StagePos = 0;
	return(true);
}


void AudioMixerClass::Stop_Voice(int voice)
{
	if ((unsigned)voice >= MAX_VOICES) return;
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	Voices[voice].Active = false;
}


bool AudioMixerClass::Is_Voice_Playing(int voice) const
{
	if ((unsigned)voice >= MAX_VOICES) return(false);
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	return(Voices[voice].Active);
}


void AudioMixerClass::Set_Voice_Gain(int voice, int gain)
{
	if ((unsigned)voice >= MAX_VOICES) return;
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	Voices[voice].Gain = Clamp_Gain(gain);
}


int AudioMixerClass::Voice_Gain(int voice) const
{
	if ((unsigned)voice >= MAX_VOICES) return(0);
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	return(Voices[voice].Gain);
}


void AudioMixerClass::Set_Master_Gain(int gain)
{
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	MasterGain = Clamp_Gain(gain);
}


/*
** Read the next source frame of a voice into frame[0..1] as signed 16-bit
** values (mono sources fill frame[0] only). Refills the staging buffer from
** the fill callback when it runs dry. Returns false at the end of the data.
*/
bool AudioMixerClass::Fetch_Frame(int index, Voice &v, int frame[2])
{
	while (v.StageLen - v.StagePos < v.FrameBytes) {
		if (!Fill) return(false);

		/*
		** Keep any partial frame, then ask for as much as fits.
		*/
		long leftover = v.StageLen - v.StagePos;
		if (leftover > 0) memmove(v.Stage, v.Stage + v.StagePos, (size_t)leftover);
		v.StagePos = 0;
		v.StageLen = leftover;
		long got = Fill(FillUser, index, v.Stage + leftover, STAGE_BYTES - leftover);
		if (got <= 0) return(false);
		if (got > STAGE_BYTES - leftover) got = STAGE_BYTES - leftover;
		v.StageLen += got;
	}

	unsigned char const *p = v.Stage + v.StagePos;
	for (int c = 0; c < v.Channels; c++) {
		if (v.Bits == 16) {
			frame[c] = (short)(unsigned short)(p[0] | (p[1] << 8));
			p += 2;
		} else {
			frame[c] = ((int)p[0] - 128) * 256;
			p += 1;
		}
	}
	if (v.Channels == 1) frame[1] = frame[0];
	v.StagePos += v.FrameBytes;
	return(true);
}


/*
** Add 'frames' output frames of one voice to the accumulator.
*/
void AudioMixerClass::Mix_Voice(int index, Voice &v, int *acc, int frames)
{
	if (!v.Primed) {
		if (!Fetch_Frame(index, v, v.Cur)) {
			v.Active = false;
			return;
		}
		if (!Fetch_Frame(index, v, v.Next)) {
			v.SourceEnded = true;
			v.Next[0] = v.Cur[0];
			v.Next[1] = v.Cur[1];
		}
		v.Primed = true;
	}

	for (int f = 0; f < frames; f++) {
		int s[2];
		for (int c = 0; c < 2; c++) {
			long long delta = (long long)(v.Next[c] - v.Cur[c]) * (long long)v.Frac;
			s[c] = v.Cur[c] + (int)(delta >> 16);
		}

		if (OutChannels == 1) {
			int mono = (v.Channels == 2) ? (s[0] + s[1]) / 2 : s[0];
			acc[f] += (int)(((long long)mono * v.Gain) >> 16);
		} else {
			int left = s[0];
			int right = s[1];
			if (Reverse) {
				int t = left;
				left = right;
				right = t;
			}
			acc[f * 2] += (int)(((long long)left * v.Gain) >> 16);
			acc[f * 2 + 1] += (int)(((long long)right * v.Gain) >> 16);
		}

		v.Frac += v.Step;
		while (v.Frac >= 0x10000UL) {
			v.Frac -= 0x10000UL;
			if (v.SourceEnded) {
				v.Active = false;
				return;
			}
			v.Cur[0] = v.Next[0];
			v.Cur[1] = v.Next[1];
			if (!Fetch_Frame(index, v, v.Next)) {
				v.SourceEnded = true;
				v.Next[0] = v.Cur[0];
				v.Next[1] = v.Cur[1];
			}
		}
	}
}


void AudioMixerClass::Mix(short *out, int frames)
{
	std::lock_guard<std::recursive_mutex> guard(*Mutex);
	int acc[CHUNK_FRAMES * 2];

	while (frames > 0) {
		int chunk = (frames < CHUNK_FRAMES) ? frames : CHUNK_FRAMES;
		if (TickHz && FramesToTick < chunk) chunk = (int)FramesToTick;

		memset(acc, 0, sizeof(int) * (size_t)chunk * (size_t)OutChannels);
		for (int i = 0; i < MAX_VOICES; i++) {
			if (Voices[i].Active) {
				Mix_Voice(i, Voices[i], acc, chunk);
			}
		}

		int samples = chunk * OutChannels;
		for (int i = 0; i < samples; i++) {
			out[i] = Clip16(((long long)acc[i] * MasterGain) >> 16);
		}
		out += samples;
		frames -= chunk;
		FramesMixed += (unsigned long)chunk;

		if (TickHz) {
			FramesToTick -= chunk;
			if (FramesToTick <= 0) {
				FramesToTick = OutRate / TickHz;
				if (FramesToTick < 1) FramesToTick = 1;
				Tick(TickUser);
			}
		}
	}
}
