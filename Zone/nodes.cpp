//o--------------------------------------------------------------
//| nodes.cpp; Yeahlight, July 17, 2008
//o--------------------------------------------------------------
//| All the classes for each type of node available
//o--------------------------------------------------------------

#include "zone.h"
#include "map.h"
#include "nodes.h"

using namespace std;

Node::Node()
{
	nodeID = 0;
	x = 0;
	y = 0;
	z = 0;
	fValue = 0;

	for(int i = 0; i < MAXZONENODESCHILDREN; i++)
	{
		myChildren[i].nodeID = 0;
		myChildren[i].cost = 0;
	}
	childCount = 0;
	pathCost = 0;
	myParent = NULL;
	next = NULL;
	distanceFromCenter = 0;
}

Node::~Node()
{

}

void Node::AddChild(Child tempChild)
{
	myChildren[childCount] = tempChild;
	childCount++;
}

Node* Node::Clone()
{
	Node* tempNode = new Node();
	tempNode->nodeID = this->nodeID;
	tempNode->x = this->x;
	tempNode->y = this->y;
	tempNode->z = this->z;
	tempNode->fValue = this->fValue;
	
	for(int i = 0; i < MAXZONENODESCHILDREN; i++)
	{
		tempNode->myChildren[i] = this->myChildren[i];
	}

	tempNode->childCount = this->childCount;
	tempNode->pathCost = this->pathCost;
	tempNode->myParent = this->myParent;
	tempNode->next = this->next;
	tempNode->distanceFromCenter = this->distanceFromCenter;
	
	return tempNode;
}