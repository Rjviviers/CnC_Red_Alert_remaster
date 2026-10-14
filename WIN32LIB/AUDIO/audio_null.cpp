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
** Null audio backend and backend selection (session S09). See audio_backend.h.
*/
#include "audio_backend.h"

#include <chrono>
#include <string.h>

namespace {

enum {
	PUMP_CHUNK = 512,					// Frames per mix call.
	MAX_CATCH_UP_MS = 250			// Longest gap Update() makes up for.
};

double Now_Seconds(void)
{
	using namespace std::chrono;
	return duration_cast<duration<double> >(steady_clock::now().time_since_epoch()).count();
}

AudioBackendClass *SelectedBackend = 0;

}	// namespace


NullAudioBackendClass::NullAudioBackendClass(void) :
	IsOpen(false),
	IsPaused(true),
	RealTime(true),
	Callback(0),
	User(0),
	LastTime(0.0),
	Carry(0.0)
{
	Format.Rate = 22050;
	Format.Channels = 1;
}


NullAudioBackendClass::~NullAudioBackendClass(void)
{
	Close();
}


char const *NullAudioBackendClass::Name(void) const
{
	return("null");
}


bool NullAudioBackendClass::Open(AudioDeviceFormat const &want, AudioMixCallback callback, void *user)
{
	Format = want;
	if (Format.Rate <= 0) Format.Rate = 22050;
	if (Format.Channels != 2) Format.Channels = 1;
	Callback = callback;
	User = user;
	IsOpen = true;
	IsPaused = true;
	LastTime = Now_Seconds();
	Carry = 0.0;
	return(true);
}


void NullAudioBackendClass::Close(void)
{
	IsOpen = false;
	IsPaused = true;
	Callback = 0;
	User = 0;
}


void NullAudioBackendClass::Pause(bool paused)
{
	if (IsPaused && !paused) {
		LastTime = Now_Seconds();		// Don't make up for time spent paused.
		Carry = 0.0;
	}
	IsPaused = paused;
}


void NullAudioBackendClass::Set_Gain(float)
{
}


/*
** Advance by the wall-clock time since the last call. Long gaps (loading
** screens, a debugger) are capped so a stalled game doesn't burn through a
** streaming score at once; samples then just finish a little later.
*/
void NullAudioBackendClass::Update(void)
{
	if (!IsOpen || IsPaused || !RealTime) return;

	double now = Now_Seconds();
	double elapsed = now - LastTime;
	LastTime = now;
	if (elapsed < 0.0) elapsed = 0.0;
	if (elapsed > MAX_CATCH_UP_MS / 1000.0) elapsed = MAX_CATCH_UP_MS / 1000.0;

	double frames = elapsed * Format.Rate + Carry;
	int whole = (int)frames;
	Carry = frames - whole;
	Pump(whole);
}


int NullAudioBackendClass::Pump(int frames, short *out)
{
	if (frames <= 0) return(0);
	if (!IsOpen || IsPaused || !Callback) {
		if (out) memset(out, 0, sizeof(short) * (size_t)frames * (size_t)Format.Channels);
		return(0);
	}

	short scratch[PUMP_CHUNK * 2];
	int done = 0;
	while (done < frames) {
		int chunk = frames - done;
		if (chunk > PUMP_CHUNK) chunk = PUMP_CHUNK;
		short *dest = out ? out + done * Format.Channels : scratch;
		Callback(User, dest, chunk);
		done += chunk;
	}
	return(done);
}


void Audio_Set_Backend(AudioBackendClass *backend)
{
	SelectedBackend = backend;
}


AudioBackendClass *Audio_Get_Backend(void)
{
	return(SelectedBackend ? SelectedBackend : Audio_Default_Backend());
}


AudioBackendClass *Audio_Default_Backend(void)
{
#ifdef RA_HAVE_SDL3
	return(Audio_SDL3_Backend());
#else
	static NullAudioBackendClass null_backend;
	return(&null_backend);
#endif
}
