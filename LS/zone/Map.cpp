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
#include "../common/debug.h"
#include <stdio.h>
#include <string.h>
#include <iostream.h>

#include "map.h"
#ifdef WIN32
#define snprintf	_snprintf
#endif

Map* Map::LoadMapfile(const char* in_zonename) {
	FILE *fp;
	char cWork[256];
	Map* ret = 0;
	
	snprintf(cWork, 250, "Maps/%s.map", in_zonename);
	
	if ((fp = fopen( cWork, "rb" ))) {
		ret = new Map(fp);
		fclose(fp);
		cout << "Map: " << cWork << " loaded." << endl;
	}
	else {
		//	cout << "Map: " << cWork << " not found." << endl;
	}
	return ret;
}

Map::Map(FILE* fp)
{
	fread(&m_Vertex, 4, 1, fp);
	fread(&m_Faces , 4, 1, fp);
	mFinalVertex	= new VERTEX[m_Vertex];
	mFinalFaces		= new FACE	[m_Faces];
	
	fread(mFinalVertex, m_Vertex, sizeof(VERTEX), fp);
	fread(mFinalFaces, m_Faces , sizeof(FACE),	fp);
	mRoot = new NODE;
	RecLoadNode(mRoot, fp );
}

Map::~Map() {
	delete mFinalVertex;
	delete mFinalFaces;
	RecFreeNode( mRoot );
}

PNODE Map::SeekNode( PNODE _node, float x, float y )
{
	if( !_node ) return NULL;
	if( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy )
	{
		if( _node->mask )
		{
			PNODE tmp;
			tmp = SeekNode( _node->node1, x, y );
			if( tmp ) return tmp;
			tmp = SeekNode( _node->node2, x, y );
			if( tmp ) return tmp;
			tmp = SeekNode( _node->node3, x, y );
			if( tmp ) return tmp;
			tmp = SeekNode( _node->node4, x, y );
			if( tmp ) return tmp;
		}
		else
			return _node;
	}
	return NULL;
}

// maybe precalc edges.
int* Map::SeekFace( PNODE _node, float x, float y )
{
	float	dx,dy;
	float	nx,ny;
	int		*face = mCandFaces;
	for( int i=0;i<_node->nfaces;i++ )
	{
		VERTEX v1 = mFinalVertex[ mFinalFaces[ _node->pfaces[ i ] ].a ];
		VERTEX v2 = mFinalVertex[ mFinalFaces[ _node->pfaces[ i ] ].b ];
		VERTEX v3 = mFinalVertex[ mFinalFaces[ _node->pfaces[ i ] ].c ];
		
		dx = v2.x - v1.x; dy = v2.y - v1.y;
		nx =    x - v1.x; ny =    y - v1.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v3.x - v2.x; dy = v3.y - v2.y;
		nx =    x - v2.x; ny =    y - v2.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v1.x - v3.x; dy = v1.y - v3.y;
		nx =    x - v3.x; ny =    y - v3.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		*face++ = _node->pfaces[ i ];
	}
	*face = -1;
	return mCandFaces;
}

// can be op?
float Map::GetFaceHeight( int _idx, float x, float y )
{
	PFACE	pface = &mFinalFaces[ _idx ];
	return ( pface->nd - x * pface->nx - y * pface->ny ) / pface->nz;
}

// Load Time Dont OP!!!
void Map::RecLoadNode( PNODE	_node, void *l_f )
{
	fread( &_node->minx, 4, 1, (FILE*)l_f );
	fread( &_node->miny, 4, 1, (FILE*)l_f );
	fread( &_node->maxx, 4, 1, (FILE*)l_f );
	fread( &_node->maxy, 4, 1, (FILE*)l_f );
	fread( &_node->nfaces, 4, 1, (FILE*)l_f );
	_node->mask		= 0;
	_node->node1 = _node->node2 = _node->node3 = _node->node4 = NULL;
	if( _node->nfaces )
	{
		_node->pfaces = new unsigned int[_node->nfaces];
		fread(  _node->pfaces, 4, _node->nfaces, (FILE*)l_f );
	}
	else
	{
		_node->pfaces = NULL;
		fread( &_node->mask, 1, 1, (FILE*)l_f );
		if( _node->mask&0x01 )	_node->node1 = new NODE;
		if( _node->mask&0x02 )	_node->node2 = new NODE;
		if( _node->mask&0x04 )	_node->node3 = new NODE;
		if( _node->mask&0x08 )	_node->node4 = new NODE;
		
		if( _node->node1 )	RecLoadNode( _node->node1, l_f );
		if( _node->node2 )	RecLoadNode( _node->node2, l_f );
		if( _node->node3 )	RecLoadNode( _node->node3, l_f );
		if( _node->node4 )	RecLoadNode( _node->node4, l_f );
	}
}

// Free Time Dont OP!!!
void	Map::RecFreeNode( PNODE	_node )
{
	if( _node )
	{
		if( _node->node1 )	RecFreeNode( _node->node1 );
		if( _node->node2 )	RecFreeNode( _node->node2 );
		if( _node->node3 )	RecFreeNode( _node->node3 );
		if( _node->node4 )	RecFreeNode( _node->node4 );
		// Borra el nodo
		if( _node->pfaces )
		{
			delete _node->pfaces;
			_node->pfaces = NULL;
		}
		delete _node;
		_node = NULL;
	}
}

