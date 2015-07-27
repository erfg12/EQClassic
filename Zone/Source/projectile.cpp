//o--------------------------------------------------------------
//| Projectile.cpp; Yeahlight, August 19, 2009
//o--------------------------------------------------------------
//| Projectile class member functions
//o--------------------------------------------------------------

#include <cmath>
#include "zone.h"
#include "projectile.h"
#include "EntityList.h"
#include "SpellsHandler.hpp"
#include "config.h"
#include "moremath.h"

using namespace std;

extern EntityList entity_list;
extern SpellsHandler spells_handler;
extern Zone* zone;

//o--------------------------------------------------------------
//| Projectile; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Projectile's constructor
//o--------------------------------------------------------------
Projectile::Projectile()
{
	x = 0;
	y = 0;
	z = 0;
	velocity = 0;
	heading = 0;
	tilt = 0;
	pitch = 0;
	yaw = 0;
	type = 0;
	itemID = 0;
	spellID = 0;
	targetID = 0;
	sourceID = 0;
	strcpy(texture, "");
	memset(collisionList, 0, sizeof(collisionList));
	collisionListCounter = 0;
	accelerationCounter = 0;
	xMultiplier = 0;
	yMultiplier = 0;
	roamer = false;
	unitsPerUpdate = 5.5;
	scan_timer = new Timer(300);
	update_timer = new Timer(150);
	despawn_timer = new Timer(30000);
}

//o--------------------------------------------------------------
//| ~Projectile; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Projectile's destructor
//o--------------------------------------------------------------
Projectile::~Projectile()
{
	safe_delete(scan_timer);
	safe_delete(update_timer);
	safe_delete(despawn_timer);
}

//o--------------------------------------------------------------
//| Process; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Projectile's process function
//o--------------------------------------------------------------
bool Projectile::Process()
{
	//Yeahlight: Time for this projectile to vanish
	if(despawn_timer->Check())
	{
		return false;
	}

	//Yeahlight: Time to update the projectile's position
	if(update_timer->Check())
	{
		//BuildProjectileTracer();
		//Yeahlight: Update the projectile's position
		if(UpdatePosition())
		{
			return false;
		}
	}

	//Yeahlight: Time for this projectile to scan its surroundings for a collision
	if(scan_timer->Check())
	{
		//Yeahlight: Update the projectile's acceleration
		if(accelerationCounter++ < 8)
			unitsPerUpdate *= 1.0525;

		//Yeahlight: Non-roaming projectile collided with an entity
		if(roamer == false && ScanArea())
		{
			return false;
		}
	}
	return true;
}

//o--------------------------------------------------------------
//| BuildCollisionList; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Builds the expected collision list for the projectile
//o--------------------------------------------------------------
void Projectile::BuildCollisionList(Mob* target, float range)
{
	//Yeahlight: A target has been supplied
	if(target && target->IsMob())
	{
		entity_list.GetSurroundingEntities(GetX(), GetY(), GetZ(), range, collisionList, collisionListCounter);
		collisionList[0] = target->GetID();
	}
	//Yeahlight: No target has been supplied
	else
	{
		//Yeahlight: Generate the vector for this projectile
		SetCoordinateMultipliers(GetHeading() / 2);
		entity_list.GetEntitiesAlongVector((float)(yMultiplier/xMultiplier), GetX(), GetY(), GetZ(), range, collisionList, collisionListCounter);
	}
}

//o--------------------------------------------------------------
//| BuildProjectile; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Spawns the projectile based on its attributes
//o--------------------------------------------------------------
void Projectile::BuildProjectile(Mob* sender)
{
	APPLAYER* outapp = new APPLAYER(OP_SpawnProjectile, sizeof(SpawnProjectile_Struct));
	SpawnProjectile_Struct* sp = (SpawnProjectile_Struct*)outapp->pBuffer;
	memset(outapp->pBuffer, 0x00, sizeof(SpawnProjectile_Struct));
	strcpy(sp->texture, GetTexture());
	sp->sourceID = GetSourceID();
	//Yeahlight: The projectile has a target to follow
	if(GetTargetID())
		sp->targetID = GetTargetID();
	else
		roamer = true;
	sp->tilt = GetTilt();
	sp->pitch = GetPitch();
	sp->velocity = GetVelocity();
	sp->x = GetX();
	sp->y = GetY();
	sp->z = GetZ();
	sp->lightSource = GetLightSource();
	sp->heading = GetHeading();
	sp->projectileType = GetType();
	sp->spellID = GetSpellID();
	sp->yaw = GetYaw();
	//Yeahlight: The projectile is a spell bolt
	if(GetSpellID())
		entity_list.QueueClients(sender, outapp, false);
	else
		entity_list.QueueCloseClients(sender, outapp, false, 350);
	safe_delete(outapp);
}

