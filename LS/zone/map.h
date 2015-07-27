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
#ifndef MAP_H
#define MAP_H

typedef struct _vertex{
	unsigned int order;
	float	x, y, z;
}VERTEX, *PVERTEX;

typedef struct _face{
	unsigned int	a, b, c;
	float nx, ny, nz, nd;
}FACE, *PFACE;

typedef struct _node{
	float						minx, miny, maxx, maxy;
	int							nfaces;
	unsigned int *	pfaces;
	char						mask;
	struct  _node	*	node1, *node2, *node3, *node4;
}NODE, *PNODE;

class Map;

class Map {
public:
	static Map* Map::LoadMapfile(const char* in_zonename);
	Map(FILE* fp);
	~Map();

	PNODE		SeekNode( PNODE _node, float x, float y );
	int		 *SeekFace( PNODE _node, float x, float y );
	float		GetFaceHeight( int _idx, float x, float y );
	inline unsigned int		GetVertexNumber( ) {return m_Vertex; }
	inline unsigned int		GetFacesNumber( ) { return m_Faces; }
	inline PVERTEX	GetVertex( int _idx ) {return mFinalVertex + _idx;	}
	inline PFACE		GetFace( int _idx) {return mFinalFaces + _idx;		}
	inline PNODE		GetRoot( ) { return mRoot; }
private:
	unsigned int		m_Vertex;
	unsigned int		m_Faces;
	PVERTEX					mFinalVertex;
	PFACE						mFinalFaces;
	PNODE						mRoot;
	int							mCandFaces[100];

	void	RecLoadNode( PNODE	_node, void *l_f );
	void	RecFreeNode( PNODE	_node						 );
};
#endif

