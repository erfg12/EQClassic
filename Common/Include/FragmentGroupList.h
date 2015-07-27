// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef EQFRAGMENT_H
#define EQFRAGMENT_H

#include "types.h"
#include "linked_list.h"
#include "FragmentGroup.h"

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class FragmentGroupList
			{
			public:
				void Add(FragmentGroup* add_group);
				FragmentGroup* Get(int16 find_seq);
				void Remove(int16 remove_seq);

			private:
				LinkedList<FragmentGroup*> fragment_group_list;
			};

		}
	}
}
#endif
