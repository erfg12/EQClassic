//o--------------------------------------------------------------
//| nodes.h; Yeahlight, July 17, 2008
//o--------------------------------------------------------------
//| All the classes for each type of node available
//o--------------------------------------------------------------

#ifndef NODES_H
#define NODES_H

//#pragma once
#include "zone.h"
#include "map.h"

class Child
{
public:
	int16 nodeID;
	float cost;
};

class Node
{
public:
	Node();
	~Node();
	void AddChild(Child tempChild);
	Node* Clone();

	int16 nodeID;
	float x;
	float y;
	float z;
	float fValue;
	Child myChildren[MAXZONENODESCHILDREN];
	int16 childCount;
	float pathCost;
	Node* myParent;
	Node* next;
	int16 distanceFromCenter;
};

class ZoneLineNode
{
public:
	int16	id;
	float	x;
	float	y;
	float	z;
	char	target_zone[16];
	float	target_x;
	float	target_y;
	float	target_z;
	int16	range;
	int8	heading;
	int16	maxZDiff;
	int8	keepX;
	int8	keepY;
	int8	keepZ;
};

#endif