//o--------------------------------------------------------------
//| BuildProjectileTracer; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Spawns the projectile's tracer for debugging
//o--------------------------------------------------------------
void Projectile::BuildProjectileTracer()
{
	APPLAYER* outapp = new APPLAYER(OP_SpawnProjectile, sizeof(SpawnProjectile_Struct));
	SpawnProjectile_Struct* sp = (SpawnProjectile_Struct*)outapp->pBuffer;
	memset(outapp->pBuffer, 0x00, sizeof(SpawnProjectile_Struct));
	strcpy(sp->texture, GetTexture());
	sp->sourceID = GetSourceID();
	sp->x = GetX();
	sp->y = GetY();
	sp->z = GetZ();
	sp->heading = GetHeading();
	sp->projectileType = GetType();
	entity_list.QueueCloseClients(NULL, outapp, false, 500);
	safe_delete(outapp);
}

//o--------------------------------------------------------------
//| UpdatePosition; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Updates the projectiles server-side position
//o--------------------------------------------------------------
bool Projectile::UpdatePosition()
{
	float x2 = 0;
	float y2 = 0;
	float d = unitsPerUpdate;

	//Yeahlight: Projectile is not a roaming projectile and has a target in mind
	if(roamer == false)
	{
		Mob* target = entity_list.GetMob(GetTargetID());
		//Yeahlight: Legitimate target has been located
		if(target && target->IsMob())
		{
			//Yeahlight: We ignore the z-axis on the primary target
			float distance = target->DistanceToLocation(GetX(), GetY());
			//Yeahlight: Projectile is in range of the target
			if(distance <= PROJECTILE_COLLISION_RANGE)
			{
				Mob* inflictor = entity_list.GetMob(GetSourceID());
				//Yeahlight: Inflictor of the projectile is not the target
				if(target && inflictor != target)
				{
					//Yeahlight: Projectile is a spell
					if(GetSpellID())
					{
						Spell* spell = spells_handler.GetSpellPtr(GetSpellID());
						//Yeahlight: Spell exists and is a valid type of spell
						if(spell && spell->IsValidSpell() && spell->GetSpellTargetType() == ST_LineOfSight)
						{
							//Yeahlight: Switch based on the spell's ID
							switch(GetSpellID())
							{
								//Yeahlight: Firework1
								case 30:
								//Yeahlight: Firework2
								case 119:
								//Yeahlight: Firework3
								case 206:
								//Yeahlight: Firework4
								case 214:
								//Yeahlight: Flare
								case 310:
								{
									target->SpellOnTarget(spell, target);
									break;
								}
								default:
								{
									inflictor->SpellOnTarget(spell, target);
									break;
								}
							}
						}
					}
				}
				return true;
			}
			//Yeahlight: Set the projectile's destination coordinates
			x2 = target->GetX();
			y2 = target->GetY();	
		}
	}
	else
	{
		//Yeahlight: Set the roaming projectile's destination coordinates
		x2 = GetX() + (float)xMultiplier * 50;
		y2 = GetY() + (float)yMultiplier * 50;
		//Yeahlight: Scan the roaming projectile's surrounding environment
		if(ScanArea())
			return true;
	}

	//Yeahlight: Projectile math
	float x1 = GetX();
	float y1 = GetY();
	float delta_y = y1 - y2;
	float delta_x =	x1 - x2;
	float new_x = 0;
	float new_y = 0;
	float new_x2 = 0;
	float new_y2 = 0;

	if(delta_y == 0)
	{
		if(x1 < x2)
		{
			new_x = x1 + d;
		}
		else
		{
			new_x = x1 - d;
		}
		new_y = y1;
	}
	else if(delta_x == 0)
	{
		if(y1 < y2)
		{
			new_y = y1 + d;
		}
		else
		{
			new_y = y1 - d;
		}
		new_x = x1;
	}
	else
	{
		float m = delta_y / delta_x;
		float radical = sqrt(Square(d) / (1 + Square(m)));
		new_x = x1 + radical;
		new_x2 = x1 - radical;
		new_y = m *new_x + (y1 - m * x1);
		new_y2 = m * new_x2 + (y1 - m * x1);
		float d1 = FindDistance(new_x,  new_y,  x2, y2);
		float d2 = FindDistance(new_x2, new_y2, x2, y2);				
		if((d1 < d2) == false)
		{
			new_x = new_x2;
			new_y = new_y2;
		}
	}

	//Yeahlight: Set the new projectile coordinates
	SetX(new_x);
	SetY(new_y);

	//Yeahlight: Check to see if the projectile has broken a plane
	if(CheckCoordLos(new_x, new_y, GetZ(), x1, y1, GetZ()) == false)
		return true;

	return false;
}

