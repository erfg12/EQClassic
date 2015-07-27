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
//#include "../common/debug.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <iostream>
#include <fstream>

using namespace std;

//#include "../common/files.h"
//#include "zone_profile.h"
#include "moremath.h"
#include "../include/types.h"
#include "../include/zone.h"
#include "../../Utils/azone/map.h"
#ifdef WIN32
#define snprintf	_snprintf
#endif

//Do we believe the normals from the map file?
//you want this enabled if it dosent break things.
//#define TRUST_MAPFILE_NORMALS

//#define OPTIMIZE_QT_LOOKUPS

#define EPS 0.002f	//acceptable error

//#define DEBUG_SEEK 1
//#define DEBUG_BEST_Z 1

/*
 of note:
 it is possible to get a null node in a valid region if it
 does not have any triangles in it.
 this will dictate bahaviour on getting a null node
 TODO: listen to the above
 */

//quick functions to clean up vertex code.
#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))
#define Vmax3(o, a, b, c) ((a.o>b.o)? (a.o>c.o?a.o:c.o) : (b.o>c.o?b.o:c.o))

//*******************************************************************************************************************************************
//Yeahlight: IMPORTANT!! These two conditions should NEVER be set to the same value; one must be false and one must be true!*****************
bool createNewMapFile = false;	//Turn this flag on to create a <zone_name>.txt file*********************************************************
bool useNewMapFile = false;		//Turn this flag on to use the .txt file instead of the .map file, this must be OFF the create the .txt file!
//*******************************************************************************************************************************************

char fileNameSuffix[] = ".txt";
char fileNamePrefix[100] = "./Maps/Maps/";
char thisZonesName[100] = "";
ofstream _3dgraphfile;
ofstream myfile;
ifstream mymapfile;

Map* Map::LoadMapfile(const char* in_zonename, const char *directory) {

	if(createNewMapFile){
		strcat(fileNamePrefix, in_zonename);
		strcat(fileNamePrefix, fileNameSuffix);
		myfile.open(fileNamePrefix);
		strcpy(fileNamePrefix, "./Maps/3DGraphs/");
		strcat(fileNamePrefix, in_zonename);
		strcat(fileNamePrefix, "3dgraph");
		strcat(fileNamePrefix, fileNameSuffix);
		_3dgraphfile.open(fileNamePrefix);
	}

	FILE *fp;
	char zBuf[64];
	char cWork[256];
	Map* ret = 0;
	strcpy(thisZonesName, in_zonename);
	
	//have to convert to lower because the short names im getting
	//are not all lower anymore, copy since strlwr edits the str.
	strncpy(zBuf, in_zonename, 64);
	zBuf[63] = '\0';
	
	if(directory == NULL)
		directory = MAP_DIR;
	if(!useNewMapFile)
		snprintf(cWork, 250, "%s/%s.map", directory, strlwr(zBuf));
	else
		snprintf(cWork, 250, "%s/%s.txt", directory, strlwr(zBuf));
	
	if ((fp = fopen( cWork, "rb" ))) {
		ret = new Map();
		if(ret != NULL) {
			ret->loadMap(fp);
			cout<<"Map loaded:"<<cWork<<endl;
		} else {
			cout<<"Map NOT loaded:"<<cWork<<endl;
		}
		fclose(fp);
	}
	else {
		cout<<"Map NOT found:"<<cWork<<endl;
	}
	return ret;
}

Map::Map() {
	_minz = FLT_MAX;
	_minx = FLT_MAX;
	_miny = FLT_MAX;
	_maxz = FLT_MIN;
	_maxx = FLT_MIN;
	_maxy = FLT_MIN;
	
	m_Faces = 0;
	m_Nodes = 0;
	m_FaceLists = 0;
	mFinalFaces = NULL;
	mNodes = NULL;
	mFaceLists = NULL;
}

