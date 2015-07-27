// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <cstring>
#include <iostream>
#include <fstream>
#include "Fragment.h"

using namespace std;

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			Fragment::Fragment()
			{
				data = 0;
				size = 0;
				memset(this->data, 0, this->size);
			}

			Fragment::~Fragment()
			{
				safe_delete_array(data);
			}

			void Fragment::SetData(uchar* d, int32 s)
			{
				safe_delete_array(data);
				size = s;
				data = new uchar[s];
				memcpy(data, d, s);
			}
		}
	}
}