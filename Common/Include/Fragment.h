// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#ifndef FRAGMENT_H
#define FRAGMENT_H

#include "types.h"
#include "linked_list.h"

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class Fragment
			{
			public:
				Fragment();
				~Fragment();

				void SetData(uchar* d, int32 s);

				uchar* GetData() 
				{
					return this->data;
				}

				int32  GetSize() 
				{ 
					return this->size; 
				}

			private:
				uchar*	data;
				int32	size;
			};
		}
	}
}
#endif