bool Map::loadMap(FILE *fp) {
//#ifndef INVERSEXY
////#warning Map files do not work without inverted XY
//	return(false);
//#endif

	if(!useNewMapFile){
		mapHeader head;
		if(fread(&head, sizeof(head), 1, fp) != 1) {
			//map read error.
			return(false);
		}
		if(head.version != MAP_VERSION) {
			//invalid version... if there really are multiple versions,
			//a conversion routine could be possible.
			printf("Invalid map version 0x%lx\n", head.version);
			return(false);
		}

		if(createNewMapFile){
			myfile<<"Version: "<<head.version<<endl;
			myfile<<"Faces: "<<head.face_count<<endl;
			myfile<<"Nodes: "<<head.node_count<<endl;
			myfile<<"Facelists: "<<head.facelist_count<<endl;
		}
		
		printf("Map header: %lu faces, %u nodes, %lu facelists\n", head.face_count, head.node_count, head.facelist_count);
		
		m_Faces = head.face_count;
		m_Nodes = head.node_count;
		m_FaceLists = head.facelist_count;
		
		/*fread(&m_Vertex, 4, 1, fp);
		fread(&m_Faces , 4, 1, fp);*/
		//mFinalVertex = new VERTEX[m_Vertex];
		mFinalFaces = new FACE [m_Faces];
		mNodes = new NODE[m_Nodes];
		mFaceLists = new unsigned long[m_FaceLists];
		
		//fread(mFinalVertex, m_Vertex, sizeof(VERTEX), fp);
		unsigned long count;
		if((count=fread(mFinalFaces, sizeof(FACE), m_Faces , fp)) != m_Faces) {
			printf("Unable to read %lu faces from map file, got %lu.\n", m_Faces, count);
			return(false);
		}
		if(fread(mNodes, sizeof(NODE), m_Nodes, fp) != m_Nodes) {
			printf("Unable to read %lu faces from map file.\n", m_Nodes);
			return(false);
		}
		if(fread(mFaceLists, sizeof(unsigned long), m_FaceLists, fp) != m_FaceLists) {
			printf("Unable to read %lu faces from map file.\n", m_FaceLists);
			return(false);
		}

		unsigned long i;
		float v;
		for(i = 0; i < m_Faces; i++) {
			v = Vmax3(x, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
			if(v > _maxx)
				_maxx = v;
			v = Vmin3(x, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
			if(v < _minx)
				_minx = v;
			v = Vmax3(y, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
			if(v > _maxy)
				_maxy = v;
			v = Vmin3(y, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
			if(v < _miny)
				_miny = v;
			v = Vmax3(z, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
			if(v > _maxz)
				_maxz = v;
			v = Vmin3(z, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
			if(v < _minz)
				_minz = v;
		}

		if(createNewMapFile){
			myfile<<"<MapCoords>"<<endl;
			myfile<<"  <MaxX> "<<_maxx<<endl;
			myfile<<"  <MinX> "<<_minx<<endl;
			myfile<<"  <MaxY> "<<_maxy<<endl;
			myfile<<"  <MinY> "<<_miny<<endl;
			myfile<<"  <MaxZ> "<<_maxz<<endl;
			myfile<<"  <MinZ> "<<_minz<<endl;
			myfile<<"</MapCoords>"<<endl;

			for(i = 0; i < m_Faces; i++) {
				myfile<<"<Face> "<<i<<endl;
				myfile<<"  <LocA> "<<mFinalFaces[i].a.x<<" "<<mFinalFaces[i].a.y<<" "<<mFinalFaces[i].a.z<<endl;
				myfile<<"  <LocB> "<<mFinalFaces[i].b.x<<" "<<mFinalFaces[i].b.y<<" "<<mFinalFaces[i].b.z<<endl;
				myfile<<"  <LocC> "<<mFinalFaces[i].c.x<<" "<<mFinalFaces[i].c.y<<" "<<mFinalFaces[i].c.z<<endl;
				myfile<<"  <Devi> "<<mFinalFaces[i].nx<<" "<<mFinalFaces[i].ny<<" "<<mFinalFaces[i].nz<<" "<<mFinalFaces[i].nd<<endl;
				myfile<<"</Face>"<<endl;
				_3dgraphfile<<mFinalFaces[i].a.x<<","<<mFinalFaces[i].a.y<<","<<mFinalFaces[i].a.z<<endl;
				_3dgraphfile<<mFinalFaces[i].b.x<<","<<mFinalFaces[i].b.y<<","<<mFinalFaces[i].b.z<<endl;
				_3dgraphfile<<mFinalFaces[i].c.x<<","<<mFinalFaces[i].c.y<<","<<mFinalFaces[i].c.z<<endl;
			}

			for(i = 0; i < m_Nodes; i++){
				myfile<<"<Node> "<<i<<endl;
				myfile<<"  <MaxX> "<<mNodes[i].maxx<<endl;
				myfile<<"  <MinX> "<<mNodes[i].minx<<endl;
				myfile<<"  <MaxY> "<<mNodes[i].maxy<<endl;
				myfile<<"  <MinY> "<<mNodes[i].miny<<endl;
				myfile<<"  <Flag> "<<int(mNodes[i].flags)<<endl;
				for(int k = 0; k < 4; k++)
					myfile<<"  <Nodes> "<<mNodes[i].nodes[k]<<endl;
				myfile<<"  <count> "<<mNodes[i].faces.count<<endl;
				myfile<<"  <offset> "<<mNodes[i].faces.offset<<endl;
				myfile<<"</Node>"<<endl;
			}

			for(i = 0; i < m_FaceLists; i++){
				myfile<<"<FaceList> "<<i<<endl;
				myfile<<"  <Data> "<<mFaceLists[i]<<endl;
			}
		}

		printf("Loaded map: %lu vertices, %lu faces\n", m_Faces*3, m_Faces);
		printf("Map BB: (%.2f -> %.2f, %.2f -> %.2f, %.2f -> %.2f)\n", _minx, _maxx, _miny, _maxy, _minz, _maxz);

		_3dgraphfile.close();
		myfile.close();

		return(true);
	}else{
		int ID;
		float x, y, z, deviation, value;
		float nodeMinX, nodeMaxX, nodeMinY, nodeMaxY;
		unsigned long count, offset;
		unsigned char miscFlag;
		unsigned short miscNodes[4] = {0};
		char buffer[30] = "";

		strcpy(fileNamePrefix, "./Maps/Maps/");
		strcat(fileNamePrefix, thisZonesName);
		strcat(fileNamePrefix, fileNameSuffix);
		mymapfile.open(fileNamePrefix);

		mapHeader head;
		mymapfile>>buffer>>count;
		head.version = count;
		mymapfile>>buffer>>count;
		head.face_count = count;
		mymapfile>>buffer>>count;
		head.node_count = count;
		mymapfile>>buffer>>count;
		head.facelist_count = count;

		m_Faces = head.face_count;
		m_Nodes = head.node_count;
		m_FaceLists = head.facelist_count;

		mFinalFaces = new FACE [m_Faces];
		mNodes = new NODE[m_Nodes];
		mFaceLists = new unsigned long[m_FaceLists];
		
		mymapfile>>buffer;
		mymapfile>>buffer>>value;
		this->_maxx = value;
		mymapfile>>buffer>>value;
		this->_minx = value;
		mymapfile>>buffer>>value;
		this->_maxy = value;
		mymapfile>>buffer>>value;
		this->_miny = value;
		mymapfile>>buffer>>value;
		this->_maxz = value;
		mymapfile>>buffer>>value;
		this->_minz = value;
		mymapfile>>buffer;

		for(int i = 0; i < m_Faces; i++){
			mymapfile>>buffer>>ID;
			mymapfile>>buffer>>x>>y>>z;
			mFinalFaces[i].a.x = x;
			mFinalFaces[i].a.y = y;
			mFinalFaces[i].a.z = z;
			mymapfile>>buffer>>x>>y>>z;
			mFinalFaces[i].b.x = x;
			mFinalFaces[i].b.y = y;
			mFinalFaces[i].b.z = z;
			mymapfile>>buffer>>x>>y>>z;
			mFinalFaces[i].c.x = x;
			mFinalFaces[i].c.y = y;
			mFinalFaces[i].c.z = z;
			mymapfile>>buffer>>x>>y>>z>>deviation;
			mFinalFaces[i].nx = x;
			mFinalFaces[i].ny = y;
			mFinalFaces[i].nz = z;
			mFinalFaces[i].nd = deviation;
			mymapfile>>buffer;
		}
		for(int i = 0; i < m_Nodes; i++){
			mymapfile>>buffer>>ID;
			mymapfile>>buffer>>nodeMaxX;
			mymapfile>>buffer>>nodeMinX;
			mymapfile>>buffer>>nodeMaxY;
			mymapfile>>buffer>>nodeMinY;
			mNodes[i].maxx = nodeMaxX;
			mNodes[i].minx = nodeMinX;
			mNodes[i].maxy = nodeMaxY;
			mNodes[i].miny = nodeMinY;
			mymapfile>>buffer>>miscFlag;
			mNodes[i].flags = miscFlag;
			for(int k = 0; k < 4; k++){
				mymapfile>>buffer>>ID;
				mNodes[i].nodes[k] = ID;
			}
			mymapfile>>buffer>>count;
			mymapfile>>buffer>>offset;
			mNodes[i].faces.count = count;
			mNodes[i].faces.offset = offset;
			mymapfile>>buffer;
		}
		for(int i = 0; i < m_FaceLists; i++){
			mymapfile>>buffer>>ID;
			mymapfile>>buffer>>offset;
			mFaceLists[i] = offset;
		}
		return(true);
	}
	return(false);
}

Map::~Map() {
	//safe_delete(mFinalVertex);
	safe_delete(mFinalFaces);
	safe_delete(mNodes);
	safe_delete(mFaceLists);
	//RecFreeNode( mRoot );
}


NodeRef Map::SeekNode( NodeRef node_r, float x, float y ) {
	if(node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NODE_NONE);
	}
	PNODE _node = &mNodes[node_r];
	if( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy ) {
		if( _node->flags & nodeFinal ) {
			return node_r;
		}
		NodeRef tmp = NODE_NONE;
		tmp = SeekNode( _node->nodes[0], x, y );
		if( tmp != NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[1], x, y );
		if( tmp != NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[2], x, y );
		if( tmp != NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[3], x, y );
		if( tmp != NODE_NONE ) return tmp;
	}
	return(NODE_NONE);
}

// maybe precalc edges.
int* Map::SeekFace(  NodeRef node_r, float x, float y ) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NULL);
	}
	PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(NULL);   //not a final node... could find the proper node...
	}
	
	
//printf("Seeking face for (%.2f, %.2f) with root 0x%x.\n", x, y, _node);
	float	dx,dy;
	float	nx,ny;
	int		*face = mCandFaces;
	unsigned long i;
	for( i=0;i<_node->faces.count;i++ ) {
		const FACE &cf = mFinalFaces[ mFaceLists[_node->faces.offset + i] ];
		const VERTEX &v1 = cf.a;
		const VERTEX &v2 = cf.b;
		const VERTEX &v3 = cf.c;
		
		dx = v2.x - v1.x; dy = v2.y - v1.y;
		nx =    x - v1.x; ny =    y - v1.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v3.x - v2.x; dy = v3.y - v2.y;
		nx =    x - v2.x; ny =    y - v2.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v1.x - v3.x; dy = v1.y - v3.y;
		nx =    x - v3.x; ny =    y - v3.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		*face++ = mFaceLists[_node->faces.offset + i];
	}
	*face = -1;
	return mCandFaces;
}

// can be op?
float Map::GetFaceHeight( int _idx, float x, float y ) {
	PFACE	pface = &mFinalFaces[ _idx ];
	return ( pface->nd - x * pface->nx - y * pface->ny ) / pface->nz;
}

//FatherNitwit's LOS code...
//Algorithm stolen from internet
//p1=start of segment
//p2=end of segment

bool Map::LineIntersectsZone(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) {
	//_ZP(Map_LineIntersectsZone);
	VERTEX step;
	VERTEX cur = start;
	
	step.x = end.x - start.x;
	step.y = end.y - start.y;
	step.z = end.z - start.z;
	float factor = step_mag / sqrt(step.x*step.x + step.y*step.y + step.z*step.z);
	step.x *= factor;
	step.y *= factor;
	step.z *= factor;
	
	NodeRef cnode, lnode, finalnode;
	lnode = NODE_NONE;	//last node visited
	
	finalnode = SeekNode(GetRoot(), end.x, end.y);
	
	//while we are not past end
	//always do this once, even if start == end.
	//Yeahlight: TODO: CRITICAL: Infinite loop in here
	int16 breakOutCounter = 0;
	do {
		//look at current location
		cnode = SeekNode(GetRoot(), cur.x, cur.y);
		if(cnode != NODE_NONE && cnode != lnode) {
			if(LineIntersectsNode(cnode, start, end, result, on))
				return(true);
			lnode = cnode;
		}
		if(cnode == finalnode)
			return(false);	//we checked in the node the end point is in
							//we will never find another node before the end
		
		//move 1 step
		cur.x += step.x;
		cur.y += step.y;
		cur.z += step.z;
		
		//watch for end conditions
		if ( (cur.x > end.x && end.x > start.x) || (cur.x < end.x && end.x < start.x) ) {
			cur.x = end.x;
		}
		if ( (cur.y > end.y && end.y > start.y) || (cur.y < end.y && end.y < start.y) ) {
			cur.y = end.y;
		}
		if ( (cur.z > end.z && end.z > start.z) || (cur.z < end.z && end.z < start.z) ) {
			cur.z = end.z;
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
		if(breakOutCounter++ > 5000)
			return(false);
	} while(cur.x != end.x || cur.y != end.y || cur.z != end.z);
	
	//walked entire line and didnt run into anything...
	return(false);
}

bool Map::LocWithinNode( NodeRef node_r, float x, float y ) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(false);
	}
	PNODE _node = &mNodes[node_r];
	//this function exists so nobody outside of MAP needs to know
	//how the NODE sturcture works
	return( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy );
}