//o--------------------------------------------------------------
//| ScanArea; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Scans the area around the projectile for entities
//o--------------------------------------------------------------
bool Projectile::ScanArea()
{
	Mob* inflictor = entity_list.GetMob(GetSourceID());
	//Yeahlight: The inflictor of the projectile exists
	if(inflictor && inflictor->IsMob())
	{
		//Yeahlight: Iterate through the expected collision list
		for(int i = 0; i < collisionListCounter; i++)
		{
			Mob* target = entity_list.GetMob(collisionList[i]);
			//Yeahlight: Target is within range of the projectile (the dummy X and Y checks are done for optimization purposes)
			if(target && inflictor != target && abs(target->GetX() - GetX()) <= PROJECTILE_COLLISION_RANGE && abs(target->GetY() - GetY()) <= PROJECTILE_COLLISION_RANGE && abs(target->GetZ() - GetZ()) <= PROJECTILE_COLLISION_RANGE && target->DistanceToLocation(GetX(), GetY()) <= PROJECTILE_COLLISION_RANGE)
			{
				//Yeahlight: The projectile is a spell bolt
				if(GetSpellID())
				{
					Spell* spell = spells_handler.GetSpellPtr(GetSpellID());
					//Yeahlight: The spell exists and is a value spell type
					if(spell && spell->IsValidSpell() && spell->GetSpellTargetType() == ST_LineOfSight)
					{
						//Yeahlight: Switch based on the spell's ID
						switch(GetSpellID())
						{
							//Yeahlight: Firework1
							case 30:
							//Yeahlight: Firework2
							case 119:
							//Yeahlight: Firework3
							case 206:
							//Yeahlight: Firework4
							case 214:
							//Yeahlight: Flare
							case 310:
							{
								target->SpellOnTarget(spell, target);
								break;
							}
							default:
							{
								inflictor->SpellOnTarget(spell, target);
								break;
							}
						}
					}
				}
				return true;
			}
		}
	}
	//Yeahlight: Inflictor of the projectile is no where to be found
	else
	{
		return true;
	}

	return false;
}

//o--------------------------------------------------------------
//| Square; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Returns the square product of a number
//o--------------------------------------------------------------
float Projectile::Square(float number)
{
	return number * number;
}

//o--------------------------------------------------------------
//| FindDistance; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Returns the 2D distance between two points in space
//o--------------------------------------------------------------
float Projectile::FindDistance(float x1, float y1, float x2, float y2)
{
	return sqrt(Square(x2 - x1) + Square(y2 - y1));
}

//o--------------------------------------------------------------
//| SetCoordinateMultipliers; Yeahlight, August 21, 2009
//o--------------------------------------------------------------
//| Generates the X and Y distance multipliers
//o--------------------------------------------------------------
void Projectile::SetCoordinateMultipliers(double degrees)
{
	degrees = (degrees / (float)(256.000 / 360.000));
	xMultiplier = sin(DegreesToRadians(degrees));
	yMultiplier = cos(DegreesToRadians(degrees));
	//Yeahlight: Avoiding "division by zero" during future slope calculations
	if(xMultiplier == 0)
		xMultiplier = 0.000001;
}