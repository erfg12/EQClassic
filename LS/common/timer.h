/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef TIMER_H
#define TIMER_H

#include "types.h"

// Disgrace: for windows compile
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	int gettimeofday (timeval *tp, ...);
#endif

class Timer
{
public:
	Timer(int32 timer_time, bool iUseAcurateTiming = false);
	~Timer() { }

	bool Check(bool iReset = true);
	void Enable();
	void Disable();
	void Start(int32 set_timer_time=0, bool ChangeResetTimer = true);
	void SetTimer(int32 set_timer_time=0);
	int32 GetRemainingTime();
	inline const int32& GetTimerTime()		{ return timer_time; }
	inline const int32& GetSetAtTrigger()	{ return set_at_trigger; }
	void Trigger();
	void SetAtTrigger(int32 set_at_trigger, bool iEnableIfDisabled = false);

	inline bool Enabled() { return enabled; }

	static const int32& SetCurrentTime();
	static const int32& GetCurrentTime();

private:
	int32	start_time;
	int32	timer_time;
	bool	enabled;
	int32	set_at_trigger;

	// Tells the timer to be more acurate about happening every X ms.
	// Instead of Check() setting the start_time = now,
	// it it sets it to start_time += timer_time
	bool	pUseAcurateTiming;

//	static int32 current_time;
//	static int32 last_time;
};

#endif
