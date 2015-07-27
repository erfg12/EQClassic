// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <cstring>
#include "FragmentGroupList.h"
#include "config.h"
#include "EQCException.hpp"

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			void FragmentGroupList::Add(FragmentGroup* add_group)
			{
				fragment_group_list.Insert(add_group);
			}
			FragmentGroup* FragmentGroupList::Get(int16 find_seq)
			{
				LinkedListIterator<FragmentGroup*> iterator(fragment_group_list);

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (iterator.GetData()->GetSeq() == find_seq)
					{
						return iterator.GetData();
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
				return 0;
			}

			void FragmentGroupList::Remove(int16 remove_seq)
			{
				LinkedListIterator<FragmentGroup*> iterator(fragment_group_list);

				iterator.Reset();
				while(iterator.MoreElements())
				{
					if (iterator.GetData()->GetSeq() == remove_seq)
					{
						iterator.RemoveCurrent();
						return;
					}
					iterator.Advance();
					//Yeahlight: Zone freeze debug
					if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
						EQC_FREEZE_DEBUG(__LINE__, __FILE__);
				}
			}
		}
	}
}