bool Map::LineIntersectsNode( NodeRef node_r, VERTEX p1, VERTEX p2, VERTEX *result, FACE **on) {
	//_ZP(Map_LineIntersectsNode);
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(true);   //can see through empty nodes, just allow LOS on error...
	}
	PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(true);   //not a final node... not sure best action
	}
	
	unsigned long i;
	
	PFACE cur;
	unsigned long *cfl = mFaceLists + _node->faces.offset;
	
	for(i = 0; i < _node->faces.count; i++) {
		if(*cfl > m_Faces)
			continue;	//watch for invalid lists, they seem to happen
		cur = &mFinalFaces[ *cfl ];
		if(LineIntersectsFace(cur,p1, p2, result)) {
			if(on != NULL)
				*on = cur;
			return(true);
		}
		cfl++;
	}

//printf("Checked %ld faces and found none in the way.\n", i);

	return(false);
}


bool Map::LineIntersectsFace( PFACE cface, VERTEX p1, VERTEX p2, VERTEX *result) {
	if( cface == NULL ) {
		return(false);  //cant intersect a face we dont have... i guess
	}

#define ABS(x) ((x)<0?-(x):(x))
	
	const VERTEX &pa = cface->a;
	const VERTEX &pb = cface->b;
	const VERTEX &pc = cface->c;
	
	//quick bounding box checks
	float tbb;
	
	tbb = Vmin3(x, pa, pb, pc);
	if(p1.x < tbb && p2.x < tbb)
		return(false);
	tbb = Vmin3(y, pa, pb, pc);
	if(p1.y < tbb && p2.y < tbb)
		return(false);
	tbb = Vmin3(z, pa, pb, pc);
	if(p1.z < tbb && p2.z < tbb)
		return(false);
	
	tbb = Vmax3(x, pa, pb, pc);
	if(p1.x > tbb && p2.x > tbb)
		return(false);
	tbb = Vmax3(y, pa, pb, pc);
	if(p1.y > tbb && p2.y > tbb)
		return(false);
	tbb = Vmax3(z, pa, pb, pc);
	if(p1.z > tbb && p2.z > tbb)
		return(false);
	
	//begin attempt 2
#define ABS(x) ((x)<0?-(x):(x))
//#define RTOD 57.2957795 	//radians to degrees constant.

	float d;
	float denom,mu;
	VERTEX n, intersect;
	
//	FACE *thisface = &mFinalFaces[ _node->pfaces[ i ] ];
	
	VERTEX *p = &intersect;
	if(result != NULL)
		p = result;

	// Calculate the parameters for the plane
	//recalculate from points
#ifndef TRUST_MAPFILE_NORMALS
	n.x = (pb.y - pa.y)*(pc.z - pa.z) - (pb.z - pa.z)*(pc.y - pa.y);
	n.y = (pb.z - pa.z)*(pc.x - pa.x) - (pb.x - pa.x)*(pc.z - pa.z);
	n.z = (pb.x - pa.x)*(pc.y - pa.y) - (pb.y - pa.y)*(pc.x - pa.x);
	Normalize(&n);
	d = - n.x * pa.x - n.y * pa.y - n.z * pa.z;
#else
	//use precaled data from .map file
	n.x = cface->nx;
	n.y = cface->ny;
	n.z = cface->nz;
	d = cface->nd;
#endif
	
	//try inverting the normals...
	n.x = -n.x;
	n.y = -n.y;
	n.z = -n.z;
	d = - n.x * pa.x - n.y * pa.y - n.z * pa.z;	//recalc
	
	
	// Calculate the position on the line that intersects the plane
	denom = n.x * (p2.x - p1.x) + n.y * (p2.y - p1.y) + n.z * (p2.z - p1.z);
	if (ABS(denom) < EPS)         // Line and plane don't intersect
	  return(false);
	mu = - (d + n.x * p1.x + n.y * p1.y + n.z * p1.z) / denom;
	if (mu < 0 || mu > 1)   // Intersection not along line segment
	  return(false);
	p->x = p1.x + mu * (p2.x - p1.x);
	p->y = p1.y + mu * (p2.y - p1.y);
	p->z = p1.z + mu * (p2.z - p1.z);


/*	//old method, slow as hell due to acos(), but it works well

	float a1,a2,a3;
	float total;
	VERTEX pa1,pa2,pa3;

	pa1.x = pa.x - p->x;
	pa1.y = pa.y - p->y;
	pa1.z = pa.z - p->z;
	Normalize(&pa1);
	pa2.x = pb.x - p->x;
	pa2.y = pb.y - p->y;
	pa2.z = pb.z - p->z;
	Normalize(&pa2);
	pa3.x = pc.x - p->x;
	pa3.y = pc.y - p->y;
	pa3.z = pc.z - p->z;
	Normalize(&pa3);
	a1 = pa1.x*pa2.x + pa1.y*pa2.y + pa1.z*pa2.z;
	a2 = pa2.x*pa3.x + pa2.y*pa3.y + pa2.z*pa3.z;
	a3 = pa3.x*pa1.x + pa3.y*pa1.y + pa3.z*pa1.z;
	
//holy hell these 3 acos are slow, we need to rewrite this...
//	total = (acos(a1) + acos(a2) + acos(a3));
//	if (ABS(total - 2*M_PI) > EPS)
	total = (acos(a1) + acos(a2) + acos(a3)) * 57.2957795;
	if (ABS(total - 360) > EPS)
	  return(false);

	return(true);
*/

/*	
	//yet another failed method, project triangle and point into
	//2 space based on largest component of the normal
	//and check the triangle there.
	float tx, ty, tz;
	if(n.x < 0)
		tx = -n.x;
	else
		tx = n.x;
	if(n.y < 0)
		ty = -n.y;
	else
		ty = n.y;
	if(n.z < 0)
		tz = -n.z;
	else
		tz = n.z;
	
	VERTEX pa2, pb2, pc2;
	if(tx < ty) {
		//keep x
		if(tz < ty) {
			//keep z, drop y
			pa2.x = pa.x; pa2.y = pa.z;
			pb2.x = pb.x; pb2.y = pb.z;
			pc2.x = pc.x; pc2.y = pc.z;
		} else {
			//keep y, drop z...
			pa2.x = pa.x; pa2.y = pa.y;
			pb2.x = pb.x; pb2.y = pb.y;
			pc2.x = pc.x; pc2.y = pc.y;
		}
	} else {
		//keep y
		if(tz < tx) {
			//keep z, drop x
			pa2.x = pa.x; pa2.y = pa.z;
			pb2.x = pb.x; pb2.y = pb.z;
			pc2.x = pc.x; pc2.y = pc.z;
		} else {
			//keep y, drop z...
			pa2.x = pa.x; pa2.y = pa.y;
			pb2.x = pb.x; pb2.y = pb.y;
			pc2.x = pc.x; pc2.y = pc.y;
		}
	}
	
	// Determine whether or not the intersection point is bounded by pa,pb,pc
#define Sign(p1, p2, p3) \
	((p1->x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1->y - p3.y))
  bool b1, b2, b3;

  b1 = Sign(p, pa2, pb2) < 0.0f;
  b2 = Sign(p, pb2, pc2) < 0.0f;
  b3 = Sign(p, pc2, pa2) < 0.0f;

  return ((b1 == b2) && (b2 == b3));
*/
	
/*	//not working well, seems to block LOS a lot

	//a new check based on barycentric coordinates, stolen from 
	//http://www.flipcode.com/cgi-bin/fcmsg.cgi?thread_show=7766
	float xcp, ycp, xab, yab, xac, yac;
	float divb;
	VERTEX barycoords;

	xcp = p->x - pc.x;
	ycp = p->z - pc.z;
	if( xcp == 0.f && ycp == 0.f )
		return(true);

	xab = pb.x - pa.x;
	yab = pb.z - pa.z;
	divb = xab * ycp - yab * xcp;
	if( divb == 0.f )
		return(false);

	xac = pc.x - pa.x;
	yac = pc.z - pa.z;

	barycoords.y = ( -yac * xcp + xac * ycp ) / divb;
	if( barycoords.y < -EPS || barycoords.y > (1+EPS) )
		return(false); // small error tolerance
	if( barycoords.y < 0.f )
		barycoords.y = 0.f;
	if( barycoords.y > 1.f )
		barycoords.y = 1.f;

//	barycoords.x = 1.f - barycoords.y;

	if( xcp != 0.f ) {
		float div = -xac + barycoords.y * xab;
		if( div == 0.f )
			return(false); // flat triangle
		barycoords.z = 1.f - xcp / div ;
	} else {
		float div = -yac + barycoords.y * yab;
		if( div == 0.f )
			return(false); // flat triangle
		barycoords.z = 1.f - ycp / div ;
	}

	if( barycoords.z < -EPS || barycoords.z > (1+EPS) )
		return(false);
//	if( barycoords.z < 0.f )
//		barycoords.z = 0.f;
//	if( barycoords.z > 1.f )
//		barycoords.z = 1.f;

//	barycoords.x *= 1.f - barycoords.z;
//	barycoords.y *= 1.f - barycoords.z;

	return(true);
*/
	
/*
	Yet another method adapted from this code:
  Vec3 pa1 = pa - p;
  Vec3 pa2 = pb - p;
  float d = pa1.cross(pa2).dot(n);
  if (d < 0) return false;
  Vec3 pa3 = pb - p;
  d = pa2.cross(pa3).dot(n);
  if (d < 0) return false;
  d = pa3.cross(pa1).dot(n);
  if (d < 0) return false;
  return true;
*/
	
	//in practice, this seems to actually take longer
	//than the arc cosine method above...
	n.x = -n.x;
	n.y = -n.y;
	n.z = -n.z;
	VERTEX pa1,pa2,pa3, tmp;
	float t;
	
	//pa1 = pa - p
	pa1.x = pa.x - p->x;
	pa1.y = pa.y - p->y;
	pa1.z = pa.z - p->z;
	
	//pa2 = pb - p
	pa2.x = pb.x - p->x;
	pa2.y = pb.y - p->y;
	pa2.z = pb.z - p->z;
	
	//tmp = pa1 cross pa2
	tmp.x = pa1.y * pa2.z - pa1.z * pa2.y;
	tmp.y = pa1.z * pa2.x - pa1.x * pa2.z;
	tmp.z = pa1.x * pa2.y - pa1.y * pa2.x;
	
	//t = tmp dot n
	t = tmp.x * n.x + tmp.y * n.y + tmp.z * n.z;
	if(t < 0)
		return(false);
//printf("t = %f\n", t);
	
	//pa3 = pb - p
	pa3.x = pc.x - p->x;
	pa3.y = pc.y - p->y;
	pa3.z = pc.z - p->z;
	
	//tmp = pa2 cross pa3
	tmp.x = pa2.y * pa3.z - pa2.z * pa3.y;
	tmp.y = pa2.z * pa3.x - pa2.x * pa3.z;
	tmp.z = pa2.x * pa3.y - pa2.y * pa3.x;
	
	//t = tmp dot n
	t = tmp.x * n.x + tmp.y * n.y + tmp.z * n.z;
	if(t < 0)
		return(false);
//printf("t = %f\n", t);
	
	//tmp = pa3 cross pa1
	tmp.x = pa3.y * pa1.z - pa3.z * pa1.y;
	tmp.y = pa3.z * pa1.x - pa3.x * pa1.z;
	tmp.z = pa3.x * pa1.y - pa3.y * pa1.x;
	
	//t = tmp dot n
	t = tmp.x * n.x + tmp.y * n.y + tmp.z * n.z;
	if(t < 0)
		return(false);
//printf("t = %f\n", t);
	
	return(true);
}

