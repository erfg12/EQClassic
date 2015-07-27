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
#ifndef SPAWN2_H
#define SPAWN2_H

#include "../common/timer.h"
class Spawn2
{
public:
	Spawn2(int32 spawn2_id, int32 spawngroup_id, float x, float y, float z, float heading, int32 respawn, int32 variance, int32 timeleft = 0, int16 grid = 0);
	~Spawn2();

	void	Enable();
	bool	Process();
	void	Reset();
	void	Depop();
	void	Repop(int32 delay = 0);

	int32	GetID()		{ return spawn2_id; }
	float	GetX()		{ return x; }
	float	GetY()		{ return y; }
	float	GetZ()		{ return z; }
	void	SetRespawnTimer(int32 newrespawntime) { respawn_ = newrespawntime; };
	int32	RespawnTimer() { return respawn_; }
	int32	SpawnGroupID() { return spawngroup_id_; }
	int32	CurrentNPCID() { return currentnpcid; }
protected:
	friend class Zone;
	int32	spawn2_id;
	Timer*	timer;
private:
	int32	resetTimer();

	int32	spawngroup_id_;
	int32	currentnpcid;
	float	x;
	float	y;
	float	z;
	float	heading;
	int32	respawn_;
	int32	variance_;
	int16	grid_;
};

#endif
