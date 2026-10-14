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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Temp timer for 32bit lib                 *
 *                                                                         *
 *                    File Name : TIMERINI.CPP                             *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : July 6, 1994                             *
 *                                                                         *
 *                  Last Update : July 6, 1994   [SKB]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Init_Timer_System -- Initialize the WW timer system.                  *
 *   Remove_Timer_System -- Removes the timer system.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
** S08 (2026-10-14): the Windows multimedia timer (timeSetEvent + a callback thread
** incrementing SysTicks/UserTicks) is replaced by the platform layer's steady clock
** (platform/platform.h). Nothing runs on another thread any more: the tick count
** is brought up to date whenever it is read. The tick period is still
** 1000/freq milliseconds in integer arithmetic, exactly what timeSetEvent was
** given, so WinTimerClass(60) ticks every 16 ms (62.5 ticks per second) like the
** original. See docs/codebase/platform-layer.md, "Timer".
**
** The class layout (TIMER.H) is unchanged. Two members that the old code never
** used get a new job:
**    TrueRate  - platform millisecond count at the last update
**    UserRate  - milliseconds since then that did not yet make a whole tick
*/

#include <wwstd.h>
#include "timer.H"
#include "platform.h"

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Global Data /////////////////////////////////////

BOOL	TimerSystemOn	= FALSE;

// Global timers that the library or user can count on existing.
TimerClass					TickCount(BT_SYSTEM);
CountDownTimerClass		CountDown(BT_SYSTEM, 0);

HANDLE	TimerThreadHandle = 0;		//No timer thread any more; kept for TIMER.H
int		InTimerCallback	= 0;		//Never set any more; kept for TIMER.H

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Code ////////////////////////////////////////


/***************************************************************************
 * WinTimerClass::WinTimerClass -- Initialize the WW timer system.         *
 *                                                                         *
 * INPUT: UINT : user timer frequency.												  *
 *        BOOL : partial - if TRUE, do not become the global WindowsTimer.  *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/5/95 3:47PM : ST Created.                                          *
 *   2026-10-14 S08: platform clock instead of timeSetEvent.               *
 *=========================================================================*/
WinTimerClass::WinTimerClass (UINT freq, BOOL partial)
{
	Frequency	= freq ? freq : 60;
	SysTicks		= 0;
	UserTicks	= 0;
	TrueRate		= Platform_Milliseconds();
	UserRate		= 0;
	TimerHandle	= 1;					// "running"; there is no OS handle any more
	TimerSystemOn = TRUE;

	if (!partial) {
		WindowsTimer=this;
		TickCount.Start();
	}
}


/***************************************************************************
 * WinTimerClass::~WinTimerClass -- Removes the timer system.              *
 *=========================================================================*/
WinTimerClass::~WinTimerClass( void )
{
	TimerHandle = 0;
	TimerSystemOn = FALSE;
	if (WindowsTimer == this) {
		WindowsTimer = NULL;
	}
}


/***********************************************************************************************
 * WinTimerClass::Update_Tick_Count -- update westwood timers                                  *
 *                                                                                             *
 *    Adds the whole tick periods that have passed since the last update. The original was    *
 *    called once per tick from the multimedia-timer callback; now it is called on every read.*
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/5/95 3:58PM ST : Created                                                              *
 *    2026-10-14 S08: catch up from the platform clock.                                        *
 *=============================================================================================*/
void WinTimerClass::Update_Tick_Count ( void )
{
	unsigned now = Platform_Milliseconds();
	unsigned elapsed = (now - TrueRate) + UserRate;		// unsigned: safe across the 32-bit wrap
	unsigned period = Platform_Tick_Period_Ms(Frequency);
	unsigned ticks = elapsed / period;

	TrueRate = now;
	UserRate = elapsed % period;
	SysTicks += ticks;
	UserTicks += ticks;
}


/***********************************************************************************************
 * WinTimerClass::Get_System_Tick_Count -- returns the system tick count                       *
 *=============================================================================================*/
unsigned WinTimerClass::Get_System_Tick_Count ( void )
{
	Update_Tick_Count();
	return ( SysTicks );
}


/***********************************************************************************************
 * WinTimerClass::Get_User_Tick_Count -- returns the user tick count                           *
 *=============================================================================================*/
unsigned WinTimerClass::Get_User_Tick_Count ( void )
{
	Update_Tick_Count();
	return ( UserTicks );
}
