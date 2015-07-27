//o--------------------------------------------------------------
//| Projectile.h; Yeahlight, August 19, 2009
//o--------------------------------------------------------------
//| Projectile class header file
//o--------------------------------------------------------------

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "zone.h"
#include "entity.h"

#define PI 3.14159265358979323846
#define DEG_CIRCLE 360
#define DEG_TO_RAD (PI / (DEG_CIRCLE / 2))
#define RAD_TO_DEG ((DEG_CIRCLE / 2) / PI)

class Projectile: public Entity
{
public:
	Projectile();
	~Projectile();

	bool	IsProjectile() { return true; }
	bool	Process();

	void	BuildCollisionList(Mob* target, float range);
	void	BuildProjectile(Mob* sender);
	void	BuildProjectileTracer();
	bool	UpdatePosition();
	bool	ScanArea();
	double	DegreesToRadians(double degrees) { return degrees * DEG_TO_RAD; }
	double	RadiansToDegrees(double radians) { return radians * RAD_TO_DEG; }

	float	Square(float number);
	float	FindDistance(float x, float y, float x2, float y2);
	void	SetCoordinateMultipliers(double degrees);

	float	GetX() { return x; }
	float	GetY() { return y; }
	float	GetZ() { return z; }
	float	GetVelocity() { return velocity; }
	float	GetHeading() { return heading; }
	float	GetYaw() { return yaw; }

	int8	GetLightSource() { return lightSource; }
	int32	GetItemID() { return itemID; }
	int32	GetSpellID() { return spellID; }

	Timer*	GetUpdateTimer() { return update_timer; }
	Timer*	GetDespawnTimer() { return despawn_timer; }

	float	GetType() { return type; }
	float	GetTargetID() { return targetID; }
	float	GetSourceID() { return sourceID; }
	float	GetTilt() { return tilt; }
	float	GetPitch() { return pitch; }

	char*	GetTexture() { return texture; }
	char*	GetName() { return 0; }

	void	SetX(float in) { x = in; }
	void	SetY(float in) { y = in; }
	void	SetZ(float in) { z = in; }
	void	SetVelocity(float in) { velocity = in; }
	void	SetHeading(float in) { heading = in; }
	void	SetItemID(int32 in) { itemID = in; }
	void	SetSpellID(int32 in) { spellID = in; }
	void	SetType(int8 in) { type = in; }
	void	SetTargetID(int32 in) { targetID = in; }
	void	SetSourceID(int32 in) { sourceID = in; }
	void	SetTexture(char* in) { strcpy(texture, in); }
	void	SetLightSource(int8 in) { lightSource = in; }
	void	SetYaw(float in) { yaw = in; }

protected:
	float x;
	float y;
	float z;
	float velocity;
	float heading;
	float tilt;
	float pitch;
	float unitsPerUpdate;
	float yaw;

	double xMultiplier;
	double yMultiplier;

	char texture[16];

	int8  type;
	int8  lightSource;
	int32 itemID;
	int32 spellID;
	int32 targetID;
	int32 sourceID;
	int32 accelerationCounter;

	int32 collisionList[150];
	int32 collisionListCounter;

	Timer* scan_timer;
	Timer* update_timer;
	Timer* despawn_timer;

	bool roamer;
};

#endif