void Map::Normalize(VERTEX *p) {
	float len = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
	p->x /= len;
	p->y /= len;
	p->z /= len;
}

float Map::FindBestZ( NodeRef node_r, VERTEX p1, VERTEX *result, FACE **on) {
	if(node_r == GetRoot()) {
		node_r = SeekNode(node_r, p1.x, p1.y);
	}
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(BEST_Z_INVALID);
	}
	const PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(BEST_Z_INVALID);   //not a final node... could find the proper node...
	}
	
	p1.z++;

	VERTEX tmp_result;	//dummy placeholder if they do not ask for a result.
	if(result == NULL)
		result = &tmp_result;
	
	VERTEX p2(p1);
	p2.z = BEST_Z_INVALID;
	
	float best_z = BEST_Z_INVALID;
	int zAttempt;

	unsigned long i;

	PFACE cur;

	// If we don't find a bestZ on the first attempt, we try again from a position CurrentZ + 10 higher
	// This is in case the pathing between waypoints temporarily sends the NPC below ground level.
	// 
	for(zAttempt=1; zAttempt<=2; zAttempt++) {

		const unsigned long *cfl = mFaceLists + _node->faces.offset;

		for(i = 0; i < _node->faces.count; i++) {
			if(*cfl > m_Faces)
		               continue;       //watch for invalid lists, they seem to happen, e.g. in eastwastes.map

			cur = &mFinalFaces[ *cfl ];
			if(LineIntersectsFace(cur, p1, p2, result)) {
				if (result->z > best_z) {
					if(on != NULL)
						*on = cur;
					best_z = result->z;
				}
			}
			cfl++;
		}

		if(best_z != BEST_Z_INVALID) return best_z;

		 p1.z = p1.z + 10 ;   // If we can't find a best Z, the NPC is probably just under the world. Try again from 10 units higher up.
	}

	return best_z;
}