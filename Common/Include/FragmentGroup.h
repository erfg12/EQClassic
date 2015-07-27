// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef FRAGMENTGROUP_H
#define FRAGMENTGROUP_H

#include "types.h"
#include "linked_list.h"
#include "Fragment.h"

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class FragmentGroup
			{
			public:
				FragmentGroup(int16 seq, int16 opcode, int16 num_fragments);
				~FragmentGroup();

				void Add(int16 frag_id, uchar* data, int32 size);
				uchar* AssembleData(int32* size);

				int16 GetSeq()
				{
					return seq;
				}

				int16 GetOpcode()
				{
					return opcode;
				}

			private:
				int16 seq;				// Sequence number
				int16 opcode;			// Fragment group's opcode
				int16 num_fragments;	//TODO: What is this one?
				Fragment* fragment;		//TODO: What is this one?
			};

		}
	}
}
#endif