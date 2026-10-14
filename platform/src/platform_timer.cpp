/*
**	Command & Conquer Red Alert(tm) - multiplayer port
**	Platform layer: time (session S08). std::chrono only, no SDL.
**
**	The original 60 Hz timer (WIN32LIB/TIMER/TIMERINI.CPP) was a Windows multimedia
**	timer: timeSetEvent(1000/freq) incremented a tick counter from a callback. For
**	freq = 60 that is a 16 ms period (integer division), i.e. 62.5 ticks per real
**	second. We do not run a thread: the tick count is computed from a steady clock
**	when it is read, with the same 16 ms period, so game speed matches the original.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/
#include "../platform.h"

#include <chrono>
#include <thread>

namespace {

std::chrono::steady_clock::time_point Timer_Base(void)
{
	static const std::chrono::steady_clock::time_point base = std::chrono::steady_clock::now();
	return base;
}

} // namespace


uint64_t Platform_Milliseconds64(void)
{
	std::chrono::steady_clock::duration elapsed = std::chrono::steady_clock::now() - Timer_Base();
	return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}


uint32_t Platform_Milliseconds(void)
{
	return (uint32_t)Platform_Milliseconds64();
}


void Platform_Sleep(int milliseconds)
{
	if (milliseconds > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	} else {
		std::this_thread::yield();
	}
}


uint32_t Platform_Tick_Period_Ms(uint32_t frequency)
{
	if (frequency == 0 || frequency > 1000) {
		return 1;
	}
	return 1000 / frequency;
}


uint32_t Platform_Ticks_In(uint64_t elapsed_ms, uint32_t frequency)
{
	/* Truncated to 32 bits like the original unsigned tick counter. */
	return (uint32_t)(elapsed_ms / Platform_Tick_Period_Ms(frequency));
}
